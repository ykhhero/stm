#ifndef __sim800c__
#define __sim800c__

#include "stm32f10x.h"
#include "usart.h"

#define SIM800C_CIP_STATUS0 0 //IP INITIAL
#define SIM800C_CIP_STATUS1 1 //IP START
#define SIM800C_CIP_STATUS2 2 //IP CONFIG
#define SIM800C_CIP_STATUS3 3 //IP GPRSACT
#define SIM800C_CIP_STATUS4 4 //IP STATUS
#define SIM800C_CIP_STATUS5 5 //TCP CONNECTING
#define SIM800C_CIP_STATUS6 6 //CONNECT OK
#define SIM800C_CIP_STATUS7 7 //TCP CLOSING
#define SIM800C_CIP_STATUS8 8 //TCP CLOSED
#define SIM800C_CIP_STATUS9 9 //PDP DEACT

extern const char* sim800c_cip_status_str(s8 no);

extern s8 g_sim800c_cip_status;
extern s8 sim800c_get_cip_status_no(void);

#define TCP_RX_DATA_BUFFER_SIZE 1024
typedef struct {
	int  m_status;
	
	char m_tcp_rx_data_buf[TCP_RX_DATA_BUFFER_SIZE];
	int  m_tcp_rx_data_buf_start;
	int  m_tcp_rx_data_buf_end;
	
       char* m_tcp_tx_data;
        int  m_tcp_tx_data_len;
	int  m_tcp_tx_done;

	char m_rx_buf[USART2_REC_LEN];//SIM800C uart respond data

} sim800c_data_s;

extern sim800c_data_s sim_data;

extern int init_sim800c(void);
extern int sim800c_connect_server(char* addr, int port);
extern int sim800c_close_tcp(void);
extern char* sim800c_read_tcp_data(int* length/*out*/, u16 waiting_time);
extern int sim800c_send_tcp_data(const char*);
extern int sim800c_write_tcp_data(const char*, int len);
extern int sim800c_transport_getdata(unsigned char* buf, int count);
extern void test_sim800c_tcp(void);


#endif
