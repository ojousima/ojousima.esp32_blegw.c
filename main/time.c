/**
 * @addtogroup Time
 *
 */
/*@{*/
/**
 * @file time.h
 * @author Rizwan Hamid Randhawa
 * @date 2019-03-14
 * @copyright Ruuvi Innovations Ltd, license BSD-3-Clause.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "lwip/err.h"
#include "apps/sntp/sntp.h"

#define TIME_SYNC_BIT (1<<0)


EventGroupHandle_t time_event_group = NULL;
EventBits_t uxBits;
static const char TAG[] = "time";

static void wait_for_sntp(void)
{
    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGD(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
}

/**
 * @bried 
 */
static void sync_sntp(void)
{
    ESP_LOGI(TAG, "Synchronizing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_setservername(1, "europe.pool.ntp.org");
    sntp_setservername(2, "uk.pool.ntp.org ");
    sntp_setservername(3, "us.pool.ntp.org");
    sntp_setservername(4, "time1.google.com");
    sntp_init();
    wait_for_sntp();
    sntp_stop();
    ESP_LOGI(TAG, "Synchronizing SNTP");
}

void time_sync(void)
{
  xEventGroupSetBits(time_event_group, TIME_SYNC_BIT);
}

void time_task(void *param)
{
    if(!time_event_group) time_event_group = xEventGroupCreate();

    for (;;) {
         /** 
          EventBits_t xEventGroupWaitBits(
                       const EventGroupHandle_t xEventGroup,
                       const EventBits_t uxBitsToWaitFor,
                       const BaseType_t xClearOnExit,
                       const BaseType_t xWaitForAllBits,
                       TickType_t xTicksToWait );
          */
        // Wait for trigger command or 24 hours to synch time. 
        xEventGroupWaitBits(time_event_group, TIME_SYNC_BIT, pdTRUE, pdFALSE, (24*60*60*1000)/portTICK_PERIOD_MS );
        sync_sntp();
    }

}

/*@}*/