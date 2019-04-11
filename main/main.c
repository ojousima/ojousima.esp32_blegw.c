/*
Copyright (c) 2017-2019 Tony Pottier

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

@file main.c
@author Tony Pottier
@brief Entry point for the ESP32 application.
@see https://idyl.io
@see https://github.com/tonyp7/esp32-wifi-manager
*/

/*
 * Changes to file:
 * Otso Jousimaa <otso@ojousima.net>
 */


#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "mdns.h"
#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/netdb.h"

#include "ble.h"
#include "dns_server.h"
#include "http_server.h"
#include "mqtt.h"
#include "wifi_manager.h"

#include "gpio.h"
#include "time.h"

#include "esp_task_wdt.h"

/** If no new data is sent on 60 seconds, restart */
#define TWDT_TIMEOUT_S 60

#define CHECK_ERROR_CODE(returned, expected) ({                        \
            if(returned != expected){                                  \
                ESP_LOGE(TAG, "MAIN ERROR\n");                         \
                abort();                                               \
            }                                                          \
})



static TaskHandle_t task_dns_server = NULL;
static TaskHandle_t task_gpio_blink_led_task = NULL;
static TaskHandle_t task_gpio_timer_task = NULL;
static TaskHandle_t task_gpio_task = NULL;
static TaskHandle_t task_http_server = NULL;
static TaskHandle_t task_monitoring_task = NULL;
static TaskHandle_t task_time_task = NULL;
static TaskHandle_t task_wifi_manager = NULL;
static bool tx_data = false;

/* @brief tag used for ESP serial console messages */
static const char TAG[] = "main";

/** @brief Publish known beacons to hard-coded MQTT server.
 * Publish RSSI of iBeacons and Eddystone beacons along with MAC address.
 *
 */
static void ble_on_broadcaster_discovered(mac_addr_t mac,
    uint8_t* adv_data, size_t adv_data_len, int rssi, broadcaster_ops_t* ops)
{
  char topic[100] = {0};
  char data[100] = {0};
  static uint8_t wifi_mac[18] = { 0 };

  if(!wifi_mac[0])
  {
    esp_wifi_get_mac(ESP_IF_WIFI_STA, wifi_mac);
    memcpy(wifi_mac, mactoa(wifi_mac), sizeof(wifi_mac));
  }

  time_t now;
  time(&now);
  size_t index = 0;
  snprintf(topic, sizeof(topic), "%s/%s/%s/RSSI", wifi_mac, ops->name, mactoa(mac));
  index = snprintf(data, sizeof(data), "%ld:%d", now, rssi);
  int status = 0;
  status |= mqtt_publish(topic, (uint8_t*)data, index, 0, true);  //QoS = 0, retain = true

  /* If device is RuuviTag */
  if(!strcmp(ops->name, "RuuviTag"))
  {
    memset(topic, 0, sizeof(topic));
    snprintf(topic, sizeof(topic), "%s/%s/%s/RAW", wifi_mac, ops->name, mactoa(mac));
    /** Print raw payload **/
    index = 0;
    index += snprintf(data, sizeof(data), "%ld:", now);

    for(uint8_t i = 7; i < adv_data_len; i++)
    {
      index += snprintf(data + index, sizeof(data) - index, "%02X", adv_data[i]);
    }

    status |= mqtt_publish(topic, (uint8_t*)data, index, 0, true);  //QoS = 0, retain = true
  }

  ESP_LOGD(TAG, "Device detected, status %d: %s:%s", status, topic, data);
  ESP_LOG_BUFFER_HEX_LEVEL(TAG, adv_data, adv_data_len, ESP_LOG_DEBUG);

  // If MQTT was connected and message was sent, reset WDT
  if(0 == status)
  {
    tx_data = true;
  }
}

static void cleanup(void)
{
  //ble_disconnect_all();
  ble_scan_stop();
  //ota_unsubscribe();
}

