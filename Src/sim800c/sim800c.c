#include "sim800c.h"
#include "usart.h"
#include "delay.h"
#include "utility.h"
#include "string.h"
#include "debug.h"

#define MAX_RETRY_TIME  3
#define TRY3TIMES(rst, fun, delay_fun) \
	rst = fun;\
	if (!rst) {\
		delay_fun;\
		rst = fun;\
		if (!rst) {\
			delay_fun;\
			rst = fun;\
		}\
	}

u8 tryntime = 0;
#define TRYnTIMES(n, rst, fun, delay_fun) \
	for (tryntime = 0; tryntime < n; ++tryntime) {\
		rst = fun;\
		if (rst) break;\
		delay_fun;\
	}

	
sim800c_data_s sim_data;	
char server_address[20];
char server_port[8];
s8 g_sim800c_cip_status = -1;


const char* SIM800C_CIP_STATUS[] = {
"IP INITIAL", 		//state 0
"IP START",   		//state 1
"IP CONFIG",
"IP GPRSACT",
"IP STATUS",
"TCP CONNECTING",
"CONNECT OK",
"TCP CLOSING",
"TCP CLOSED",
"PDP DEACT"		//state 9
};


#define CIP_STATUS() (sim800c_cip_status_str(sim800c_get_cip_status_no()))

int sim800c_connected()
{
	if (0 == strncmp(CIP_STATUS(), "CONNECT OK", sizeof("CONNECT OK"))) {
		return 1;
	}
	return 0;
}

const char* sim800c_cip_status_str(s8 no)
{
	if (no >= 0 && no < sizeof(SIM800C_CIP_STATUS) / sizeof(SIM800C_CIP_STATUS[0])) {
		return SIM800C_CIP_STATUS[no];
	}
	else {
		return "CIP STATUS ERROR";
	}
}

u8 sim800c_write(const char* cmd)
{
	DEBUG_SIM_CMD_VAARGS("[write sim cmd] \"%s\"\r\n", cmd);

	//send the command
	fprintf(UART2_FILE, "%s\r\n", cmd);
	
	return 1;
}

void clear_rx_buf()
{
	uart2_rx_buf_clear();
	memset(sim_data.m_rx_buf, 0, sizeof(sim_data.m_rx_buf));
}


extern u16 USART2_RX_STA;  
u8 get_sim800c_respond(const char* cmd, const char* check_rst_contained_str, u16 max_waiting_time)
{
	u8 len = 0;
	u8 retry = 0;
	u8 ok = 1;
	u8 retry_times = 0;
	const u8 RETRY_INTERVAL = 100;//100ms
	retry_times = max_waiting_time / RETRY_INTERVAL + 1;
	
	sim_data.m_rx_buf[0] = '\0';
	//waiting to get the responding data
	for (retry = 0; retry < retry_times; ++retry) {
		//waiting the rx data
		len = wait_uart2_rx_data(100);
		if (len == 0) {
			//DEBUG_SIM_RESPOND("[read sim]no data, wait %dms and read again\r\n", RETRY_INTERVAL);
			delay_ms(RETRY_INTERVAL);
			continue;
		}
		
		//pre read the rx data, not really read out
		len = uart2_rx_data(sim_data.m_rx_buf, CHECK_RX);
		//DEBUG_SIM_RESPOND("pre_read data:\"%s\"\r\n", sim_data.m_rx_buf);
		
		if (check_rst_contained_str) {
			if (strstr((const char*)sim_data.m_rx_buf, check_rst_contained_str)) {
				ok = 1;
				break;
			}
			else {
				ok = 0;
				//cannot find the wanted respond data, wait again
				if (strstr((const char*)sim_data.m_rx_buf, "OK\r\n") || 
				    strstr((const char*)sim_data.m_rx_buf, "ERROR\r\n") || 
				    strstr((const char*)sim_data.m_rx_buf, "> ")) {
					break;
				}
				delay_ms(RETRY_INTERVAL);
			}
		}
		else if (strstr((const char*)sim_data.m_rx_buf, "OK\r\n") || 
		         strstr((const char*)sim_data.m_rx_buf, "> ")) {
			ok = 1;
			break;
		}
		else if (strstr((const char*)sim_data.m_rx_buf, "ERROR\r\n")) {
			ok = 0;
			break;
		}
		else {
			delay_ms(RETRY_INTERVAL);
		}
	}

	len = uart2_rx_data(sim_data.m_rx_buf, READ_RX);
	
	DEBUG_SIM_RESPOND("[read sim] len=%d, data=\"%s\"\r\n", len, sim_data.m_rx_buf);
	return ok;
}

