#include "task.h"
#include "sim800c.h"
#include "key.h"

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


#include "includes.h"

#define TASK_TIME 100

extern unsigned long long global_timer_ticket; //ms
extern unsigned long long global_timer_ticket_s;//second

tasks_status global_tasks_status;


void task_sim()
{

}

void task_cip_status()
{
	static unsigned long long ticket = 0;

	if ((global_timer_ticket - ticket) < 3000) {
		return;
	}
	
	sim800c_get_cip_status_no();
	if (g_sim800c_cip_status != SIM800C_CIP_STATUS6) {

		sim800c_connect_server(0, 0);
	}
	ticket = global_timer_ticket;
}

void task_usart2_rx()
{
	static unsigned long long ticket = 0;
	
	if ((global_timer_ticket - ticket) < 100) {
		return;
	}

	if (g_sim800c_cip_status == SIM800C_CIP_STATUS6) {//CONNECT
		sim800c_read_tcp_data(0,0);
	}
	
	ticket = global_timer_ticket;
}

extern int mqtt_publish_data(char* str);

void task_mqtt_publish_data()
{
	static unsigned long long ticket = 0;
	
	if ((global_timer_ticket - ticket) < 5000) {
		//return;
	}
	if (IS_KEY_PRESSED(KEY0_PRES)) {
		mqtt_publish_data("this is endpoint data,key0");
		CLEAR_KEY_PRESSED(KEY0_PRES);
	}
	else if (IS_KEY_PRESSED(KEY1_PRES)) {
		mqtt_publish_data("this is endpoint data,key1");
		CLEAR_KEY_PRESSED(KEY1_PRES);		
	}
	else if (IS_KEY_PRESSED(KEY2_PRES)) {
		mqtt_publish_data("this is endpoint data,key2");
		CLEAR_KEY_PRESSED(KEY2_PRES);		
	}
	else if (IS_KEY_PRESSED(WKUP_PRES)) {
		mqtt_publish_data("this is endpoint data,wkup");
		CLEAR_KEY_PRESSED(WKUP_PRES);		
	}
	else if ((global_timer_ticket - ticket) < 5000) {
		return;
	}
	mqtt_publish_data("this is endpoint data");
	ticket = global_timer_ticket;	
}

void task_init()
{
	global_tasks_status.cip_status = -1;
	//init_mqtt();
}

 
/////////////////////////UCOSII任务设置///////////////////////////////////
#define MQTT_SUB_PRIO     			4 
#define MQTT_SUB_STK_SIZE 	    		512
OS_STK MQTT_SUB_STK[MQTT_SUB_STK_SIZE];
void task_mqtt_sub(void *pdata);

#define MQTT_PUB_PRIO     			5 
#define MQTT_PUB_STK_SIZE 	    		512
OS_STK MQTT_PUB_STK[MQTT_PUB_STK_SIZE];
void task_mqtt_pub(void *pdata);

#define MQTT_INIT_PRIO     			6 
#define MQTT_INIT_STK_SIZE 	    		512
OS_STK MQTT_INIT_STK[MQTT_INIT_STK_SIZE];
void task_mqtt_init(void *pdata);

#define SIM800C_CONTROL_PRIO     			4 
#define SIM800C_CONTROL_STK_SIZE 	    		128
OS_STK SIM800C_CONTROL_STK[SIM800C_CONTROL_STK_SIZE];
void task_sim800c_control(void *pdata);

#define LED0_TASK_PRIO       			7 
#define LED0_STK_SIZE  		    		64
OS_STK LED0_TASK_STK[LED0_STK_SIZE];
void led0_task(void *pdata);

#define LED1_TASK_PRIO       			8 
#define LED1_STK_SIZE  				64
OS_STK LED1_TASK_STK[LED1_STK_SIZE];
void led1_task(void *pdata);

#define MAIN_TASK_PRIO       			9 
#define MAIN_STK_SIZE  		    		2048
OS_STK MAIN_TASK_STK[MAIN_STK_SIZE];
void main_task(void *pdata);

void create_tasks(void)
{
	OS_CPU_SR cpu_sr = 0;
	OSInit();

	OS_ENTER_CRITICAL();

	OSTaskCreate(main_task,(void *)0,(OS_STK*)&MAIN_TASK_STK[MAIN_STK_SIZE-1],MAIN_TASK_PRIO);
	OSTaskCreate(led0_task,(void *)0,(OS_STK*)&LED0_TASK_STK[LED0_STK_SIZE-1],LED0_TASK_PRIO);
	OSTaskCreate(led1_task,(void *)0,(OS_STK*)&LED1_TASK_STK[LED1_STK_SIZE-1],LED1_TASK_PRIO);

	OS_EXIT_CRITICAL();

	OSStart();	  	 
}
 
