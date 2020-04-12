#ifndef __BEE_UART_GPU__H__
#define __BEE_UART_GPU__H__

#include "driver/uart.h"


#define GPU_TX_PIN 17
#define GPU_RX_PIN 16
#define GPU_UART UART_NUM_1
#define GPU_UART_BUFFER_SIZE 2048
#define MAX_LINE_SIZE 255



extern void init_gpu_uart();

extern void gpu_lcd_task(void *pvParameters);

#endif /* __BEE_UART_GPU__H__ */
