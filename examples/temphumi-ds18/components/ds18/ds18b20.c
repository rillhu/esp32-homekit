#include "string.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "esp_log.h"

#define TAG "DSB20"

int DS_GPIO;
float dsb20_temp = 0.0;

/*CRC8 table*/
unsigned char crc_array[256] = {
    0x00, 0x5e, 0xbc, 0xe2, 0x61, 0x3f, 0xdd, 0x83,
    0xc2, 0x9c, 0x7e, 0x20, 0xa3, 0xfd, 0x1f, 0x41,
    0x9d, 0xc3, 0x21, 0x7f, 0xfc, 0xa2, 0x40, 0x1e,
    0x5f, 0x01, 0xe3, 0xbd, 0x3e, 0x60, 0x82, 0xdc,
    0x23, 0x7d, 0x9f, 0xc1, 0x42, 0x1c, 0xfe, 0xa0,
    0xe1, 0xbf, 0x5d, 0x03, 0x80, 0xde, 0x3c, 0x62,
    0xbe, 0xe0, 0x02, 0x5c, 0xdf, 0x81, 0x63, 0x3d,
    0x7c, 0x22, 0xc0, 0x9e, 0x1d, 0x43, 0xa1, 0xff,
    0x46, 0x18, 0xfa, 0xa4, 0x27, 0x79, 0x9b, 0xc5,
    0x84, 0xda, 0x38, 0x66, 0xe5, 0xbb, 0x59, 0x07,
    0xdb, 0x85, 0x67, 0x39, 0xba, 0xe4, 0x06, 0x58,
    0x19, 0x47, 0xa5, 0xfb, 0x78, 0x26, 0xc4, 0x9a,
    0x65, 0x3b, 0xd9, 0x87, 0x04, 0x5a, 0xb8, 0xe6,
    0xa7, 0xf9, 0x1b, 0x45, 0xc6, 0x98, 0x7a, 0x24,
    0xf8, 0xa6, 0x44, 0x1a, 0x99, 0xc7, 0x25, 0x7b,
    0x3a, 0x64, 0x86, 0xd8, 0x5b, 0x05, 0xe7, 0xb9,
    0x8c, 0xd2, 0x30, 0x6e, 0xed, 0xb3, 0x51, 0x0f,
    0x4e, 0x10, 0xf2, 0xac, 0x2f, 0x71, 0x93, 0xcd,
    0x11, 0x4f, 0xad, 0xf3, 0x70, 0x2e, 0xcc, 0x92,
    0xd3, 0x8d, 0x6f, 0x31, 0xb2, 0xec, 0x0e, 0x50,
    0xaf, 0xf1, 0x13, 0x4d, 0xce, 0x90, 0x72, 0x2c,
    0x6d, 0x33, 0xd1, 0x8f, 0x0c, 0x52, 0xb0, 0xee,
    0x32, 0x6c, 0x8e, 0xd0, 0x53, 0x0d, 0xef, 0xb1,
    0xf0, 0xae, 0x4c, 0x12, 0x91, 0xcf, 0x2d, 0x73,
    0xca, 0x94, 0x76, 0x28, 0xab, 0xf5, 0x17, 0x49,
    0x08, 0x56, 0xb4, 0xea, 0x69, 0x37, 0xd5, 0x8b,
    0x57, 0x09, 0xeb, 0xb5, 0x36, 0x68, 0x8a, 0xd4,
    0x95, 0xcb, 0x29, 0x77, 0xf4, 0xaa, 0x48, 0x16,
    0xe9, 0xb7, 0x55, 0x0b, 0x88, 0xd6, 0x34, 0x6a,
    0x2b, 0x75, 0x97, 0xc9, 0x4a, 0x14, 0xf6, 0xa8,
    0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7,
    0xb6, 0xe8, 0x0a, 0x54, 0xd7, 0x89, 0x6b, 0x35,
};

unsigned char CRC8_Table(unsigned char *p, char counter)
{
    unsigned char crc8 = 0;
    for( ; counter > 0; counter--){
        crc8 = crc_array[crc8^*p]; //lookup crc8 table to get crc code
        p++;
    }
    return crc8;
}

/*one wire bus reset*/
char ow_reset(void)
{
    char RstFlag = 0;

    gpio_set_direction(DS_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DS_GPIO, 0);
    ets_delay_us(500);
    gpio_set_level(DS_GPIO, 1);

    gpio_set_direction(DS_GPIO, GPIO_MODE_INPUT);
    ets_delay_us(30);

    if(gpio_get_level(DS_GPIO)==0){
        RstFlag=1; //1: reset successfully
    }else{
        RstFlag=0;
        return(RstFlag);
    }
    //ESP_LOGI(TAG,"rst flag:%d",RstFlag);
    ets_delay_us(470);
    if(gpio_get_level(DS_GPIO)==1){
       RstFlag=1;//1: reset successfully
    }else{
        RstFlag=0;
        return(RstFlag);
    }
    //ESP_LOGI(TAG,"rst flag 2:%d",RstFlag);
    return(RstFlag);
}

