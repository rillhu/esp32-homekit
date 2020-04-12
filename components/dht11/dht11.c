#include <string.h>

#include "rom/ets_sys.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "dht11.h"
#include "driver/gpio.h"

//#define DHT_DEBUG_PRINT		true
#ifdef DHT_DEBUG_PRINT
#undef DHT_DEBUG_PRINT
#define DHT_DEBUG_PRINT(...) printf(__VA_ARGS__);
#else
#define DHT_DEBUG_PRINT(...)
#endif

#define sleepms(x) ets_delay_us(x*1000);

#define TAG "DHT"

DHT_Sensor_Data dht_data;
char dht_temp_c[4];// extern global varaible to store the temperature
char dht_humi_c[4];// extern global varaible to store the humidity
bool dht_ok = false;

static inline float scale_humidity(DHTType sensor_type, int *data)
{
	if(sensor_type == DHT11) {
		return (float) data[0];
	} else {
		float humidity = data[0] * 256 + data[1];
		return humidity /= 10;
	}
}

static inline float scale_temperature(DHTType sensor_type, int *data)
{
	if(sensor_type == DHT11) {
		return (float) data[2];
	} else {
		float temperature = data[2] & 0x7f;
		temperature *= 256;
		temperature += data[3];
		temperature /= 10;
		if (data[2] & 0x80)
			temperature *= -1;
		return temperature;
	}
}

char* DHTFloat2String(char* buffer, float value)
{
  sprintf(buffer, "%d.%d", (int)(value),(int)((value - (int)value)*100));
  return buffer;
}

bool DHTRead(DHT_Sensor *sensor, DHT_Sensor_Data* output)
{
	int counter = 0;
	int laststate = 1;
	int i = 0;
	int j = 0;
	int checksum = 0;
	int data[100];
	data[0] = data[1] = data[2] = data[3] = data[4] = 0;
	uint8_t pin = sensor->pin;

	// Wake up device, 250ms of high
	gpio_set_direction(pin, GPIO_MODE_OUTPUT);
	gpio_set_level(pin, 1);
	sleepms(250);
	// Hold low for 20ms
	gpio_set_level(pin, 0);
	//sleepms(20);
	sleepms(22);
	// High for 40us
	gpio_set_level(pin, 1);
	//ets_delay_us(40);
	ets_delay_us(43);
	// Set DHT_PIN pin as an input
	gpio_set_direction(pin, GPIO_MODE_INPUT);

	// wait for pin to drop
	while (gpio_get_level(pin) == 1 && i < DHT_MAXCOUNT) {
		ets_delay_us(1);
		i++;
	}

	if(i == DHT_MAXCOUNT)
	{
		DHT_DEBUG_PRINT("DHT: Failed to get reading from GPIO%d, dying\r\n", pin);
	    return false;
	}

	// read data
	for (i = 0; i < DHT_MAXTIMINGS; i++)
	{
		// Count high time (in approx us)
		counter = 0;
		while (gpio_get_level(pin) == laststate)
		{
			counter++;
			ets_delay_us(1);
			if (counter == 1000)
				break;
		}
		laststate = gpio_get_level(pin);
		if (counter == 1000)
			break;
		// store data after 3 reads
		if ((i>3) && (i%2 == 0)) {
			// shove each bit into the storage bytes
			data[j/8] <<= 1;
			if (counter > DHT_BREAKTIME)
				data[j/8] |= 1;
			j++;
		}
	}

	if (j >= 39) {
		checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;
	    DHT_DEBUG_PRINT("DHT%s: %02x %02x %02x %02x [%02x] CS: %02x (GPIO%d)\r\n",
	              sensor->type==DHT11?"11":"22",
	              data[0], data[1], data[2], data[3], data[4], checksum, pin);
		if (data[4] == checksum) {
			// checksum is valid
			output->temperature = scale_temperature(sensor->type, data);
			output->humidity = scale_humidity(sensor->type, data);
			DHT_DEBUG_PRINT("DHT: Temperature*100 =  %d *C, Humidity*100 = %d %% (GPIO%d)\n",
		          (int) (output->temperature * 100), (int) (output->humidity * 100), pin);
		} else {
			DHT_DEBUG_PRINT("DHT: Checksum was incorrect after %d bits. Expected %d but got %d (GPIO%d)\r\n",
		                j, data[4], checksum, pin);
		    return false;
		}
	} else {
	    DHT_DEBUG_PRINT("DHT: Got too few bits: %d should be at least 40 (GPIO%d)\r\n", j, pin);
	    return false;
	}
	return true;
}

bool DHTInit(DHT_Sensor *sensor)
{
    gpio_config_t conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = 1<<sensor->pin
    };

    if(gpio_config(&conf)==ESP_OK){
		DHT_DEBUG_PRINT("DHT: Setup for type %s connected to GPIO%d\n", sensor->type==DHT11?"DHT11":"DHT22", sensor->pin);
		return true;
	} else {
		DHT_DEBUG_PRINT("DHT: Error in function set_gpio_mode for type %s connected to GPIO%d\n", sensor->type==DHT11?"DHT11":"DHT22", sensor->pin);
		return false;
	}
}

/*Task to handle the dht11 parameters setting and read*/
void bee_dht11_task(void *pv)
{    
    DHT_Sensor sensor;
    sensor.pin = GPIO_NUM_22;
    sensor.type = DHT11;

    DHTInit(&sensor);
	sleepms(1000);

    while(1){
        if(DHTRead(&sensor, &dht_data)){
            char temp[4],humi[4];
            sprintf(temp, "%d", (int)(dht_data.temperature));
            sprintf(humi, "%d", (int)(dht_data.humidity));
            if(strcmp(temp,dht_temp_c)||strcmp(humi,dht_humi_c)){  //potential bug          
                ESP_LOGI(TAG,"Temp: %s*C, Humi: %s%%",temp,humi);            
                sprintf(dht_temp_c, "%d", (int)(dht_data.temperature));
                sprintf(dht_humi_c, "%d", (int)(dht_data.humidity));
                dht_ok = true;            
            }
        } else{
            ESP_LOGW(TAG,"Read sensor failed, retrying");
        }
        
        vTaskDelay(10000 / portTICK_RATE_MS);
   }
}

