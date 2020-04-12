
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "hap.h"

static const char *TAG = "homekit ledc";

extern int led_brightness;

/*Homekit macros*/
#define ACCESSORY_NAME  "LED"
#define MANUFACTURER_NAME   "Rillhu"
#define MODEL_NAME  "ESP32_ACC"
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

/*LEDC macros*/
#define LEDC_IO_0 (18)
#define LEDC_IO_1 (19)
#define PWM_TARGET_DUTY 8192

void light_io_init(void) //ledc init
{
    // enable ledc module
    periph_module_enable(PERIPH_LEDC_MODULE);

    // config the timer
    ledc_timer_config_t ledc_timer = {
        //set timer counter bit number
        .bit_num = LEDC_TIMER_13_BIT,
        //set frequency of pwm
        .freq_hz = 5000,
        //timer mode,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        //timer index
        .timer_num = LEDC_TIMER_0
    };
    ledc_timer_config(&ledc_timer);

    //config the channel
    ledc_channel_config_t ledc_channel = {
        //set LEDC channel 0
        .channel = LEDC_CHANNEL_0,
        //set the duty for initialization.(duty range is 0 ~ ((2**bit_num)-1)
        .duty = 100,
        //GPIO number
        .gpio_num = LEDC_IO_0,
        //GPIO INTR TYPE, as an example, we enable fade_end interrupt here.
        .intr_type = LEDC_INTR_FADE_END,
        //set LEDC mode, from ledc_mode_t
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        //set LEDC timer source, if different channel use one timer,
        //the frequency and bit_num of these channels should be the same
        .timer_sel = LEDC_TIMER_0
    };
    //set the configuration
    ledc_channel_config(&ledc_channel);

    //config ledc channel1
    ledc_channel.channel = LEDC_CHANNEL_1;
    ledc_channel.gpio_num = LEDC_IO_1;
    ledc_channel_config(&ledc_channel);
}

/*--------------------------------------------------------------*/
static void* acc_ins;

/*LED ON 'service'*/
static void* _ev_handle;
static int led_on = false;
void* led_on_read(void* arg)
{
    ESP_LOGI(TAG,"LED READ\n");
    return (void*)led_on;
}


void led_on_write(void* arg, void* value, int len)
{
   ESP_LOGI(TAG,"LED WRITE. %d, brigh: %d\n", (int)value, led_brightness);

    led_on = (int)value;
    if (value) {
        led_on = true;
        //LED channel 0
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, led_brightness*PWM_TARGET_DUTY/100);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
         //LED channel 1
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, led_brightness*PWM_TARGET_DUTY/100);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
    }
    else {
        led_on = false;
        //LED channel 0
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
        //LED channel 1
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, 0);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
    }

    if (_ev_handle)
        hap_event_response(acc_ins, _ev_handle, (void*)led_on);

    return;
}


void led_on_notify(void* arg, void* ev_handle, bool enable)
{
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

/*LED brightness 'service'*/
static void* brightness_ev_handle;
int led_brightness;     // brightness is scaled 0 to 100
void led_brightness_write(void* arg, void* value, int len)
{
    //ESP_LOGI(TAG,"brightness write\n");
    led_brightness = (int)(value);
    ESP_LOGI(TAG,"brightness write %d",led_brightness);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, led_brightness*PWM_TARGET_DUTY/100);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
    if (brightness_ev_handle)
    hap_event_response(acc_ins, brightness_ev_handle, (void*)led_brightness);

}

void* led_brightness_read(void* arg)
{
    ESP_LOGI(TAG,"led_brightness_read\n");
    return (void*)led_brightness;
}

void led_brightness_notify(void* arg, void* ev_handle, bool enable)
{
    ESP_LOGI(TAG,"led_brightness noti\n");
    if (enable) {
        brightness_ev_handle = ev_handle;
    }
    else {
        brightness_ev_handle = NULL;
    }
}

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
        {HAP_CHARACTER_BRIGHTNESS, (void*)led_brightness, NULL, led_brightness_read, led_brightness_write, led_brightness_notify},
    };
    hap_service_and_characteristics_add(acc_ins, accessory_object, HAP_SERVICE_LIGHTBULB, cd, ARRAY_SIZE(cd));
}


/*Interface for app_main*/
void hap_register_device_handler(char *acc_id)
{
    hap_init();

    hap_accessory_callback_t callback;
    callback.hap_object_init = hap_object_init;
    acc_ins = hap_accessory_register((char*)ACCESSORY_NAME, acc_id, (char*)"111-11-131", (char*)MANUFACTURER_NAME, HAP_ACCESSORY_CATEGORY_LIGHTBULB, 811, 1, NULL, &callback);
}


