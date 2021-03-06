#include "usart.h"
#include "delay.h"
#include "utility.h"
#include "string.h"

char* str_simplify(char* str)
{
	s16 i = 0, start = 0, end = 0;
	while (str[i] != '\0') {
		if (str[i] == ' ' || str[i] == '\r' || str[i] == '\n') {
			start = i + 1;
			++i;
			continue;
		}
		break;
	}

	i = strlen(str) - 1;
	end = i + 1;
	while (i >= 0) {
		if (str[i] == ' ' || str[i] == '\r' || str[i] == '\n') {
			end = i;
			--i;
			continue;
		}
		break;
	}
	for (i = start; i < end; ++i) {
		str[i - start] = str[i];
	}
	str[end - start] = 0;
	return str;
}

u8 simplify_sim800c_responding_str(char* str)
{
	char* p = NULL;
	u8 ok = 0;
	while ((p = strstr(str, "OK")) != NULL) {
		p[0] = ' ';
		p[1] = ' ';
		ok = 1;
	}
	
	while ((p = strstr(str, "ERROR")) != NULL) {
		p[0] = ' ';
		p[1] = ' ';
		p[2] = ' ';
		p[3] = ' ';
		p[4] = ' ';
	}
	str_simplify(str);

	return ok;
}

u8 wait_uart1_rx_data(u16 waitting_ms)
{
	int i = 0;
	u8 len = 0;
	
	if (USART_RX_STA & 0x8000) {
		len = USART_RX_STA & 0x3fff;//得到此次接收到的数据长度 
		return len;
	}

	if (waitting_ms > 0) {
		do {
			delay_ostimedly(1);
			if (USART_RX_STA & 0x8000) {
				len = USART_RX_STA & 0x3fff;//得到此次接收到的数据长度 
				break;
			}
		} while (++i < ms_to_ticket(waitting_ms));
	}
	else {
		while (1) {
			if (USART_RX_STA & 0x8000) {
				len = USART_RX_STA & 0x3fff;//得到此次接收到的数据长度 
				break;
			}
			delay_ostimedly(1);
		}
	}
	return len;
}

u8 uart1_rx_data(char* outdata, u8 only_checking_data)
{
	u8 len = USART_RX_STA & 0x3fff;
	USART_RX_BUF[len] = 0;
	if (outdata != 0) {
		memcpy((char*)outdata, (char*)USART_RX_BUF, len);
		outdata[len] = 0;
	}
	
	if (only_checking_data == READ_RX) {
		USART_RX_STA = 0;
	}
	return len;
}

void uart1_rx_buf_clear()
{
	uart1_rx_data(0 , READ_RX);
}

u8 wait_uart2_rx_data(u16 waitting_ms)
{
	int i = 0;
	u8 len = 0;
	
	if ((len = USART2_RX_STA & 0x3fff) != 0) {//得到此次接收到的数据长度
		//complete read one package
		if (USART2_RX_STA & 0x8000) {
			return len;
		}
	}

	if (waitting_ms > 0) {
		do {
			delay_ostimedly(1);
			if ((len = USART2_RX_STA & 0x3fff) != 0) {//得到此次接收到的数据长度
				//complete read one package
				if (USART2_RX_STA & 0x8000) {
					break;
				}
			}
		} while (++i < ms_to_ticket(waitting_ms));
	}
	else {
		while (1) {
			if ((len = USART2_RX_STA & 0x3fff) != 0) {
				//complete read one package
				if (USART2_RX_STA & 0x8000) {
					break;
				}
			}
			delay_ostimedly(1);
		}
	}
	return len;
}

u8 uart2_rx_data(char* outdata, u8 only_checking_data)
{
	u8 len = (USART2_RX_STA & 0x3fff);

	if (outdata != 0) {
		memcpy((char*)outdata, (char*)USART2_RX_BUF, len);
		outdata[len] = 0;
	}
	//only_checking_data: READ_RX/ONLY_CHECKING_RX_DATA
	if (only_checking_data == READ_RX) {
		USART2_RX_STA = 0;
	}
	return len;
}

void uart2_rx_buf_clear()
{
	uart2_rx_data(0 , READ_RX);
}



