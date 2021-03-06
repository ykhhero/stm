#include "sys.h"
#include "usart.h"	  
////////////////////////////////////////////////////////////////////////////////// 	 
//如果使用ucos,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//ucos 使用	  
#endif

void print_hex(FILE* f, const char* str, int len)
{
	int i;
	for (i = 0; i < len; ++i) {
		fprintf(f, "0x%x ", str[i]);
	}
}

void print_hex_ln(FILE* f, const char* str, int len)
{
	int i;
	for (i = 0; i < len; ++i) {
		fprintf(f, "0x%x ", str[i]);
	}
	fprintf(f, "\r\n");
}

//printf use which uart, default is uart1
unsigned char printf_use_uart_no = 1;//use uart1
//no : 1 uart1, 2 uart2
void set_uart_no_for_printf(unsigned char no)
{
	printf_use_uart_no = no;
}
//////////////////////////////////////////////////////////////////
//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
_sys_exit(int x) 
{ 
	x = x; 
}

//重定义fputc函数 
int fputc(int ch, FILE *f)
{
	//fprintf flow
	if (f == UART1_FILE) {
		while((USART1->SR&0X40)==0);//循环发送,直到发送完毕   
		USART1->DR = (u8) ch;
	}
	else if (f == UART2_FILE) {
		while((USART2->SR&0X40)==0);//循环发送,4直到发送完毕   
		USART2->DR = (u8) ch;      
	}
	else {//printf flow
		if (printf_use_uart_no == 1) {
			while((USART1->SR&0X40)==0);//循环发送,直到发送完毕   
			USART1->DR = (u8) ch; 
		}
		else if (printf_use_uart_no == 2) {
			while((USART2->SR&0X40)==0);//循环发送,直到发送完毕   
			USART2->DR = (u8) ch;      
		}
	}
	return ch;
}

int fgetc(FILE *f)
{
	//fprintf flow
	if (f == UART1_FILE) {
		while(!(USART1->SR & USART_FLAG_RXNE));  
		return ((int)(USART1->DR & 0X1FF));  
	}
	else if (f == UART2_FILE) {
		while(!(USART2->SR & USART_FLAG_RXNE));  
		return ((int)(USART2->DR & 0X1FF));   
	}
	else {//printf flow
		if (printf_use_uart_no == 1) {
			while(!(USART1->SR & USART_FLAG_RXNE));  
			return ((int)(USART1->DR & 0X1FF));  
		}
		else if (printf_use_uart_no == 2) {
			while(!(USART2->SR & USART_FLAG_RXNE));  
			return ((int)(USART2->DR & 0X1FF));      
		}
	}
	return 0;
}
#endif 

/*使用microLib的方法*/
 /* 
int fputc(int ch, FILE *f)
{
	USART_SendData(USART1, (uint8_t) ch);

	while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {}	
   
    return ch;
}
int GetKey (void)  { 

    while (!(USART1->SR & USART_FLAG_RXNE));

    return ((int)(USART1->DR & 0x1FF));
}
*/
 

//串口1中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   	
u8 USART_RX_BUF[USART_REC_LEN];
u8 USART2_RX_BUF[USART2_REC_LEN];
//usart1 接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u16 USART_RX_STA=0;       //接收状态标记
//usart2 bit15 one segment done
//bit13-0 size
u16 USART2_RX_STA=0;       //接收状态标记

void uart_init(u32 bound)
{
	//GPIO端口设置
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//使能USART1，GPIOA时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);	//使能USART1，GPIOA时钟
	//USART1_TX   GPIOA.9
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.9 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOA.9

	//USART1_RX	  GPIOA.10初始化 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;//PA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOA.10  

	//Usart1 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器

	//USART 初始化设置

	USART_InitStructure.USART_BaudRate = bound;//串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式

	USART_Init(USART1, &USART_InitStructure); //初始化串口1
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启串口接受中断
	USART_Cmd(USART1, ENABLE);                    //使能串口1 
}
  
void uart2_init(u32 bound)
{
	//GPIO端口设置
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//使能USART1，GPIOA时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);	//使能USART1，GPIOA时钟
	//USART2_TX   GPIOA.2
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//USART2_RX	  GPIOA.3 初始化 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//Usart2 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器

	//USART 初始化设置
	USART_InitStructure.USART_BaudRate = bound;//串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式


	USART_Init(USART2, &USART_InitStructure); //初始化串口2
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//开启串口接受中断
	USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);
	
	USART_Cmd(USART2, ENABLE);                    //使能串口2 
}

void USART1_IRQHandler(void)                	//串口1中断服务程序
{
	u8 Res;

	//接收中断(接收到的数据必须是0x0d 0x0a结尾)
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
		Res = USART_ReceiveData(USART1);//读取接收到的数据

		if ((USART_RX_STA & 0x8000) == 0) {//接收未完成
			if (USART_RX_STA & 0x4000) {//接收到了0x0d
				if (Res != 0x0a) {
					USART_RX_STA = 0;//接收错误,重新开始
				}
				else {
					USART_RX_STA |= 0x8000;//接收完成了
					USART_RX_BUF[USART_RX_STA & 0X3FFF] = 0;
				}
			}
			else {//还没收到0X0D
				if (Res == 0x0d) {
					USART_RX_STA |= 0x4000;
				}
				else {
					USART_RX_BUF[USART_RX_STA & 0X3FFF] = Res;
					++USART_RX_STA;
					if (USART_RX_STA > (USART_REC_LEN-1)) {
						USART_RX_STA=0;//接收数据错误,重新开始接收
					}
				}
			}
		}
	}
}

void USART2_IRQHandler(void)
{
	u8 Res;

	if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
		Res = USART_ReceiveData(USART2);//读取接收到的数据

		USART2_RX_BUF[USART2_RX_STA & 0X3FFF] = Res;
		++USART2_RX_STA;
		if ((USART2_RX_STA & 0X3FFF) > (USART2_REC_LEN-1)) {
			USART2_RX_STA = 0;//接收数据错误,重新开始接收
		}
	}
	else if (USART_GetITStatus(USART2, USART_IT_IDLE) != RESET) {
		//complete one frame
		USART2->SR;
		USART2->DR;
		USART2_RX_STA |= 0x8000;

	}
}