/*Read one byte from one-wire bus*/
char read_byte(void)
{
    unsigned char i;
    unsigned char temp;
    temp=0;
    for(i=0;i<8;i++){
        gpio_set_direction(DS_GPIO, GPIO_MODE_OUTPUT);
        gpio_set_level(DS_GPIO, 0);
        ets_delay_us(2);
        gpio_set_level(DS_GPIO, 1);
        ets_delay_us(15);

        gpio_set_direction(DS_GPIO, GPIO_MODE_INPUT);
        temp=temp>>1;
        if(gpio_get_level(DS_GPIO)==1)
        {
            temp=temp+0x80; //Read MSB bit first, the read LSB bit
        }
        ets_delay_us(15);
    }
    return(temp);
}

/*Write one byte from one-wire bus*/
void write_byte(char val)
{
    unsigned char i;
    for(i=0;i<8;i++){
        gpio_set_direction(DS_GPIO, GPIO_MODE_OUTPUT);
        gpio_set_level(DS_GPIO, 0);
        ets_delay_us(15);
        gpio_set_level(DS_GPIO, val&0x1); //LSB is first
        ets_delay_us(45);
        gpio_set_level(DS_GPIO, 1);
        val=val>>1;
        ets_delay_us(1);
    }
}

/*Read temperature*/
float Read_Temperature()
{
    char rst_flag = 0;
    char ds_temp[9];
    float  x = 150.0;
    union{char c[2]; int x;}temp;

    rst_flag = ow_reset();

    if(1==rst_flag){
        //ESP_LOGI(TAG,"reset OK!");
        write_byte(0xCC); //Skip ROM
        write_byte(0x44); //translate temperature
        if(1==ow_reset()){
            //ESP_LOGI(TAG,"reset 2 OK!");
            write_byte(0xCC); //Skip ROM
            write_byte(0xbe);  //read ds18b20
            temp.c[1]=read_byte(); //Read LSB 8 bits
            temp.c[0]=read_byte(); //Read MSB 8 bits
            ds_temp[2] = read_byte();
            ds_temp[3] = read_byte();
            ds_temp[4] = read_byte();
            ds_temp[5] = read_byte();
            ds_temp[6] = read_byte();
            ds_temp[7] = read_byte();
            ds_temp[8] = read_byte();
        }else{
            ESP_LOGE(TAG,"Rst fail!");
        }
    }
    else{
        ESP_LOGE(TAG,"Rst fail2!");
    }

    ds_temp[0] = temp.c[1];
    ds_temp[1] = temp.c[0];

    unsigned char crc_rst= CRC8_Table((unsigned char*)ds_temp,8);

    if(crc_rst == ds_temp[8]){ //if CRC is OK
        //3 TODO: Add code for temp is lower than 0
        x=((temp.c[0]&0x07)*256 + temp.c[1])*0.0625;
        //ESP_LOGI(TAG,"Temp reg 0x%x, 0x%x", temp.c[0], temp.c[1]);
        unsigned char i;
        for(i=0;i<7;i++){
            //ESP_LOGI(TAG,"Other reg: %d",ds_temp[i]);
        }
    }else {
        ESP_LOGE(TAG,"CRC NOK");
    }
    return x;
}

void DS_init(int GPIO){
    DS_GPIO = GPIO;
    gpio_config_t conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = 1<<DS_GPIO
    };

    if(gpio_config(&conf)==ESP_OK){
    	ESP_LOGI(TAG,"Setup for %s connected to GPIO%d", "DS18B20", DS_GPIO);
    	return true;
    } else {
    	ESP_LOGE(TAG,"Error in %s connected to GPIO%d\n", "DS18B20",DS_GPIO);
    	return false;
    }
    //gpio_pad_select_gpio(DS_GPIO);
}

/*Task for hanle ds18b20*/
void bee_ds18b20_task(void *pv)
{
    const int DS_PIN = GPIO_NUM_23; //GPIO where you connected ds18b20
    DS_init(DS_PIN);
    while (1) {
        float temp = Read_Temperature();
        ESP_LOGI(TAG,"B20 Temp: %f",temp);

        if(temp!=150.0 && temp!=85.0){
            dsb20_temp = temp;
            vTaskDelay(5000 / portTICK_PERIOD_MS);
        }else{
            vTaskDelay(100/portTICK_PERIOD_MS);
        }
    }
}