u8 setup_sim800c(const char* cmd, const char* rst_contained_str, u16 max_waiting_time)
{
	u8 rst;
	
	//clear read buffer
	clear_rx_buf();
	
	sim800c_write(cmd);
	
	rst = get_sim800c_respond(cmd, rst_contained_str, max_waiting_time);

	return rst;
}

u8 query_sim800c()
{
	s8 rst = 1;
	int rsii = -1, ber = 0;
	u8 retry = 0;
	
	/*AT+CPIN?
	 * 用于查询 SIM 卡的状态，主要是 PIN 码，
	 * 如果该指令返回：+CPIN:READY,则表明SIM 卡状态正常，
	 * 返回其他值，则有可能是没有 SIM 卡。*/
	//try 60 times, 1 second every time, so 60second
	TRYnTIMES(60, rst, setup_sim800c("AT+CPIN?", "READY", 100), delay_s(1));
	
	if (!rst) {
		ERROR("Failed to get sim status\r\n");
	}
	else {
		INFO("Sim status: \"%s\"\r\n", sim_data.m_rx_buf);
	}

	if (rst) {
		TRYnTIMES(60, rst, setup_sim800c("AT+CREG?", "OK", 100), delay_s(1));
		if (!rst) {
			ERROR("Failed to query GSM Network Registration\r\n");
		}
		else {
			INFO("GSM Network Registration: \"%s\"\r\n", sim_data.m_rx_buf);
		}
	}
	
	if (rst) {
		TRYnTIMES(60, rst, setup_sim800c("AT+CGREG?", "OK", 100), delay_s(1));
		if (!rst) {
			ERROR("Failed to query GPRS Network Registration\r\n");
		}
		else {
			INFO("GPRS Network Registration: \"%s\"\r\n", sim_data.m_rx_buf);
		}
	}

	/*AT+CSQ 
	该指令用于查询信号质量，返回 SIM800C 模块的接收信号强度，如返回：+CSQ：24,0，
	表示信号强度是24（最大的有效值是 31）。如果信号强度过低，则要检查天线是否接好了? */
	if (rst) {
		for (retry = 0; retry < 30; ++retry) {
			rst = setup_sim800c("AT+CSQ", "OK", 100);
			if (!rst) {
				ERROR("Failed to get the signal quality, will retry(%d) again\r\n", retry);
				delay_s(1);
				continue;
			}

			str_simplify(sim_data.m_rx_buf);
			if (!strncmp(sim_data.m_rx_buf, "AT+CSQ\r\r\n+CSQ: ", sizeof("AT+CSQ\r\r\n+CSQ: ") - 1)) {
				sscanf(sim_data.m_rx_buf,"AT+CSQ\r\r\n+CSQ: %d,%d", &rsii, &ber);
			}
			else if (!strncmp(sim_data.m_rx_buf, "+CSQ: ", sizeof("+CSQ: ") - 1)) {
				sscanf(sim_data.m_rx_buf,"+CSQ: %d,%d", &rsii, &ber);
			}
			if (rsii >= 0) {
				if (rsii == 99) {
					ERROR("Signal quality(%d) is not known or not detectable\r\n", rsii);
				}
				else if (rsii >= 20) {
					INFO("Signal quality(%d) is good\r\n", rsii);
				}
				else if (rsii > 15) {
					WARNING("Signal quality(%d) is not good\r\n", rsii);
				}
				else if (rsii > 10) {
					WARNING("Signal quality(%d) is a little poor\r\n", rsii);
				}
				else if (rsii > 5) {
					WARNING("Signal quality(%d) is poor\r\n", rsii);
				}
				else {
					WARNING("Signal quality(%d) is poor poor\r\n", rsii);
				}

				if (rsii > 10 && rsii < 99) {
					rst = 1;
					break;
				}
			}
			else {
				ERROR("Signal quality(%d) is not known or not detectable\r\n", rsii);
				ERROR("\"%s\"\r\n", sim_data.m_rx_buf);
			}
			rst = 0;
			delay_s(1);
		}
	}

	if (rst) {
		setup_sim800c("AT+CNUM", NULL, 100);
		simplify_sim800c_responding_str(sim_data.m_rx_buf);
		INFO("Sim Card Number: %s\r\n", sim_data.m_rx_buf);
	}

	return rst;
}

