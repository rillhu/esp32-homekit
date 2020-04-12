#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"

#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_attr.h"


#include "lwip/err.h"
#include "lwip/apps/sntp.h"

static const char *TAG = "sntp";

bool sntp_init_flg = false;

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

void obtain_time(void)
{
    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while(timeinfo.tm_year < (2019 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
}

void get_sntp_time_task(void *arg)
{
    initialize_sntp();
    time_t now;
    struct tm timeinfo;
    char strftime_buf[64];
    while(1){

        obtain_time();
        // update 'now' variable with current time
        time(&now);
        // Set timezone to China Standard Time
        setenv("TZ", "CST-8CDT-8,M4.2.0/2,M9.2.0/3", 1);
        tzset();
        localtime_r(&now, &timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        ESP_LOGI(TAG, "The current date/time in Xian is: %s", strftime_buf);
        
        //Update time every 1 hour
        for(int countdown = 60; countdown > 0; countdown--) {
            ESP_LOGI(TAG, "%d... ", countdown);
            vTaskDelay(60000 / portTICK_PERIOD_MS);
        }
    }
}