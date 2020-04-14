
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ws2812_control.h"

#include "hap.h"

static const char *TAG = "homekitLed";


/*H-S-I*/
extern int led_on;
extern uint16_t led_hue;        //0~360
extern uint16_t led_saturation; //0~100
extern uint16_t led_brightness; //0~100
 
/*Homekit macros*/
#define ACCESSORY_NAME  "LEDSTRIP"
#define MANUFACTURER_NAME   "Rillhu"
#define MODEL_NAME  "ESP32_ACC"
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

void light_io_init(void) //ledc init
{
    ws2812_control_init();
}

static bool hsi2rgb(uint16_t h, uint16_t s, uint16_t i, uint8_t *r, uint8_t *g, uint8_t *b)
{
    bool res = true;
    uint16_t hi, F, P, Q, T;

    if (h > 360) return false;
    if (s > 100) return false;
    if (i > 100) return false;

    hi = (h / 60) % 6;
    F = 100 * h / 60 - 100 * hi;
    P = i * (100 - s) / 100;
    Q = i * (10000 - F * s) / 10000;
    T = i * (10000 - s * (100 - F)) / 10000;

    switch (hi) {
    case 0:
        *r = i;
        *g = T;
        *b = P;
        break;
    case 1:
        *r = Q;
        *g = i;
        *b = P;
        break;
    case 2:
        *r = P;
        *g = i;
        *b = T;
        break;
    case 3:
        *r = P;
        *g = Q;
        *b = i;
        break;
    case 4:
        *r = T;
        *g = P;
        *b = i;
        break;
    case 5:
        *r = i;
        *g = P;
        *b = Q;
        break;
    default:
        return false;
    }
    return res;
}

static void led_update()
{
    uint8_t r,g,b;
    uint16_t brightness = (led_on==true)?led_brightness:0;
    ESP_LOGI(TAG,"h:%d,s:%d,i:%d,i2:%d", led_hue,led_saturation,led_brightness,brightness);
    if(!hsi2rgb(led_hue, led_saturation, brightness, &r, &g, &b))
    {
        ESP_LOGW(TAG,"r:%d,g:%d,b:%d", r, g, b);
        return;
    }
    r = r*255/100;
    g = g*255/100;
    b = b*255/100;
    ESP_LOGI(TAG,"r:%d,g:%d,b:%d", r, g, b);

    struct led_state new_state;
    for (size_t i = 0; i < NUM_LEDS; i++)
    {
        new_state.leds[i] = g<<16|r<<8|b; //GRB
    }
    
    ws2812_write_leds(new_state);
}

/*--------------------------homekit-----------------------------*/
static void* acc_ins;  //accessory instance

/*LED ON 'service'*/
static void* _ev_handle;
int led_on = false;
void* led_on_read(void* arg)
{
    ESP_LOGI(TAG,"on read");
    return (void*)led_on;
}


void led_on_write(void* arg, void* value, int len)
{
    ESP_LOGI(TAG,"on write %d, bright: %d", (int)value, led_brightness);
    led_on = (int)value;
    if (value) {
        led_on = true;
    }
    else {
        led_on = false;
    }

    led_update();

    if (_ev_handle)
        hap_event_response(acc_ins, _ev_handle, (void*)led_on);

    return;
}


void led_on_notify(void* arg, void* ev_handle, bool enable)
{
    ESP_LOGI(TAG,"on noti: %d", enable);
    if (enable) {
        _ev_handle = ev_handle;
    }
    else {
        _ev_handle = NULL;
    }
}


static bool _identifed = false;
void* identify_read(void* arg)
{
    return (void*)true;
}


/*LED hue 'service'*/
static void* hue_ev_handle;
uint16_t led_hue;     // hue is scaled 0 to 360
void led_hue_write(void* arg, void* value, int len)
{
    led_hue = (int)(value);
    ESP_LOGI(TAG,"hue write %d",led_hue);
    //ESP_LOGI(TAG,"hue write2 %f",(float)(value));

    if(led_on==true){
        led_update();
    }

    uint16_t hue = led_hue * 100;
    if (hue_ev_handle)
        hap_event_response(acc_ins, hue_ev_handle, (void*)(long)hue);
}

void* led_hue_read(void* arg)
{
    ESP_LOGI(TAG,"hue read");
    return (void*)(long)led_hue;
}

void led_hue_notify(void* arg, void* ev_handle, bool enable)
{
    ESP_LOGI(TAG,"hue noti");
    if (enable) {
        hue_ev_handle = ev_handle;
    }
    else {
        hue_ev_handle = NULL;
    }
}