int init_sim800c()
{
	u8 rst = 1;

	memset(&sim_data, 0, sizeof(sim_data));

	//first time to calibre the uart baudrate of sim800c
	sim800c_write("AT");
	
	delay_ms(50);
	
	clear_rx_buf();
	TRYnTIMES(2, rst, setup_sim800c("AT", "OK", 100), delay_ms(50));
	
	//don't echo command 
	sim800c_write("ATE0");
	
	if (rst) {
		rst = query_sim800c();
	}

	return rst;
}

u8 get_cip_status_raw(char* buf)
{
	u8 retry = 0;
	u8 len = 0;
	const char* cmd = "AT+CIPSTATUS";
	const char* rst_contained_str = "STATE:";
	
	//clear uart2 read buffer
	clear_rx_buf();
	*buf = 0;

	fprintf(UART2_FILE, "%s\r\n", cmd);
	delay_ms(10);
	
	for (retry = 0; retry < MAX_RETRY_TIME; ++retry) {
		len = wait_uart2_rx_data(500);
		if (len == 0) {
			//fprintf(UART1_FILE, "[RESPOND] no data, wait %dms and read again\r\n", waiting_ms);
			delay_ms(500);
			continue;
		}
		
		//pre read the rx data, not really read out
		len = uart2_rx_data(buf, CHECK_RX);
		//fprintf(UART1_FILE, "[RESPOND] preread data:\r\n\"%s\"\r\n", buf);
		
		//have not read the prefered string, continue
		if (!strstr((const char*)buf, rst_contained_str)) {
			delay_ms(500);
			continue;
		}
		
		if (strstr((const char*)buf, "\r\n") || strstr((const char*)buf, "> ")) {
			//read out the rx data, the rx buffer will be cleared
			//fprintf(UART1_FILE, "[RESPOND] final data:\r\n\"%s\"\r\n", buf);
		
			break;
		}
		else {
			//have not read the completely data, waiting and then continue
			delay_ms(500);
			//fprintf(UART1_FILE, "[RESPOND] ***format error of responding data***\r\n");
		}
	}
	len = uart2_rx_data(buf, READ_RX);
	//fprintf(UART1_FILE, "[CIPSTATUS] raw data \"%s\"\r\n", buf);
	return len;
}

s8 sim800c_get_cip_status_no()
{
	s8 i = 0;
	if (get_cip_status_raw(sim_data.m_rx_buf) > 0) {
		for (i = 0; i < 10; ++i) {
			if (strstr((const char*)sim_data.m_rx_buf, SIM800C_CIP_STATUS[i])) {
				DEBUG(e_debug_print_sim_cip_status, "cip status: %s\r\n", SIM800C_CIP_STATUS[i]);
				g_sim800c_cip_status = i;
				break;
			}
		}
	}
	if (g_sim800c_cip_status == -1) {
		ERROR("Failed to get cip status, raw data:\"%s\"\r\n", sim_data.m_rx_buf);
	}
	return g_sim800c_cip_status;
}

