#include <string.h>
#include <stdlib.h>
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
//#include "freertos/heap_regions.h"

#include "esp_log.h"
#include "esp_system.h"
#include "uart_gpu.h"

#define TAG "GPU"

void init_gpu_uart()
{
	ESP_LOGI(TAG, "init UART");

	uart_config_t myUartConfig;
	myUartConfig.baud_rate           = 115200;
	myUartConfig.data_bits           = UART_DATA_8_BITS;
	myUartConfig.parity              = UART_PARITY_DISABLE;
	myUartConfig.stop_bits           = UART_STOP_BITS_1;
	myUartConfig.flow_ctrl           = UART_HW_FLOWCTRL_DISABLE;
	myUartConfig.rx_flow_ctrl_thresh = 122;

	uart_param_config(GPU_UART, &myUartConfig);

	uart_set_pin(GPU_UART,
			GPU_TX_PIN,         // TX
			GPU_RX_PIN,         // RX
			UART_PIN_NO_CHANGE, // RTS
			UART_PIN_NO_CHANGE  // CTS
  );

    uart_driver_install(GPU_UART, GPU_UART_BUFFER_SIZE, GPU_UART_BUFFER_SIZE, 0, NULL, 0);
}

void deinit_gpu_uart()
{
	ESP_LOGI(TAG, "deinit UART");
	uart_driver_delete(GPU_UART);
}

#if 0
/*Task for handle the GPS*/
void gpu_lcd_task(void *pvParameters)
{
	uint8_t line[MAX_LINE_SIZE+1];
	initUART(GPU_UART);
    bool cls_ind = false;
    int cnt = 0;
	while(1) {
		//readLine(GPU_UART,line);
        if(cls_ind==false){
            char *str = "CLS(0);\r\n";
            uart_write_bytes(GPU_UART,str, strlen(str));
            cls_ind = true;
            vTaskDelay(10 / portTICK_RATE_MS);
        }

        char str2[1024];//
        memset(str2,'\0',1024);

        sprintf(str2, "DS64(25,134,'Friday',5);\r\n");
        uart_write_bytes(GPU_UART,str2, strlen(str2));
        printf("%s",str2);
        vTaskDelay(10 / portTICK_RATE_MS);

        memset(str2,'\0',1024);
        sprintf(str2, "DS64(25,200,'2017-05-26',5);\r\n");
        uart_write_bytes(GPU_UART,str2, strlen(str2));
        printf("%s",str2);
        vTaskDelay(10 / portTICK_RATE_MS);

        memset(str2,'\0',1024);
        sprintf(str2, "DS64(25,300,'11:01:%d',5);\r\n",cnt++);
        cnt = cnt>59?0:cnt;
        uart_write_bytes(GPU_UART,str2, strlen(str2));
        printf("%s",str2);
        vTaskDelay(10 / portTICK_RATE_MS);

        vTaskDelay(1000 / portTICK_RATE_MS);
	}


	vTaskDelete(NULL);
}
#endif
