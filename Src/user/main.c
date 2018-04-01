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


void init(void);
extern void create_tasks(void);

int main(void)
{
	init();
	create_tasks();
}
 
void init(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // 设 置 NVIC 中 断 分 组 2	
	delay_init();

	uart_init(115200);
	uart2_init(115200);

	LED_Init();
	BEEP_Init();
	KEY_Init();
	EXTIX_Init();
	LED0=0;
	//init_memory();
}



