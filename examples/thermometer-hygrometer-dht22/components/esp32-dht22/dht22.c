#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_system.h>

#define DHT22_HOST_PULL_LOW_TIME        3000
#define DHT22_HOST_PULL_UP_TIME         25
#define DHT22_SLAVE_PULL_LOW_TIME       85
#define DHT22_SLAVE_PULL_UP_TIME        85
#define DHT22_START_TRANSMISSION_TIME   56
#define DHT22_HIGH_BIT_TIME             75
#define DHT22_DATA_LEN                  5

#define TAG     "DHT22"

static portMUX_TYPE mutex = portMUX_INITIALIZER_UNLOCKED;

static int _slave_signal_get(int gpio, int timeout_us, int level)
{
    int usec = 0;

    while (gpio_get_level(gpio) == level) {
        if (usec > timeout_us)
            return -1;

        usec++;
        ets_delay_us(1);
    }

    return usec;
}

static bool prepare(int gpio)
{
    gpio_set_direction(gpio, GPIO_MODE_OUTPUT);

    gpio_set_level(gpio, 0);
    ets_delay_us(DHT22_HOST_PULL_LOW_TIME);

    gpio_set_level(gpio, 1);
    ets_delay_us(DHT22_HOST_PULL_UP_TIME);

    gpio_set_direction(gpio, GPIO_MODE_INPUT);

    if (_slave_signal_get(gpio, DHT22_SLAVE_PULL_LOW_TIME, 0) < 0) {
        return false;
    }

    if (_slave_signal_get(gpio, DHT22_SLAVE_PULL_UP_TIME, 1) < 0) {
        return false;
    }

    return true;
}

static int _read_bit(int gpio)
{
    int usec = _slave_signal_get(gpio, DHT22_START_TRANSMISSION_TIME, 0);
    if (usec < 0) {
        return -1;
    }

    usec = _slave_signal_get(gpio, DHT22_HIGH_BIT_TIME, 1);
    if (usec > 40) {
        return 1;
    }
    return 0;
}

static bool checksum_verify(uint8_t* data) 
{
    return (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xff));
}

int dht22_read(int gpio, float* temperature, float* humidity)
{
    if (!temperature || !humidity) {
        ESP_LOGE(TAG, "temperature or humidity is NULL");
        return -1;
    }

    uint8_t data[DHT22_DATA_LEN] = {0,};
    int bit;

    taskENTER_CRITICAL(&mutex);

    if (prepare(gpio) == false) {
        taskEXIT_CRITICAL(&mutex);
        ESP_LOGE(TAG, "prepare failed");
        return -1;
    }

    for (int i=0; i<DHT22_DATA_LEN; i++) {
        for (int j=7; j>=0; j--) {
            bit = _read_bit(gpio);
            if (bit < 0) {
                ESP_LOGE(TAG, "_read_bit failed");
                goto exit_loop;
            }

            data[i] |= (bit << j);
        }
    }

exit_loop:
    taskEXIT_CRITICAL(&mutex);

    if (checksum_verify(data) == false) {
        ESP_LOGE(TAG, "checksum_verify failed");
        return -1;
    }

    *temperature = data[2] & 0x7f;
    *temperature *= 0x100;
    *temperature += data[3];
    *temperature /= 10;

    if (data[2] & 0x80)
        *temperature *= -1;

    *humidity = data[0];
    *humidity *= 0x100;
    *humidity += data[1];
    *humidity /= 10;

    return 0;
}
