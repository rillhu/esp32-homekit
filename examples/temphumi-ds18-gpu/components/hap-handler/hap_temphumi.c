#include <string.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs_flash.h"


#include "hap.h"
#include "ds18b20.h"


static const char *TAG = "homekit DS";


/*Homekit macros*/
#define ACCESSORY_NAME  "TEMP"
#define MANUFACTURER_NAME   "YOUNGHYUN"
#define MODEL_NAME  "ESP32_ACC"
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))


/* DS18b20*/
//static gpio_num_t DHT22_GPIO = GPIO_NUM_22;

static void* acc_ins;
static SemaphoreHandle_t ev_mutex;
static int temperature = 0;
static int humidity = 0;
static void* _temperature_ev_handle = NULL;
static void* _humidity_ev_handle =  NULL;

void temperature_humidity_monitoring_task(void* arm)
{
    const int DS_PIN = GPIO_NUM_23; //GPIO where you connected ds18b20
    DS_init(DS_PIN);

    while(1){
        float temp = Read_Temperature();

        if(temp!=150.0 && temp!=85.0){
            temperature = temp * 100;
            dsb20_temp = temp;
            ESP_LOGI(TAG,"T: %d *C",temperature);

        } else{
            ESP_LOGW(TAG,"Read sensor failed, retrying");
        }

        //xSemaphoreTake(ev_mutex, 0);

#if 1
        //if (_humidity_ev_handle)
        //    hap_event_response(acc_ins, _humidity_ev_handle, (void*)humidity);

        if (_temperature_ev_handle)
            hap_event_response(acc_ins, _temperature_ev_handle, (void*)temperature);

#endif
        //xSemaphoreGive(ev_mutex);

        //printf("%d %d\n", temperature, humidity);
        vTaskDelay( 3000 / portTICK_RATE_MS );
    }
}


static void* _temperature_read(void* arg)
{
    ESP_LOGI("MAIN", "_temperature_read");
    return (void*)temperature;
}


void _temperature_notify(void* arg, void* ev_handle, bool enable)
{
    ESP_LOGI("MAIN", "_temperature_notify");
    //xSemaphoreTake(ev_mutex, 0);

    if (enable) 
        _temperature_ev_handle = ev_handle;
    else 
        _temperature_ev_handle = NULL;

    //xSemaphoreGive(ev_mutex);
}


static void* _humidity_read(void* arg)
{
    ESP_LOGI("MAIN", "_humidity_read");
    return (void*)humidity;
}


void _humidity_notify(void* arg, void* ev_handle, bool enable)
{
    ESP_LOGI("MAIN", "_humidity_notify");
    //xSemaphoreTake(ev_mutex, 0);

    if (enable) 
        _humidity_ev_handle = ev_handle;
    else 
        _humidity_ev_handle = NULL;

    //xSemaphoreGive(ev_mutex);
}


static bool _identifed = false;
void* identify_read(void* arg)
{
    return (void*)true;
}


void hap_object_init(void* arg)
{
    void* accessory_object = hap_accessory_add(acc_ins);
    struct hap_characteristic cs[] = {
        {HAP_CHARACTER_IDENTIFY, (void*)true, NULL, identify_read, NULL, NULL},
        {HAP_CHARACTER_MANUFACTURER, (void*)MANUFACTURER_NAME, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_MODEL, (void*)MODEL_NAME, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_NAME, (void*)ACCESSORY_NAME, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_SERIAL_NUMBER, (void*)"0123456789", NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_FIRMWARE_REVISION, (void*)"1.0", NULL, NULL, NULL, NULL},
    };
    hap_service_and_characteristics_add(acc_ins, accessory_object, HAP_SERVICE_ACCESSORY_INFORMATION, cs, ARRAY_SIZE(cs));

/*
    struct hap_characteristic humidity_sensor[] = {
        {HAP_CHARACTER_CURRENT_RELATIVE_HUMIDITY, (void*)humidity, NULL, _humidity_read, NULL, _humidity_notify},
        {HAP_CHARACTER_NAME, (void*)"HYGROMETER" , NULL, NULL, NULL, NULL},
    };
    hap_service_and_characteristics_add(acc_ins, accessory_object, HAP_SERVICE_HUMIDITY_SENSOR, humidity_sensor, ARRAY_SIZE(humidity_sensor));
*/
    struct hap_characteristic temperature_sensor[] = {
        {HAP_CHARACTER_CURRENT_TEMPERATURE, (void*)temperature, NULL, _temperature_read, NULL, _temperature_notify},
        {HAP_CHARACTER_NAME, (void*)"THERMOMETER" , NULL, NULL, NULL, NULL},
    };
    hap_service_and_characteristics_add(acc_ins, accessory_object, HAP_SERVICE_TEMPERATURE_SENSOR, temperature_sensor, ARRAY_SIZE(temperature_sensor));

}


void hap_register_device_handler(char *acc_id)
{
    hap_init();

    hap_accessory_callback_t callback;
    callback.hap_object_init = hap_object_init;
    acc_ins = hap_accessory_register((char*)ACCESSORY_NAME, acc_id, (char*)"111-11-121", (char*)MANUFACTURER_NAME, HAP_ACCESSORY_CATEGORY_OTHER, 811, 1, NULL, &callback);

}