/* Wi-Fi callback functions */
static void wifi_on_connected(void)
{
  ESP_LOGI(TAG, "Connected to WiFi, connecting to MQTT");
  mqtt_connect("playground.ruuvi.com", 1883,
               NULL, NULL,
               NULL);
  time_sync();

  if(NULL != task_gpio_blink_led_task)
  {
    vTaskSuspend(task_gpio_blink_led_task);
  }
}

static void wifi_on_disconnected(void)
{
  ESP_LOGI(TAG, "Disconnected from WiFi, stopping MQTT");
  mqtt_disconnect();
  /* We don't get notified when manually stopping MQTT */
  cleanup();

  if(NULL != task_gpio_blink_led_task)
  {
    vTaskResume(task_gpio_blink_led_task);
  }
}

/* MQTT callback functions */
static void mqtt_on_connected(void)
{
  ESP_LOGI(TAG, "Connected to MQTT, scanning for BLE devices");
  ble_scan_start();
  //Subscribe this task to TWDT, then check if it is subscribed
  CHECK_ERROR_CODE(esp_task_wdt_add(task_monitoring_task), ESP_OK);
  CHECK_ERROR_CODE(esp_task_wdt_status(task_monitoring_task), ESP_OK);
}

static void mqtt_on_disconnected(void)
{
  ESP_LOGI(TAG, "Disconnected from MQTT, stopping BLE");
  cleanup();
}

/**
 * @brief RTOS task that periodically prints the heap memory available.
 * @note Pure debug information, should not be ever started on production code! This is an example on how you can integrate your code with wifi-manager
 */
static void monitoring_task(void* pvParameter)
{
  for(;;)
  {
    ESP_LOGI(TAG, "free heap: %d", esp_get_free_heap_size());
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    if(tx_data)
    {
      CHECK_ERROR_CODE(esp_task_wdt_reset(), ESP_OK);
      tx_data = false;
    }
  }
}


void app_main()
{
  /* disable the default wifi logging */
  esp_log_level_set("wifi", ESP_LOG_NONE);
  esp_log_level_set(TAG, ESP_LOG_INFO);
  ESP_LOGI(TAG, "App started");
  gpio_init();
  xTaskCreate(&gpio_task, "gpio_task", 3072, NULL, 3, &task_gpio_task);
  xTaskCreate(&gpio_timer_task, "gpio_timer_task", 3072, NULL, 3, &task_gpio_timer_task);
  xTaskCreate(&gpio_blink_led_task, "gpio_blink_led_task", 3072, NULL, 3,
              &task_gpio_blink_led_task);
  /* initialize flash memory */
  nvs_flash_init();
  /* Task to fetch time once per 24 hours. Waits until wifi_on_connected triggers data fetch */
  xTaskCreate(&time_task, "time_task", 2048, NULL, 1, &task_time_task);
  /* start the HTTP Server task */
  xTaskCreate(&http_server, "http_server", 2048, NULL, 5, &task_http_server);
  /* start the DNS Server task */
  xTaskCreate(&dns_server, "dns_server", 3072, NULL, 5, &task_dns_server);
  /* start the wifi manager task */
  wifi_manager_set_on_connected(wifi_on_connected);
  wifi_manager_set_on_disconnected(wifi_on_disconnected);
  xTaskCreate(&wifi_manager, "wifi_manager", 4096, NULL, 4, &task_wifi_manager);
  /* your code should go here. Here we simply create a task on core 2 that monitors free heap memory */
  xTaskCreatePinnedToCore(&monitoring_task, "monitoring_task", 2048, NULL, 1,
                          &task_monitoring_task, 1);
  /* Init MQTT */
  ESP_ERROR_CHECK(mqtt_initialize());
  mqtt_set_on_connected_cb(mqtt_on_connected);
  mqtt_set_on_disconnected_cb(mqtt_on_disconnected);
  /* Init BLE */
  ESP_ERROR_CHECK(ble_initialize());
  ble_set_on_broadcaster_discovered_cb(ble_on_broadcaster_discovered);
  /* Init Watchdog */
  ESP_LOGI(TAG, "Initialize TWDT");
  //Initialize or reinitialize TWDT
  CHECK_ERROR_CODE(esp_task_wdt_init(TWDT_TIMEOUT_S, true), ESP_OK);
}
