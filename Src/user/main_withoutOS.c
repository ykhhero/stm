#include "stm32f10x.h" 
#include "sys.h"
#include "usart.h"
#include "delay.h"
#include "utility.h"
#include "sim800c.h"
#include "string.h"
#include "debug.h"
#include "task.h"

#include "led.h"
#include "key.h"
#include "exti.h"
#include "beep.h"

#include "memory.h"
#include "base64.h"
 void init(void)
 {

 	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // 设 置 NVIC 中 断 分 组 2	
	SysTick_Configuration();
	
	uart_init(115200);
	uart2_init(115200);

 	LED_Init();		  		//初始化与LED连接的硬件接口
	BEEP_Init();         	//初始化蜂鸣器端口
	KEY_Init();         	//初始化与按键连接的硬件接口
	EXTIX_Init();		 	//外部中断初始化
	LED0=0;					//点亮LED0	 
	init_memory();
 }




extern int mqtt_init(void);
extern unsigned long global_timer_ticket;
void push_data(char* str, int len);
int pop_data(char* str, int len);
int main(void) 
{ 
	char  buf[USART_REC_LEN];


	init();

  	//base64_encode("1234567890abcdefg", memory_pg0);
	//base64_decode(memory_pg0, memory_pg1);
	
	fprintf(UART1_FILE, "uart1 works...\n");
	fprintf(UART2_FILE, "uart2 works...\n");
	delay_s(5);

	init_sim800c();
	while(1)
	{
		fprintf(UART1_FILE, "I am from uart1, please input data. \r\n"
			       	"1:connect to server\r\n"
				"2:send data to server\r\n"
				"3:read data\r\n"
				"4:close connection\r\n"
				"5:set debug info\r\n"
				"6:init sim\r\n"
				"7: mqtt\r\n"
						"8: none\r\n"
				"0:menu\r\n");
		while (wait_uart1_rx_data(1) == 0) {
			//task_usart2_rx();
		}

		uart1_rx_data(buf, READ_RX);

		fprintf(UART1_FILE, "you send data:%s\r\n", buf);
		if (!strcmp(buf, "1")) {
			fprintf(UART1_FILE, "1:connect to server\r\n");
			sim800c_connect_server(0, 0);
		}
		else if (!strcmp(buf, "2")) {
			fprintf(UART1_FILE, "2:send data to server\r\n");
			
			while (1) {
				while (wait_uart1_rx_data(1000) == 0);
				uart1_rx_data(buf, READ_RX);
			
				if (!strcmp(buf, "0")) {
					break;
				}
				sim800c_send_tcp_data(buf);
			}
		}
		else if (!strcmp(buf, "3")) {
			INFO("CIP STATUS: %s\r\n", sim800c_cip_status_str(sim800c_get_cip_status_no()));
			fprintf(UART1_FILE, "3:read data\r\n");
			sim800c_read_tcp_data(0,2000);
		}
		else if (!strcmp(buf, "4")) {
			fprintf(UART1_FILE, "close connection\r\n");
			sim800c_close_tcp();
		}
		else if (!strcmp(buf, "0")) {
			fprintf(UART1_FILE, "menu\r\n");
		}
		else if (!strcmp(buf, "5")) {
			get_debug_type_from_user();
		}
		else if (!strcmp(buf, "6")) {
			init_sim800c();
		}
		else if (!strcmp(buf, "7")) {
			task(); 
		}
		else if (!strcmp(buf, "8")) {	
 
		}
	} 
}

