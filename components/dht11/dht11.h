#ifndef __DHT22_H__
#define __DHT22_H__

typedef enum {
	DHT11,
	DHT22
} DHTType;

typedef struct {
  float temperature;
  float humidity;
} DHT_Sensor_Data;

typedef struct {
  uint8_t pin;
  DHTType type;
} DHT_Sensor;

#define DHT_MAXTIMINGS	10000
#define DHT_BREAKTIME	20
#define DHT_MAXCOUNT	40
//#define DHT_DEBUG		true

extern DHT_Sensor_Data dht_data;

extern char dht_temp_c[4];
extern char dht_humi_c[4];
extern bool dht_ok;

#define sleepms(x) ets_delay_us(x*1000);

bool DHTInit(DHT_Sensor *sensor);
bool DHTRead(DHT_Sensor *sensor, DHT_Sensor_Data* output);
char* DHTFloat2String(char* buffer, float value);
void bee_dht11_task(void *pv);

#endif