void create_mqtt_task()
{
	OS_CPU_SR cpu_sr = 0;

	OS_ENTER_CRITICAL();

	OSTaskCreate(task_mqtt_sub, (void *)0, (OS_STK*)&MQTT_SUB_STK[MQTT_SUB_STK_SIZE-1], MQTT_SUB_PRIO);
	//OSTaskCreate(task_mqtt_pub, (void *)0, (OS_STK*)&MQTT_PUB_STK[MQTT_PUB_STK_SIZE-1], MQTT_PUB_PRIO);
	//OSTaskCreate(task_mqtt_init, (void *)0, (OS_STK*)&MQTT_INIT_STK[MQTT_INIT_STK_SIZE-1], MQTT_INIT_PRIO);
	//OSTaskCreate(task_sim800c_status_monitor,
	//	     (void *)0,
	//	     (OS_STK*)&SIM800C_CONTROL_STK[SIM800C_CONTROL_STK_SIZE - 1],
	//	     MQTT_INIT_PRIO);

	OS_EXIT_CRITICAL();
}

//LED0任务
u16 cnt1 = 0;
void led0_task(void *pdata)
{	 	
	while(1)
	{
		LED0=0;
		delay_ms(500);
		LED0=1;
		delay_ms(500);
		//fprintf(UART1_FILE, "led0 task %ds\r\n", ++cnt1);
	};
}

//LED1任务
void led1_task(void *pdata)
{	  
	while(1)
	{
		LED1=0;
		delay_ms(400);
		LED1=1;
		delay_ms(400);
	};
}

extern int open_socket(void);
extern int close_socket(void);
extern int connect_mqtt_server(void);
extern int mqtt_subscribe(void);
extern int sim800c_connected(void);


void task_sim800c_control(void* pdata) 
{

}

void task_mqtt_init(void* pdata) 
{

}
extern int mqtt_publish_data(char* str);
extern void mqtt_disconnect(void);

void subscribed_data_arraived_cb(unsigned char* data, int len) 
{
	char msg[100];
	data[len] = 0;
	sprintf(msg, "endpoint ack:%s\r\n", data);

	INFO("len(%d), data:%s\r\n", len, data);

	if (!strncmp("close", (char*)data, 4)) {
		mqtt_disconnect();
	}
	else if (!strncmp("ack", (char*)data, 3)) {
		mqtt_publish_data(msg);
	}
	
}

extern void mqtt_ping(void);
extern int listen_sub_data(void (*cb)(unsigned char*, int));

void task_mqtt_sub(void *pdata)
{
	int rst;
	int nodata_times = 0;
	while (1) {
		if (!sim800c_connected()) {
			close_socket();
OPEN_SOCKET:		while (open_socket() != 1) {
				delay_ms(1000);
			}
			
			while (connect_mqtt_server() != 1) {
				delay_ms(1000);
				if (!sim800c_connected()) {
					close_socket();
					goto OPEN_SOCKET;
				}
			}
			
			while (mqtt_subscribe() != 1) {
				delay_ms(1000);
				if (!sim800c_connected()) {
					close_socket();
					goto OPEN_SOCKET;
				}
			}
		}

		//connected and subscribed ok
		nodata_times = 0;
		while (1)
		{
			rst = listen_sub_data(subscribed_data_arraived_cb);
			if (rst == -1) {
				close_socket();
				goto OPEN_SOCKET;
			}
			else if (rst == 0) {
				//no data
				if (++nodata_times % 100 == 0) {
					if (!sim800c_connected()) {
						close_socket();
						goto OPEN_SOCKET;
					}
					nodata_times = 0;
					mqtt_ping();

				}
				delay_ms(100);
			}
			else if (rst == 1) {
				nodata_times = 0;
				INFO("receive the subscribed data\r\n");
			}
		}
	}
}

void task_mqtt_pub(void *pdata)
{
	while (1) {
		if (mqtt_publish_data("helloworld") == -1) {
			delay_ms(5000);
			ERROR("publish failed\r\n");
		}
		INFO("publish ok\r\n");
		delay_ms(5000);
	}
}

void main_task(void *pdata)
{
	char buf[20];

  	//base64_encode("1234567890abcdefg", memory_pg0);
	//base64_decode(memory_pg0, memory_pg1);
	
	fprintf(UART1_FILE, "uart1 works...\n");
	fprintf(UART2_FILE, "uart2 works...\n");
	delay_s(3);

	while(!init_sim800c()) {
		ERROR("Failed to init sim800c, waiting 5s to retry\r\n")
		delay_s(5);
	}

	while(1)
	{
		fprintf(UART1_FILE,
			"I am from uart1, please input data: \r\n"
			"1:connect to server\r\n"
			"2:send data to server\r\n"
			"3:read data\r\n"
			"4:close connection\r\n"
			"5:set debug info\r\n"
			"6:init sim\r\n"
			"7:mqtt task\r\n"
			"8:none\r\n"
			"0:menu\r\n");

		while (wait_uart1_rx_data(2) == 0);

		uart1_rx_data(buf, READ_RX);

		fprintf(UART1_FILE, "you send data:%s\r\n", buf);
		if (!strcmp(buf, "1")) {
			sim800c_connect_server("39.108.59.249", 8089);
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
			create_mqtt_task(); 
		}
		else if (!strcmp(buf, "8")) {	
 
		}
	} 
}

