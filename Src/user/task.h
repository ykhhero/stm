#ifndef __TASK__H__
#define __TASK__H__

#include "stm32f10x.h" 

typedef struct {
	s8 cip_status;

} tasks_status;
extern tasks_status global_tasks_status;

extern void task(void);
extern void task_usart2_rx(void);

#endif
