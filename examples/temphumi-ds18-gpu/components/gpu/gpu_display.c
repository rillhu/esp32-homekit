#include "string.h"
#include "time.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_system.h"

#include "esp_log.h"

#include "uart_gpu.h"
#include "ds18b20.h"
#include "sntp.h"

#define TAG "gpudisplay"

void gpu_send( const char *format,...)
{
    char buf[64] = {0};
    va_list args;
    va_start(args,format);
    vsprintf(buf,format,args);
    va_end(args);
    uart_write_bytes(GPU_UART,buf, strlen(buf));
    vTaskDelay(100 / portTICK_RATE_MS);
    //ESP_LOGI(TAG,"%s",buf);
}

uint32_t time_update_cnt;
void update_gpu_time()
{
    if(time_update_cnt==0){
        time_t now;
        struct tm timeinfo;
        // update 'now' variable with current time
        time(&now);
        // Set timezone to China Standard Time
        setenv("TZ", "CST-8CDT-8,M4.2.0/2,M9.2.0/3", 1);
        tzset();
        localtime_r(&now, &timeinfo);
        gpu_send("STIM(%d,%d,%d);\r\n",timeinfo.tm_hour,timeinfo.tm_min, timeinfo.tm_sec);
    }
    if(time_update_cnt++>600)
        time_update_cnt=0;    
}

uint32_t dht_update_cnt;
float get_temp_humi()
{
    float t = 0.0;
    if(dht_update_cnt == 0){

        float temp = Read_Temperature();
        if(temp!=150.0 && temp!=85.0){
            t = temp;
        }
    }
    if(dht_update_cnt++>60)
        dht_update_cnt=0;   
    return t;
}

/*Task for hanle ---*/
void gpu_display_task(void *pv)
{
    init_gpu_uart();
    gpu_send("CLS(0);\r\n");
    gpu_send("W8DF(6,3,'112333112445662445');\r\n");
    gpu_send("W8MU(0,0,45,75,5,2);\r\n");
    gpu_send("W8UE(1);\r\n");
    gpu_send("DS16(2,2,'Bedroom',4);\r\n");
    gpu_send("DS16(60,60,'*C',4);\r\n");
    gpu_send("SXY(0,0);\r\n");
    gpu_send("W8UE(2);\r\n");
    gpu_send("DS32(14,2,'H',15);\r\n");
    gpu_send("DS32(14,34,'O',15);\r\n");
    gpu_send("DS32(14,66,'M',15);\r\n");
    gpu_send("DS32(14,98,'E',15);\r\n");
    gpu_send( "DS32(14,130,'K',15);\r\n");
    gpu_send("DS32(18,162,'I',15);\r\n");
    gpu_send("DS32(14,194,'T',15);\r\n");
    gpu_send("SXY(0,0);;\r\n");
    gpu_send("W8UE(3);\r\n");
    gpu_send("SHTM(32,25,20,15,56);\r\n");
    gpu_send("SXY(0,0);;\r\n");
    gpu_send("W8UE(1);\r\n");
    
    while (1) {
        gpu_send("DS32(20,25,'%.2f',15);\r\n",dsb20_temp); //update temperature 
        update_gpu_time();
    }
}