s8 wait_cip_status(u8 status_no, u8 time_out_seconds)
{
	u8 i = 0;
	s8 status = -1;
	for (i = 0; i < 2 * time_out_seconds; ++i) {
		status = sim800c_get_cip_status_no();
		if (status == status_no) {
			break;
		}
		delay_ms(500);
	}
	return status;
}

int sim800c_connect_server(char* addr, int port)
{
	u8 rst = 0;
	u8 retry = 0;
	s8 status = 0;
	char str[50];
	
	const char* fun_name = "Connect TCP Server";
	debug_print_func_name(fun_name);

	if (addr) {
		sprintf(server_address, "%s", addr);
		sprintf(server_port, "%d", port);
	}
	else {
		sprintf(server_address, "%s", "39.108.59.249");
		sprintf(server_port, "%d", 8089);
	}
	//step1=> goto SIM800C_CIP_STATUS0: IP INITIAL
	if (sim800c_get_cip_status_no() != SIM800C_CIP_STATUS0) {
		setup_sim800c("AT+CIPSHUT", "SHUT OK", 5000);
	}
	
	if (SIM800C_CIP_STATUS0 != wait_cip_status(SIM800C_CIP_STATUS0, 60)) {
		rst = 0;
		goto DONE;
	}
	setup_sim800c("AT+CIPMODE=0", "OK", 100);
	setup_sim800c("AT+CIPRXGET=1", "OK", 100);
	setup_sim800c("AT+CIPHEAD=1", NULL, 100);
	setup_sim800c("AT+CIPSRIP=1", NULL, 100);
	setup_sim800c("AT+CIPSHOWTP", NULL, 100);

STATUS0:
	fprintf(UART1_FILE, "STATUS0\r\n");
	if (++retry > 5) {
		rst = 0;
		goto DONE;
	}

	//SIM800C_CIP_STATUS0->SIM800C_CIP_STATUS1: IP START
	setup_sim800c("AT+CSTT=\"CMNET\"", "OK", 500);
	if (SIM800C_CIP_STATUS0 == wait_cip_status(SIM800C_CIP_STATUS1, 10)) {
		delay_s(5);
		goto STATUS0;
	}
	
//STATUS1:
	//SIM800C_CIP_STATUS1->SIM800C_CIP_STATUS2(IP CIP CONFIG)->SIM800C_CIP_STATUS3(IP GPRSACT)
	setup_sim800c("AT+CIICR", "OK", 10000);
	//SIM800C_CIP_STATUS2->SIM800C_CIP_STATUS3 automatically
	status = wait_cip_status(SIM800C_CIP_STATUS3, 10);
	if (status == SIM800C_CIP_STATUS0) {
		delay_s(5);
		goto STATUS0;
	}
	else if (status == SIM800C_CIP_STATUS9) {
		goto STATUS9;
	}
	else if (status == SIM800C_CIP_STATUS3) {
		//ok
	}

//STATUS3:
	//SIM800C_CIP_STATUS3->SIM800C_CIP_STATUS4(IP STATUS)
	setup_sim800c("AT+CIFSR", ".", 1000);
	if (SIM800C_CIP_STATUS9 == wait_cip_status(SIM800C_CIP_STATUS4, 10)) {
		goto STATUS9;
	}

//STATUS4:	

	//SIM800C_CIP_STATUS4->SIM800C_CIP_STATUS5(TCP CONNECTING)->SIM800C_CIP_STATUS6(CONNECT OK)
	sprintf(str, "AT+CIPSTART=\"TCP\",\"%s\",\"%s\"", server_address, server_port);
	setup_sim800c(str, "OK", 5000);
	status = wait_cip_status(SIM800C_CIP_STATUS6, 10);
	if (SIM800C_CIP_STATUS0 == status) {
		delay_s(5);
		goto STATUS0;
	}
	else if (SIM800C_CIP_STATUS9 == status) {
		goto STATUS9;
	}
	else if (status == SIM800C_CIP_STATUS6) {
		rst = 1;
		goto DONE;
	}
	else {//status == SIM800C_CIP_STATUS8
		rst = 0;
		goto DONE;
	}


STATUS9:
	fprintf(UART1_FILE, "STATUS9\r\n");
	if (SIM800C_CIP_STATUS0 != wait_cip_status(SIM800C_CIP_STATUS0, 60)) {
		rst = 0;
		goto DONE;
	}
	goto STATUS0;

DONE:
	debug_print_func_result(rst, fun_name);

	INFO("connect to server: %s\r\n", sim800c_cip_status_str(g_sim800c_cip_status));
	return rst;
}

