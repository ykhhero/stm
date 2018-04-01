#include "debug.h"
#include "stdio.h"
#include "usart.h"
#include "utility.h"
#include "ctype.h"

u8 fun_level = 0;
//default debug type values
u32 debug_level = (1 << e_debug_print_info) | 
		  (1 << e_debug_print_error) |
		  (1 << e_debug_print_warning);// |  (1 << e_debug_print_sim_cmd);

const debug_type_info_s debug_type_map[] = {
	{'0', e_debug_none, 		"Quit"},
	{'1', e_debug_none, 		"No debug"},
	{'i', e_debug_print_info, 	"Print info message"},
	{'e', e_debug_print_error, 	"Print error message"},
	{'w', e_debug_print_warning, 	"Print warning message"},
	{'f', e_debug_print_func_name, 	"Print function name"},
	{'c', e_debug_print_sim_cmd, 	"Print sim800c command"},
	{'r', e_debug_print_sim_respond,"Print sim800c respond"},
	{'s', e_debug_print_sim_cip_status,"Print sim800c cip status"},
	{'m', e_debug_print_mqtt_tcp_send_data,"Print mqtt sended tcp data"},
	{'n', e_debug_print_mqtt_tcp_receive_data,"Print mqtt received tcp data"}
};

void set_debug_level(char c)
{
	u8 i;
	for (i = 0; i < sizeof(debug_type_map) / sizeof(debug_type_info_s); ++i) {
		if (c >= 'a' && c <= 'z') {
			//set
			if (debug_type_map[i].id == c) {
				debug_level |= (1 << debug_type_map[i].type);
			}
		}
		else if (c >= 'A' && c <= 'Z') {
			//unset
			if (debug_type_map[i].id - c == 'a' - 'A') {
				debug_level ^= (1 << debug_type_map[i].type);
			}
		}
	}
}

void get_debug_type_from_user()
{
	u8 i;
	u8 len;
	char buf[32];
	
	fprintf(DEBUG_UART_FILE, "\r\n/*Please Choose the Debug Message to Print.  */\r\n"
		       		 "/*Lower case(a)to set, Upper case(A) to unset*/\r\n"
		       		 "/*You can combine the setting(ab, aB, a0,...)*/\r\n"
				 "/*(eg: '0' or 'a' or 'aB' or 'Ab0' ...)      */\r\n\r\n");
	for (i = 0; i < sizeof(debug_type_map) / sizeof(debug_type_info_s); ++i) {
		if (debug_type_map[i].id >= 'a' && debug_type_map[i].id <= 'z') {
			if (DEBUG_ENABLED(debug_type_map[i].type)) {
				fprintf(DEBUG_UART_FILE, "[Checked] '%c': %s\r\n", 
					debug_type_map[i].id,
					debug_type_map[i].type_description);
			}
			else {
				fprintf(DEBUG_UART_FILE, "          '%c': %s\r\n",
					debug_type_map[i].id,
					debug_type_map[i].type_description);
			}
		}
		else {
			fprintf(DEBUG_UART_FILE, "          '%c': %s\r\n",
				debug_type_map[i].id,
				debug_type_map[i].type_description);
		}
	}
	
	while (wait_uart1_rx_data(0) == 0);

	len = uart1_rx_data(buf, READ_RX);
	for (i = 0; i < len; ++i) {
		if (buf[i] == '0') {
			break;
		}
		else if (buf[i] == '1') {
			debug_level = 0;
			break;
		}
		else {
			set_debug_level(buf[i]);
		}
	}
}

void debug_print_indent()
{
	u8 i;
	
	if (!DEBUG_ENABLED(e_debug_print_func_name)) return;

	for (i = 0; i < fun_level; ++i) {
		fprintf(DEBUG_UART_FILE, "  ");
	}
}

void debug_print_func_name(const char* str)
{
	if (!DEBUG_ENABLED(e_debug_print_func_name)) return;
	
	fprintf(DEBUG_UART_FILE, "\r\n");
	debug_print_indent();
	fprintf(DEBUG_UART_FILE, "[%s]\r\n", str);
	
	++fun_level;
}

void debug_print_func_result(u8 rst, const char* str)
{
	if (!DEBUG_ENABLED(e_debug_print_func_name)) return;

	--fun_level;

	debug_print_indent();
	if (rst) {
		fprintf(DEBUG_UART_FILE, "[%s] [OK]\r\n\r\n", str);
	}
	else {
		fprintf(DEBUG_UART_FILE, "[%s] [FAIL]\r\n\r\n", str);
	}
}

