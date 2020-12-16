#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"

#define WATERING_THRESHOLD 2000
#define GPIO_WATER_PUMP GPIO_NUM_18
#define ADC_CHANNEL_SOIL_MOISTURE ADC1_CHANNEL_6

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define MIN(a, b) ((a) < (b) ? (a) : (b))

httpd_handle_t start_webserver(void);

static const char *TAG = "main";

esp_err_t stop_pump(gpio_num_t port);

void initialize_cJSON()
{
  cJSON_Hooks hooks = {
    .malloc_fn = malloc,
    .free_fn = free
  };
  cJSON_InitHooks(&hooks);
}

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
      if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void initialize_wifi()
{
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  s_wifi_event_group = xEventGroupCreate();
  
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      &event_handler,
                                                      NULL,
                                                      &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                      IP_EVENT_STA_GOT_IP,
                                                      &event_handler,
                                                      NULL,
                                                      &instance_got_ip));

  wifi_config_t wifi_config = {
    .sta = {
      .ssid = CONFIG_ESP_WIFI_SSID,
      .password = CONFIG_ESP_WIFI_PASSWORD,
      .threshold.authmode = WIFI_AUTH_WPA2_PSK,
      
      .pmf_cfg = {
        .capable = true,
        .required = false
      },
    },
  };
  
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
  ESP_ERROR_CHECK(esp_wifi_start() );

  ESP_LOGI(TAG, "wifi_init_sta finished.");

  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE,
                                         pdFALSE,
                                         portMAX_DELAY);
  
  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(TAG, "connected to ap SSID");
  } else if (bits & WIFI_FAIL_BIT) {
    ESP_LOGI(TAG, "Failed to connect to SSID");
  } else {
    ESP_LOGE(TAG, "UNEXPECTED EVENT");
  }

  ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
  ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
  vEventGroupDelete(s_wifi_event_group);
}


int
measure_soil_humidity(adc1_channel_t channel, int num_samples)
{
  if (num_samples <= 0) {
    num_samples = 64;
  }
  int res = 0;
  int samples_run = num_samples;
  adc1_config_channel_atten(channel, ADC_ATTEN_DB_11);
  adc1_config_width(ADC_WIDTH_BIT_12);

  while (samples_run--) {
    int val = adc1_get_raw(channel);
    res += val;
  }
  return res / num_samples;
}


esp_err_t initialize_pump(gpio_num_t port)
{
  gpio_config_t config = {
    .pin_bit_mask = BIT64(port),
    .mode = GPIO_MODE_OUTPUT_OD,
    .pull_up_en = true,
    .pull_down_en = false,
    .intr_type = GPIO_PIN_INTR_DISABLE
  };
  ESP_ERROR_CHECK(gpio_config(&config));
  ESP_ERROR_CHECK(stop_pump(port));
  return ESP_OK;
}

esp_err_t start_pump(gpio_num_t port)
{
  return gpio_set_level(port, 0);
}


esp_err_t stop_pump(gpio_num_t port)
{
  return gpio_set_level(port, 1);
}

/* Our URI handler function to be called during GET /uri request */
esp_err_t get_handler(httpd_req_t *req)
{
  /* Send a simple response */
  cJSON *resp = cJSON_CreateObject();
  int soil_humidity = measure_soil_humidity(ADC_CHANNEL_SOIL_MOISTURE, 128);
  cJSON_AddNumberToObject(resp, "soil_humidity", soil_humidity);
  cJSON_AddBoolToObject(resp, "watering", soil_humidity > WATERING_THRESHOLD);
  char *out = cJSON_Print(resp);
  httpd_resp_send(req, out, HTTPD_RESP_USE_STRLEN);
  cJSON_free(resp);
  return ESP_OK;
}

/* URI handler structure for GET /uri */
httpd_uri_t uri_get = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = get_handler,
    .user_ctx = NULL
};

/* Function for starting the webserver */
httpd_handle_t start_webserver(void)
{
    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Empty handle to esp_http_server */
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_get);
    }
    /* If server failed to start, handle will be NULL */
    return server;
}

/* Function for stopping the webserver */
void stop_webserver(httpd_handle_t server)
{
    if (server) {
        /* Stop the httpd server */
        httpd_stop(server);
    }
}


void app_main(void)
{
  gpio_num_t port = GPIO_WATER_PUMP;
  initialize_pump(port);
  initialize_cJSON();
  initialize_wifi();
  start_webserver();
  while (1) {
    int humidity = measure_soil_humidity(ADC_CHANNEL_SOIL_MOISTURE, 128);
    ESP_LOGI(TAG, "Measured humidity: %d", humidity);
    if (humidity > WATERING_THRESHOLD) {
      ESP_LOGI(TAG, "Starting pump");
      start_pump(port);
    } else {
      ESP_LOGI(TAG, "Stopping pump");
      stop_pump(port);
    }
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