int sim800c_send_tcp_data(const char* str)
{
	u8 rst = 1;
	char end[2] = {0x1A, 0};
	//if (debug_level >= 3) fprintf(UART1_FILE, "[SEND TCP DATA to SIM800C] \"%s\"\r\n", str);
	
	//send the command
	setup_sim800c("AT+CIPSEND", ">", 500);
	
	//clear uart2 read buffer
	clear_rx_buf();
	
	fprintf(UART2_FILE, "%s%s", str, end);
	delay_s(1);
	
	rst = get_sim800c_respond(str, "SEND OK", 5000);
	
	return rst;
}

int sim800c_write_tcp_data(const char* str, int len)
{
	u8 rst = 1;
	int i;
	char end = 0x1A;
	
	//send the command
	setup_sim800c("AT+CIPSEND", ">", 500);
	
	//clear uart2 read buffer
	clear_rx_buf();
	
	for (i = 0; i < len; ++i) {
		fputc(str[i], UART2_FILE);
	}
	fputc(end, UART2_FILE);
	//delay_s(1);
	delay_ms(500);
	
	rst = get_sim800c_respond(str, "SEND OK", 5000);
	
	if (rst) {
		if (DEBUG_ENABLED(e_debug_print_mqtt_tcp_send_data)) {
			fprintf(DEBUG_UART_FILE, "send tcp data: ");
			print_hex_ln(DEBUG_UART_FILE, str, len);
		}
		INFO("sim800c_write_tcp_data ok, len=%d\r\n", len);
		return len;
	}
	ERROR("sim800c_write_tcp_data fail\r\n");
	return 0;
}

char tcp_server_ip[32];
char* sim800c_read_tcp_data(int* length, u16 waiting_time)
{
	int mode = 0, reqlength = 0, cnflength = 0;
	char* start = NULL;

	//\r\n+CIPRXGET: 2,0,0,"a.b.c.d:port"\r\ndatas\r\nOK\r\n
	setup_sim800c("AT+CIPRXGET=2,1460", "OK\r\n", waiting_time);
	
	if (-1 == sscanf(sim_data.m_rx_buf, "\r\n+CIPRXGET: %d,%d,%d,%s\r\n", &mode, &reqlength, &cnflength, tcp_server_ip)) {
		ERROR("tcp data: \"%s\"\r\n", sim_data.m_rx_buf);
		//sim_data.m_rx_buf will be changed in CIP_STATUS
		ERROR("read tcp data error, cip status: %s\r\n", CIP_STATUS());
		return NULL;
	}
	/*sscanf(sim_data.m_rx_buf,
	       "\r\n+CIPRXGET: %d,%d,%d,%s\r\n%s\r\nOK\r\n", 
	       &mode, &reqlength, &cnflength, tcp_server_ip, tcp_rx_buf);
	*/
	if (reqlength > 0) {
		start = strstr(sim_data.m_rx_buf + 2, "\r\n");
		start += 2;
		if (start != NULL) {
			start[reqlength] = '\0';
			
			if (DEBUG_ENABLED(e_debug_print_mqtt_tcp_receive_data)) {
				fprintf(DEBUG_UART_FILE, "server ip: %s\r\nreqlength:%d\r\n", tcp_server_ip, reqlength);
				fprintf(DEBUG_UART_FILE, "data(hex): ");
				print_hex_ln(DEBUG_UART_FILE, start, reqlength);
			}
		}
		
		if (length) {
		    *length = reqlength;
		}
		return start;
	}

	if (length) *length = 0;
	return NULL;
}

