#ifndef _DS18B20_H_
#define _DS18B20_H_

extern float dsb20_temp;
void DS_init(int GPIO);
float Read_Temperature();
void bee_ds18b20_task(void *pv);

#endif
