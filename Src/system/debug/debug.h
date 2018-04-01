#ifndef __DEBUG__H__
#define __DEBUG__H__
#include "stm32f10x.h"
#include "usart.h"

#define DEBUG_UART_FILE UART1_FILE

#define DEBUG_ENABLED(a) (debug_level & (1 << (a)))
#define DEBUG(type, format, ...) \
	if (DEBUG_ENABLED(type)) {\
		debug_print_indent();\
		fprintf(DEBUG_UART_FILE, format, __VA_ARGS__);\
	}

#define INFO(...) \
	if (DEBUG_ENABLED(e_debug_print_info)) { \
		debug_print_indent(); \
		fprintf(DEBUG_UART_FILE, "[Info]"); \
		fprintf(DEBUG_UART_FILE, __VA_ARGS__) ;\
	}

#define ERROR(...) \
	if (DEBUG_ENABLED(e_debug_print_error)) { \
		debug_print_indent(); \
		fprintf(DEBUG_UART_FILE, "***[Error]***"); \
		fprintf(DEBUG_UART_FILE, __VA_ARGS__) ;\
	}

#define WARNING(...) \
	if (DEBUG_ENABLED(e_debug_print_warning)) { \
		debug_print_indent(); \
		fprintf(DEBUG_UART_FILE, "---[Warning]---"); \
		fprintf(DEBUG_UART_FILE, __VA_ARGS__); \
	}

#define DEBUG_SIM_CMD_VAARGS(...) \
	if (DEBUG_ENABLED(e_debug_print_sim_cmd)) { \
		debug_print_indent(); \
		fprintf(DEBUG_UART_FILE, __VA_ARGS__) ;\
	}

#define DEBUG_SIM_CMD(cmd) \
	if (DEBUG_ENABLED(e_debug_print_sim_cmd)) { \
		debug_print_indent(); \
		fprintf(DEBUG_UART_FILE, "%s\r\n", cmd);\
	}

#define DEBUG_SIM_RESPOND(...) \
	if (DEBUG_ENABLED(e_debug_print_sim_respond)) { \
		debug_print_indent(); \
		fprintf(DEBUG_UART_FILE, __VA_ARGS__); \
	}

//use below together to calculate fun_level correct
#define DEBUG_FUNC_NAME() debug_print_func_name(__FUNCTION__)
#define DEBUG_FUNC_RESULT(rst) debug_print_func_result(rst, __FUNCTION__)


typedef enum {
	e_debug_none = -1,
	e_debug_print_info = 0,
	e_debug_print_error,
	e_debug_print_warning,
	e_debug_print_func_name,
	e_debug_print_sim_cmd,
	e_debug_print_sim_respond,
	e_debug_print_sim_cip_status,
	e_debug_print_mqtt_tcp_send_data,
	e_debug_print_mqtt_tcp_receive_data,
} debug_type_e;

typedef struct {
		char id;//a-z
	debug_type_e type;
         const char* type_description;
} debug_type_info_s;

extern u8 fun_level;
extern u32 debug_level;

void get_debug_type_from_user(void);

void debug_print_indent(void);
void debug_print_func_name(const char* str);
void debug_print_func_result(u8 rst, const char* str);

#endif

