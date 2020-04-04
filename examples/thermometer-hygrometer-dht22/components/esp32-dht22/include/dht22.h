#ifndef _DHT22_H_
#define _DHT22_H_

#ifdef __cplusplus
extern "C" {
#endif

int dht22_read(int gpio, float* temperature, float* humidity);

#ifdef __cplusplus
}
#endif

#endif //#ifndef _DHT22_H_

