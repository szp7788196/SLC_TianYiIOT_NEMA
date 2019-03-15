#ifndef __BCXX_H
#define __BCXX_H

#include "sys.h"
#include "ringbuf.h"


#define BCXX_RST_HIGH		GPIO_SetBits(GPIOC,GPIO_Pin_2)
#define BCXX_RST_LOW		GPIO_ResetBits(GPIOC,GPIO_Pin_2)

#define BCXX_PWREN_HIGH		GPIO_SetBits(GPIOC,GPIO_Pin_3)
#define BCXX_PWREN_LOW		GPIO_ResetBits(GPIOC,GPIO_Pin_3)


#define BCXX_PRINTF_RX_BUF

#define CMD_DATA_BUFFER_SIZE 256
#define NET_DATA_BUFFER_SIZE 1500

#define TIMEOUT_1S 1000
#define TIMEOUT_2S 2000
#define TIMEOUT_5S 5000
#define TIMEOUT_6S 6000
#define TIMEOUT_7S 7000
#define TIMEOUT_10S 10000
#define TIMEOUT_11S 10000
#define TIMEOUT_12S 10000
#define TIMEOUT_13S 10000
#define TIMEOUT_14S 10000
#define TIMEOUT_15S 15000
#define TIMEOUT_20S 20000
#define TIMEOUT_25S 25000
#define TIMEOUT_30S 30000
#define TIMEOUT_35S 35000
#define TIMEOUT_40S 40000
#define TIMEOUT_45S 45000
#define TIMEOUT_50S 50000
#define TIMEOUT_55S 55000
#define TIMEOUT_60S 60000
#define TIMEOUT_65S 65000
#define TIMEOUT_70S 70000
#define TIMEOUT_75S 75000
#define TIMEOUT_80S 80000
#define TIMEOUT_85S 85000
#define TIMEOUT_90S 90000
#define TIMEOUT_95S 95000
#define TIMEOUT_100S 100000
#define TIMEOUT_105S 105000
#define TIMEOUT_110S 110000
#define TIMEOUT_115S 115000
#define TIMEOUT_120S 120000
#define TIMEOUT_125S 125000
#define TIMEOUT_130S 130000
#define TIMEOUT_135S 135000
#define TIMEOUT_140S 140000
#define TIMEOUT_145S 145000
#define TIMEOUT_150S 150000
#define TIMEOUT_155S 155000
#define TIMEOUT_160S 160000
#define TIMEOUT_165S 165000
#define TIMEOUT_170S 170000
#define TIMEOUT_175S 175000
#define TIMEOUT_180S 180000

#define IMEI_LEN	15

#define NET_NULL 0x00000000

typedef enum
{
	NEED_PLUS = 0,
	NEED_N1,
	NEED_N2,
	NEED_M,
	NEED_I,
	NEED_MAO,
	NEED_DOU,
	NEED_LEN_DATA,
	NEED_USER_DATA
} NET_DATA_STATE_E;

//接收数据的状态
typedef enum
{
    WAITING = 0,
    RECEIVING = 1,
    RECEIVED  = 2,
    TIMEOUT = 3
} CMD_STATE_E;

//当前的模式
typedef enum
{
    NET_MODE = 0,
    CMD_MODE = 1,
} BCXX_MODE_E;

//连接状态
typedef enum
{
    UNKNOW_ERROR 	= 255,	//获取连接状态失败
    GET_READY 		= 0,	//就绪状态
    NEED_CLOSE 		= 1,	//需要关闭移动场景
    NEED_WAIT 		= 2,	//需要等待
	ON_SERVER 		= 4,	//连接建立成功
} CONNECT_STATE_E;


extern CONNECT_STATE_E ConnectState;	//连接状态

extern char 			bcxx_rx_cmd_buf[CMD_DATA_BUFFER_SIZE];
extern unsigned short  	bcxx_rx_cnt;
extern pRingBuf     	bcxx_net_buf;
extern unsigned short  	bcxx_net_data_rx_cnt;
extern unsigned short  	bcxx_net_data_len;
extern unsigned char   	bcxx_break_out_wait_cmd;
extern unsigned char   	bcxx_init_ok;
extern unsigned int    	bcxx_last_time;
extern CMD_STATE_E 		bcxx_cmd_state;
extern BCXX_MODE_E 		bcxx_mode;
extern USART_TypeDef* 	bcxx_USARTx;
extern char   			*bcxx_imei;



void			bcxx_init(void);
void 			bcxx_hard_reset(void);


unsigned short 	bcxx_read(unsigned char *buf);


unsigned char	bcxx_set_AT_NRB(void);
unsigned char	bcxx_set_AT(void);
unsigned char	bcxx_set_AT_ATE(char cmd);
unsigned char	bcxx_set_AT_UART(unsigned int baud_rate);
unsigned char	bcxx_get_AT_CSQ(void);
unsigned char	bcxx_set_AT_CFUN(unsigned char cmd);
unsigned char	bcxx_set_AT_NBAND(unsigned char cmd);
unsigned char   bcxx_get_AT_CGSN(void);
unsigned char 	bcxx_set_AT_NCDP(char *addr, char *port);
unsigned char	bcxx_set_AT_CSCON(unsigned char cmd);
unsigned char	bcxx_set_AT_CEREG(unsigned char cmd);
unsigned char	bcxx_set_AT_NNMI(unsigned char cmd);
unsigned char	bcxx_set_AT_CGATT(unsigned char cmd);
unsigned char   bcxx_set_AT_QREGSWT(unsigned char cmd);
unsigned char 	bcxx_set_AT_CEDRXS(unsigned char cmd);
unsigned char 	bcxx_set_AT_CPSMS(unsigned char cmd);
unsigned char   bcxx_get_AT_CGPADDR(char **ip);
unsigned char	bcxx_set_AT_NMGS(unsigned int len,char *buf);
unsigned char	bcxx_get_AT_CCLK(char *buf);

unsigned char	bcxx_set_AT_NSCOR(char *type, char *protocol,char *port);
unsigned char	bcxx_set_AT_NSOCL(unsigned char socket);
unsigned char 	bcxx_set_AT_NSOFT(unsigned char socket, char *ip,char *port,unsigned int len,char *inbuf,char *outbuf);
unsigned char 	bcxx_set_AT_NSOCO(unsigned char socket, char *ip,char *port);
unsigned char 	bcxx_set_AT_NSOSD(unsigned char socket, unsigned int len,char *inbuf,char *outbuf);


unsigned char 	bcxx_set_AT_OPEN(char *type, char *addr, char *port);
unsigned char 	bcxx_set_AT_CLOSE(void);
unsigned char 	bcxx_set_AT_SEND(unsigned char *buffer, unsigned int len);

void        	bcxx_clear_rx_cmd_buffer(void);
int	 			bcxx_available(void);

void        	bcxx_print_rx_buf(void);
void        	bcxx_print_cmd(CMD_STATE_E cmd);
CMD_STATE_E 	bcxx_wait_cmd1(unsigned int wait_time);
CMD_STATE_E 	bcxx_wait_cmd2(const char *spacial_target, unsigned int wait_time);
unsigned char   bcxx_wait_mode(BCXX_MODE_E mode);
void 			bcxx_get_char(void);
void 			bcxx_uart_interrupt_event(void);
void 			bcxx_net_data_state_process(char c);


































#endif