int sim800c_close_tcp()
{
	u8 rst = 1;

	const char* fun_name = "Close TCP Connection";
	debug_print_func_name(fun_name);
	
	if (rst) rst = setup_sim800c("AT+CIPCLOSE=1", "CLOSE OK", 5000);
	if (rst) rst = setup_sim800c("AT+CIPSHUT", "SHUT OK", 5000);
	
	debug_print_func_result(rst, fun_name);
	return rst;
}

void test_sim800c_tcp()
{
	u8 rst = 0;
	
	rst = init_sim800c();

	if (rst) rst = sim800c_connect_server(0, 0);
	
	if (rst) rst = sim800c_send_tcp_data("helloworld");
	
	if (rst) rst = sim800c_close_tcp();
}

int tcp_rx_data_length()
{
	if (sim_data.m_tcp_rx_data_buf_end >= sim_data.m_tcp_rx_data_buf_start) {
		return (sim_data.m_tcp_rx_data_buf_end - sim_data.m_tcp_rx_data_buf_start);
	}
	else {
		return (TCP_RX_DATA_BUFFER_SIZE - (sim_data.m_tcp_rx_data_buf_start - sim_data.m_tcp_rx_data_buf_end));
	}
}

int tcp_rx_push_data(char* str, int len)
{
	int i;
	int pos;
	
	for (i = 0; i < len; ++i) {
		pos = (sim_data.m_tcp_rx_data_buf_end + i) % TCP_RX_DATA_BUFFER_SIZE;
		if (pos == sim_data.m_tcp_rx_data_buf_start - 1) {
			break;
		}
		sim_data.m_tcp_rx_data_buf[(sim_data.m_tcp_rx_data_buf_end + i) % TCP_RX_DATA_BUFFER_SIZE] = str[i];
	}
	sim_data.m_tcp_rx_data_buf_end = (sim_data.m_tcp_rx_data_buf_end + i) % TCP_RX_DATA_BUFFER_SIZE;
	
	return i;
}

int tcp_rx_pop_data(char* str, int len)
{
	int count = 0;
	int pos = sim_data.m_tcp_rx_data_buf_start;

	while (pos != sim_data.m_tcp_rx_data_buf_end) {
		str[count++] = sim_data.m_tcp_rx_data_buf[pos++];
		if (count == len) break;

		pos %= TCP_RX_DATA_BUFFER_SIZE;
	}

	sim_data.m_tcp_rx_data_buf_start = (sim_data.m_tcp_rx_data_buf_start + count) % TCP_RX_DATA_BUFFER_SIZE;
	
	if (sim_data.m_tcp_rx_data_buf_start == sim_data.m_tcp_rx_data_buf_end) {
		sim_data.m_tcp_rx_data_buf_start = 0;
		sim_data.m_tcp_rx_data_buf_end = 0;
	}
	return count;
}

int sim800c_transport_getdata(unsigned char* buf, int count)
{
	int len = 0;
	char* d = 0;
	
	d = sim800c_read_tcp_data(&len, 100);
	if (d && len > 0) {
		tcp_rx_push_data(d, len);
	}

	if (count > tcp_rx_data_length()) {
		return 0;
	}

	return tcp_rx_pop_data((char*)buf, count);	
}