/*LED saturation 'service'*/
static void* saturation_ev_handle;
uint16_t led_saturation;     // saturation is scaled 0 to 100
void led_saturation_write(void* arg, void* value, int len)
{
    led_saturation = (int)(value);
    ESP_LOGI(TAG,"saturation write %d",led_saturation);
    //ESP_LOGI(TAG,"saturation write2 %f",(float)(value));

    if(led_on){
        led_update();
    }    

    uint16_t saturation = led_saturation * 100;
    if (saturation_ev_handle)
        hap_event_response(acc_ins, saturation_ev_handle, (void*)(long)saturation);
}

void* led_saturation_read(void* arg)
{
    ESP_LOGI(TAG,"saturation read");
    return (void*)(long)led_saturation;
}

void led_saturation_notify(void* arg, void* ev_handle, bool enable)
{
    ESP_LOGI(TAG,"saturation noti");
    if (enable) {
        saturation_ev_handle = ev_handle;
    }
    else {
        saturation_ev_handle = NULL;
    }
}

/*LED brightness 'service'*/
static void* brightness_ev_handle;
uint16_t led_brightness;     // brightness is scaled 0 to 100
void led_brightness_write(void* arg, void* value, int len)
{
    led_brightness = (int)(value);
    ESP_LOGI(TAG,"brightness write %d",led_brightness);

    if(led_on==true){
        led_update();
    }

    uint16_t brightness = led_brightness; //rm *100
    if (brightness_ev_handle)
        hap_event_response(acc_ins, brightness_ev_handle, (void*)(long)brightness);
}

void* led_brightness_read(void* arg)
{
    ESP_LOGI(TAG,"brightness read");
    return (void*)(long)led_brightness;
}

void led_brightness_notify(void* arg, void* ev_handle, bool enable)
{
    ESP_LOGI(TAG,"brightness noti");
    if (enable) {
        brightness_ev_handle = ev_handle;
    }
    else {
        brightness_ev_handle = NULL;
    }
}

#if 0
/*periodically report led's status to homekit*/
void led_status_report_task(void* arm)
{
    while(1){       
        if (led_on==true){
            if (hue_ev_handle){
                hap_event_response(acc_ins, hue_ev_handle, (void*)(long)led_hue);
            }

            if (saturation_ev_handle){
                hap_event_response(acc_ins, saturation_ev_handle, (void*)(long)led_saturation);
            }

            if (brightness_ev_handle){
                hap_event_response(acc_ins, brightness_ev_handle, (void*)(long)led_brightness);  
            }
        }
        vTaskDelay( 3000 / portTICK_RATE_MS );
    }
}
#endif

/*HAP object init*/
void hap_object_init(void* arg)
{
    void* accessory_object = hap_accessory_add(acc_ins);
    struct hap_characteristic cs[] = {
        {HAP_CHARACTER_IDENTIFY, (void*)true, NULL, identify_read, NULL, NULL},
        {HAP_CHARACTER_MANUFACTURER, (void*)MANUFACTURER_NAME, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_MODEL, (void*)MODEL_NAME, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_NAME, (void*)ACCESSORY_NAME, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_SERIAL_NUMBER, (void*)"202004053201", NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_FIRMWARE_REVISION, (void*)"1.0", NULL, NULL, NULL, NULL},
    };
    hap_service_and_characteristics_add(acc_ins, accessory_object, HAP_SERVICE_ACCESSORY_INFORMATION, cs, ARRAY_SIZE(cs));

    struct hap_characteristic cd[] = {
        {HAP_CHARACTER_ON, (void*)led_on, NULL, led_on_read, led_on_write, led_on_notify},
        {HAP_CHARACTER_HUE, (void*)(long)led_hue, NULL, led_hue_read, led_hue_write, led_hue_notify},
        {HAP_CHARACTER_SATURATION, (void*)(long)led_saturation, NULL, led_saturation_read, led_saturation_write, led_saturation_notify},
        {HAP_CHARACTER_BRIGHTNESS, (void*)(long)led_brightness, NULL, led_brightness_read, led_brightness_write, led_brightness_notify},
    };
    hap_service_and_characteristics_add(acc_ins, accessory_object, HAP_SERVICE_LIGHTBULB, cd, ARRAY_SIZE(cd));
}


/*Interface for app_main*/
void hap_register_device_handler(char *acc_id)
{
    hap_init();

    hap_accessory_callback_t callback;
    callback.hap_object_init = hap_object_init;
    acc_ins = hap_accessory_register((char*)ACCESSORY_NAME, acc_id, (char*)"111-11-132", (char*)MANUFACTURER_NAME, HAP_ACCESSORY_CATEGORY_LIGHTBULB, 811, 1, NULL, &callback);
}


