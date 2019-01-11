#include "bcxx.h"
#include "usart.h"
#include "common.h"
#include "delay.h"
#include <string.h>
#include <stdlib.h>

CONNECT_STATE_E ConnectState = UNKNOW_ERROR;	//����״̬
unsigned char net_temp_data_rx_buf[8];

char 			bcxx_rx_cmd_buf[CMD_DATA_BUFFER_SIZE];
unsigned short  bcxx_rx_cnt;
pRingBuf     	bcxx_net_buf;
unsigned short  bcxx_net_data_rx_cnt;
unsigned short  bcxx_net_data_len;
unsigned char   bcxx_break_out_wait_cmd;
unsigned char   bcxx_init_ok;
unsigned int    bcxx_last_time;
CMD_STATE_E 	bcxx_cmd_state;
BCXX_MODE_E 	bcxx_mode;
USART_TypeDef* 	bcxx_USARTx = USART2;
char   			*bcxx_imei;


void bcxx_hard_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	BCXX_PWREN_LOW;
	BCXX_RST_LOW;
}

void bcxx_hard_reset(void)
{
	BCXX_PWREN_LOW;						//�رյ�Դ
	delay_ms(300);
	BCXX_PWREN_HIGH;					//�򿪵�Դ

	delay_ms(100);

	BCXX_RST_HIGH;						//Ӳ����λ
	delay_ms(300);
	BCXX_RST_LOW;

	bcxx_init_ok = 1;
}


void bcxx_init(void)
{
	u8 ret = 0;
	static u8 hard_inited = 0;
	u8 fail_time = 0;

	if(hard_inited == 0)
	{
		ret = RingBuf_Init(&bcxx_net_buf, NET_DATA_BUFFER_SIZE);
		if(ret == 0)
		{
			return;
		}

		bcxx_hard_init();
	}

	RE_HARD_RESET:
	bcxx_hard_reset();

	delay_ms(5000);

	bcxx_clear_rx_cmd_buffer();
	bcxx_available();
	bcxx_net_data_rx_cnt = 0;
	bcxx_net_data_len = 0;
	bcxx_break_out_wait_cmd = 0;
	bcxx_rx_cnt = 0;
	bcxx_mode = NET_MODE;
	bcxx_last_time = GetSysTick1ms();

	fail_time = 0;
	while(!bcxx_set_AT())
	{
		fail_time ++;
		if(fail_time >= 3)
		{
			goto RE_HARD_RESET;
		}
	}
	delay_ms(100);

	fail_time = 0;
	while(!bcxx_set_AT_ATE(0))
	{
		fail_time ++;
		if(fail_time >= 3)
		{
			goto RE_HARD_RESET;
		}
	}
	delay_ms(100);

	fail_time = 0;
	while(!bcxx_set_AT_CFUN(0))
	{
		fail_time ++;
		if(fail_time >= 3)
		{
			goto RE_HARD_RESET;
		}
	}
	delay_ms(100);

	fail_time = 0;
	while(!bcxx_set_AT_NBAND(Operators))
	{
		fail_time ++;
		if(fail_time >= 3)
		{
			goto RE_HARD_RESET;
		}
	}
	delay_ms(100);

	fail_time = 0;
	while(!bcxx_set_AT_NCDP((char *)ServerIP,(char *)ServerPort))
	{
		fail_time ++;
		if(fail_time >= 3)
		{
			goto RE_HARD_RESET;
		}
	}
	delay_ms(100);

	fail_time = 0;
	while(!bcxx_get_AT_CGSN())
	{
		fail_time ++;
		if(fail_time >= 1)
		{
			goto RE_HARD_RESET;
		}
	}
	delay_ms(100);

	fail_time = 0;
	while(!bcxx_set_AT_NRB())
	{
		fail_time ++;
		if(fail_time >= 1)
		{
			goto RE_HARD_RESET;
		}
	}
	delay_ms(10000);

	fail_time = 0;
	while(!bcxx_set_AT_CFUN(1))
	{
		fail_time ++;
		if(fail_time >= 3)
		{
			goto RE_HARD_RESET;
		}
	}
	delay_ms(100);

	fail_time = 0;
	while(!bcxx_set_AT_CSCON(0))
	{
		fail_time ++;
		if(fail_time >= 3)
		{
			goto RE_HARD_RESET;
		}
	}
	delay_ms(100);

	fail_time = 0;
	while(!bcxx_set_AT_CEREG(4))
	{
		fail_time ++;
		if(fail_time >= 3)
		{
			goto RE_HARD_RESET;
		}
	}
	delay_ms(100);

	fail_time = 0;
	while(!bcxx_set_AT_NNMI(1))
	{
		fail_time ++;
		if(fail_time >= 3)
		{
			goto RE_HARD_RESET;
		}
	}
	delay_ms(100);

	fail_time = 0;
	while(!bcxx_set_AT_CGATT(1))
	{
		fail_time ++;
		if(fail_time >= 3)
		{
			goto RE_HARD_RESET;
		}
	}

	delay_ms(10000);
}


unsigned char bcxx_send_string(USART_TypeDef* USARTx,unsigned char *str, unsigned short len)
{
	if(USARTx == USART1)
	{
		memcpy(Usart1TxBuf,str,len);
		Usart1SendLen = len;
	}
	else if(USARTx == USART2)
	{
		memcpy(Usart2TxBuf,str,len);
		Usart2SendLen = len;
	}
	else if(USARTx == UART4)
	{
		memcpy(Usart4TxBuf,str,len);
		Usart4SendLen = len;
	}
	else
	{
		return 0;
	}

	USART_ITConfig(USARTx, USART_IT_TC, ENABLE);

	return 1;
}



unsigned short bcxx_read(unsigned char *buf)
{
    int i = 0;
    unsigned short len = bcxx_available();
    if(len > 0)
    {
        for(i = 0; i < len; i++)
        {
            buf[i] = bcxx_net_buf->read(&bcxx_net_buf);
        }
    }
    else
    {
        len = 0;
    }
    return len;
}

unsigned char bcxx_set_AT_NRB(void)
{
	unsigned char ret = 0;
    bcxx_wait_mode(CMD_MODE);
    bcxx_clear_rx_cmd_buffer();
	printf("AT+NRB\r\n");
    if(bcxx_wait_cmd2("REBOOTING",TIMEOUT_10S) == RECEIVED)
    {
        if(search_str((unsigned char *)bcxx_rx_cmd_buf, "REBOOTING") != -1)
            ret = 1;
        else
            ret = 0;
    }
    bcxx_mode = NET_MODE;
#ifdef BCXX_PRINTF_RX_BUF
	bcxx_print_rx_buf();
#endif
    return ret;
}


//����ATָ�����
unsigned char bcxx_set_AT()
{
	unsigned char ret = 0;
    bcxx_wait_mode(CMD_MODE);
    bcxx_clear_rx_cmd_buffer();
	printf("AT\r\n");
    if(bcxx_wait_cmd2("OK",TIMEOUT_1S) == RECEIVED)
    {
        if(search_str((unsigned char *)bcxx_rx_cmd_buf, "OK") != -1)
            ret = 1;
        else
            ret = 0;
    }
    bcxx_mode = NET_MODE;
#ifdef BCXX_PRINTF_RX_BUF
	bcxx_print_rx_buf();
#endif
    return ret;
}

//���û��Թ���
unsigned char bcxx_set_AT_ATE(char cmd)
{
	unsigned char ret = 0;
    bcxx_wait_mode(CMD_MODE);
    bcxx_clear_rx_cmd_buffer();
	printf("ATE%d\r\n", cmd);
    if(bcxx_wait_cmd2("OK",TIMEOUT_10S) == RECEIVED)
    {
        if(search_str((unsigned char *)bcxx_rx_cmd_buf, "OK") != -1)
            ret = 1;
        else
            ret = 0;
    }
    bcxx_mode = NET_MODE;
#ifdef BCXX_PRINTF_RX_BUF
	bcxx_print_rx_buf();
#endif
    return ret;

}

unsigned char bcxx_set_AT_UART(unsigned int baud_rate)
{
	unsigned char ret = 0;


    return ret;
}

//��ѯ�ź�ǿ��
unsigned char bcxx_get_AT_CSQ(void)
{
	unsigned char ret = 0;
	unsigned short pos = 0;

    bcxx_wait_mode(CMD_MODE);
    bcxx_clear_rx_cmd_buffer();
	printf("AT+CSQ\r\n");
    if(bcxx_wait_cmd2("+CSQ:",TIMEOUT_1S) == RECEIVED)
    {
        if(search_str((unsigned char *)bcxx_rx_cmd_buf, "+CSQ:") != -1)
		{
			pos = MyStrstr((u8 *)bcxx_rx_cmd_buf, "+CSQ:", 128, 5);
			if(pos != 0xFFFF)
			{
				if(bcxx_rx_cmd_buf[pos + 6] != ',')
				{
					ret = (bcxx_rx_cmd_buf[pos + 5] - 0x30) * 10 +\
						bcxx_rx_cmd_buf[pos + 6] - 0x30;
				}
				else
				{
					ret = bcxx_rx_cmd_buf[pos + 5] - 0x30;
				}

				if(ret == 0 && ret == 99)
				{
					ret = 0;
				}
			}
		}
    }
    bcxx_mode = NET_MODE;
#ifdef BCXX_PRINTF_RX_BUF
	bcxx_print_rx_buf();
#endif
    return ret;
}

unsigned char bcxx_set_AT_CFUN(unsigned char cmd)
{
	unsigned char ret = 0;
    bcxx_wait_mode(CMD_MODE);
    bcxx_clear_rx_cmd_buffer();
	printf("AT+CFUN=%d\r\n",cmd);
    if(bcxx_wait_cmd1(TIMEOUT_90S) == RECEIVED)
    {
        if(search_str((unsigned char *)bcxx_rx_cmd_buf, "OK") != -1)
            ret = 1;
        else
            ret = 0;
    }
    bcxx_mode = NET_MODE;
#ifdef BCXX_PRINTF_RX_BUF
	bcxx_print_rx_buf();
#endif
    return ret;
}

//����Ƶ��
unsigned char bcxx_set_AT_NBAND(unsigned char operators)
{
	unsigned char ret = 0;
	unsigned char band = 8;
	
	switch(operators)
	{
		case 0:				//�ƶ�
			band = 8;
		break;
		
		case 1:				//��ͨ
			band = 8;
		break;
		
		case 2:				//����
			band = 5;
		break;
		
		default:
			band = 8;
		break;
	}
	
    bcxx_wait_mode(CMD_MODE);
    bcxx_clear_rx_cmd_buffer();
	printf("AT+NBAND=%d\r\n",band);
    if(bcxx_wait_cmd2("OK",TIMEOUT_1S) == RECEIVED)
    {
        if(search_str((unsigned char *)bcxx_rx_cmd_buf, "OK") != -1)
            ret = 1;
        else
            ret = 0;
    }
    bcxx_mode = NET_MODE;
#ifdef BCXX_PRINTF_RX_BUF
	bcxx_print_rx_buf();
#endif
    return ret;
}

//��ȡIMEI��
unsigned char bcxx_get_AT_CGSN(void)
{
	unsigned char ret = 0;
	char buf[32];

    bcxx_wait_mode(CMD_MODE);
    bcxx_clear_rx_cmd_buffer();
    printf("AT+CGSN=1\r\n");
    if(bcxx_wait_cmd1(TIMEOUT_1S) == RECEIVED)
    {
        if(search_str((unsigned char *)bcxx_rx_cmd_buf, "OK") != -1)
		{
			memset(buf,0,32);

			get_str1((unsigned char *)bcxx_rx_cmd_buf, "CGSN:", 1, "\r\n", 2, (unsigned char *)buf);

			if(strlen(buf) == 15)
			{
				if(bcxx_imei == NULL)
				{
					bcxx_imei = (char *)mymalloc(sizeof(char) * 16);
				}
				if(bcxx_imei != NULL)
				{
					memset(bcxx_imei,0,16);
					memcpy(bcxx_imei,buf,15);

					ret = 1;
				}
			}
		}
    }
    bcxx_mode = NET_MODE;
#ifdef BCXX_PRINTF_RX_BUF
	bcxx_print_rx_buf();
#endif
    return ret;
}

//����IOTƽ̨IP�Ͷ˿�
unsigned char bcxx_set_AT_NCDP(char *addr, char *port)
{
	unsigned char ret = 0;
    bcxx_wait_mode(CMD_MODE);
    bcxx_clear_rx_cmd_buffer();
	printf("AT+NCDP=%s,%s\r\n",addr,port);
    if(bcxx_wait_cmd2("OK",TIMEOUT_1S) == RECEIVED)
    {
        if(search_str((unsigned char *)bcxx_rx_cmd_buf, "OK") != -1)
            ret = 1;
        else
            ret = 0;
    }
    bcxx_mode = NET_MODE;
#ifdef BCXX_PRINTF_RX_BUF
	bcxx_print_rx_buf();
#endif
    return ret;
}

//��������״̬�Զ�����
unsigned char bcxx_set_AT_CSCON(unsigned char cmd)
{
	unsigned char ret = 0;
    bcxx_wait_mode(CMD_MODE);
    bcxx_clear_rx_cmd_buffer();
	printf("AT+CSCON=%d\r\n",cmd);
    if(bcxx_wait_cmd2("OK",TIMEOUT_1S) == RECEIVED)
    {
        if(search_str((unsigned char *)bcxx_rx_cmd_buf, "OK") != -1)
            ret = 1;
        else
            ret = 0;
    }
    bcxx_mode = NET_MODE;
#ifdef BCXX_PRINTF_RX_BUF
	bcxx_print_rx_buf();
#endif
    return ret;
}

//EPS Network Registration Status
unsigned char bcxx_set_AT_CEREG(unsigned char cmd)
{
	unsigned char ret = 0;
    bcxx_wait_mode(CMD_MODE);
    bcxx_clear_rx_cmd_buffer();
	printf("AT+CEREG=%d\r\n",cmd);
    if(bcxx_wait_cmd2("OK",TIMEOUT_1S) == RECEIVED)
    {
        if(search_str((unsigned char *)bcxx_rx_cmd_buf, "OK") != -1)
            ret = 1;
        else
            ret = 0;
    }
    bcxx_mode = NET_MODE;
#ifdef BCXX_PRINTF_RX_BUF
	bcxx_print_rx_buf();
#endif
    return ret;
}

//�����������ݽ���ģʽ
unsigned char bcxx_set_AT_NNMI(unsigned char cmd)
{
	unsigned char ret = 0;
    bcxx_wait_mode(CMD_MODE);
    bcxx_clear_rx_cmd_buffer();
	printf("AT+NNMI=%d\r\n",cmd);
    if(bcxx_wait_cmd2("OK",TIMEOUT_1S) == RECEIVED)
    {
        if(search_str((unsigned char *)bcxx_rx_cmd_buf, "OK") != -1)
            ret = 1;
        else
            ret = 0;
    }
    bcxx_mode = NET_MODE;
#ifdef BCXX_PRINTF_RX_BUF
	bcxx_print_rx_buf();
#endif
    return ret;
}

unsigned char bcxx_set_AT_CGATT(unsigned char cmd)
{
	unsigned char ret = 0;
    bcxx_wait_mode(CMD_MODE);
    bcxx_clear_rx_cmd_buffer();
	printf("AT+CGATT=%d\r\n",cmd);
    if(bcxx_wait_cmd2("OK",TIMEOUT_2S) == RECEIVED)
    {
        if(search_str((unsigned char *)bcxx_rx_cmd_buf, "OK") != -1)
            ret = 1;
        else
            ret = 0;
    }
    bcxx_mode = NET_MODE;
#ifdef BCXX_PRINTF_RX_BUF
	bcxx_print_rx_buf();
#endif
    return ret;
}

//��IOTƽ̨��������
unsigned char bcxx_set_AT_NMGS(unsigned int len,char *buf)
{
	unsigned char ret = 0;
    bcxx_wait_mode(CMD_MODE);
    bcxx_clear_rx_cmd_buffer();
	printf("AT+NMGS=%d,%s\r\n",len,buf);
    if(bcxx_wait_cmd1(TIMEOUT_1S) == RECEIVED)
    {
        if(search_str((unsigned char *)bcxx_rx_cmd_buf, "OK") != -1)
		{
			ret = 1;
		}
        else
		{
			ConnectState = UNKNOW_ERROR;

			ret = 0;
		}
    }
    bcxx_mode = NET_MODE;
#ifdef BCXX_PRINTF_RX_BUF
	bcxx_print_rx_buf();
#endif
    return ret;
}

//��ȡ����IP��ַ
unsigned char bcxx_get_AT_CGPADDR(char **ip)
{
	unsigned char ret = 0;
	unsigned char len = 0;
	unsigned char new_len = 0;
	unsigned char msg[20];
    bcxx_wait_mode(CMD_MODE);
    bcxx_clear_rx_cmd_buffer();
	printf("AT+CGPADDR\r\n");
    if(bcxx_wait_cmd2("OK",TIMEOUT_1S) == RECEIVED)
    {
        if(search_str((unsigned char *)bcxx_rx_cmd_buf, "OK") != -1)
		{
			memset(msg,0,20);

			get_str1((unsigned char *)bcxx_rx_cmd_buf, "+CGPADDR:0,", 1, "\r\n", 2, (unsigned char *)msg);

			new_len = strlen((char *)msg);

			if(new_len != 0)
			{
				if(*ip == NULL)
				{
					*ip = (char *)mymalloc(sizeof(u8) * len + 1);
				}

				if(*ip != NULL)
				{
					len = strlen((char *)*ip);

					if(len == new_len)
					{
						memset(*ip,0,new_len + 1);
						memcpy(*ip,msg,new_len);
						ret = 1;
					}
					else
					{
						myfree(*ip);
						*ip = (char *)mymalloc(sizeof(u8) * new_len + 1);
						if(ip != NULL)
						{
							memset(*ip,0,new_len + 1);
							memcpy(*ip,msg,new_len);
							len = new_len;
							new_len = 0;
							ret = 1;
						}
					}
				}
			}
		}
    }
    bcxx_mode = NET_MODE;
#ifdef BCXX_PRINTF_RX_BUF
	bcxx_print_rx_buf();
#endif
    return ret;
}

//�½�һ��SOCKET
unsigned char bcxx_set_AT_NSCOR(char *type, char *protocol,char *port)
{
	unsigned char ret = 255;
	unsigned char buf[3] = {0,0,0};
    bcxx_wait_mode(CMD_MODE);
    bcxx_clear_rx_cmd_buffer();
	printf("AT+NSOCR=%s,%s,%s,1\r\n",type,protocol,port);
    if(bcxx_wait_cmd2("OK",TIMEOUT_1S) == RECEIVED)
    {
        if(search_str((unsigned char *)bcxx_rx_cmd_buf, "OK") != -1)
		{
			get_str1((unsigned char *)bcxx_rx_cmd_buf, "\r\n", 1, "\r\n", 2, (unsigned char *)buf);
			if(strlen((char *)buf) == 1)
			{
				ret = buf[0] - 0x30;
			}
		}
    }
    bcxx_mode = NET_MODE;
#ifdef BCXX_PRINTF_RX_BUF
	bcxx_print_rx_buf();
#endif
    return ret;
}

//�ر�һ��SOCKET
unsigned char bcxx_set_AT_NSOCL(unsigned char socket)
{
	unsigned char ret = 0;
    bcxx_wait_mode(CMD_MODE);
    bcxx_clear_rx_cmd_buffer();
	printf("AT+NSOCL=%d\r\n",socket);
    if(bcxx_wait_cmd2("OK",TIMEOUT_1S) == RECEIVED)
    {
        if(search_str((unsigned char *)bcxx_rx_cmd_buf, "OK") != -1)
		{
			ret = 1;
		}
    }
    bcxx_mode = NET_MODE;
#ifdef BCXX_PRINTF_RX_BUF
	bcxx_print_rx_buf();
#endif
    return ret;
}

//��UDP�������������ݲ��ȴ���Ӧ����
unsigned char bcxx_set_AT_NSOFT(unsigned char socket, char *ip,char *port,unsigned int len,char *inbuf,char *outbuf)
{
	unsigned char ret = 0;
    bcxx_wait_mode(CMD_MODE);
    bcxx_clear_rx_cmd_buffer();
	printf("AT+NSOST=%d,%s,%s,%d,%s\r\n",socket,ip,port,len,inbuf);
    if(bcxx_wait_cmd2("OK",TIMEOUT_1S) == RECEIVED)
    {
        if(search_str((unsigned char *)bcxx_rx_cmd_buf, "OK") != -1)
		{
			bcxx_clear_rx_cmd_buffer();
			if(bcxx_wait_cmd2("+NSONMI:",TIMEOUT_15S) == RECEIVED)
			{
				bcxx_clear_rx_cmd_buffer();
				printf("AT+NSORF=%d,%d\r\n",socket,1358);
				if(bcxx_wait_cmd2("OK",TIMEOUT_2S) == RECEIVED)
				{
					get_str1((unsigned char *)bcxx_rx_cmd_buf, ",", 4, ",", 5, (unsigned char *)outbuf);

					ret = strlen((char *)outbuf);
				}
			}
		}
    }
    bcxx_mode = NET_MODE;
#ifdef BCXX_PRINTF_RX_BUF
	bcxx_print_rx_buf();
#endif
    return ret;
}

//��ģ���ȡʱ��
unsigned char bcxx_get_AT_CCLK(char *buf)
{
	unsigned char ret = 0;
    bcxx_wait_mode(CMD_MODE);
    bcxx_clear_rx_cmd_buffer();
	printf("AT+CCLK?\r\n");
    if(bcxx_wait_cmd2("OK",TIMEOUT_1S) == RECEIVED)
    {
        if(search_str((unsigned char *)bcxx_rx_cmd_buf, "+CCLK:") != -1)
		{
			get_str1((unsigned char *)bcxx_rx_cmd_buf, "+CCLK:", 1, "\r\n\r\nOK", 1, (unsigned char *)buf);

			ret = 1;
		}
    }
    bcxx_mode = NET_MODE;
#ifdef BCXX_PRINTF_RX_BUF
	bcxx_print_rx_buf();
#endif
    return ret;
}


//��Զ�˷���������������
unsigned char bcxx_set_AT_OPEN(char *type, char *addr, char *port)
{
	unsigned char ret = 0;

    return ret;
}

//�رպ�Զ�˷������ĵ�����
unsigned char bcxx_set_AT_CLOSE(void)
{
	unsigned char ret = 0;

    return ret;
}

//�ڵ�����ģʽ�·�������
unsigned char bcxx_set_AT_SEND(unsigned char *buffer, unsigned int len)
{
	unsigned char ret = 0;

    return ret;
}


//���ATָ����ջ���
void bcxx_clear_rx_cmd_buffer(void)
{
	uint16_t i;
    for(i = 0; i < CMD_DATA_BUFFER_SIZE; i++)
    {
        bcxx_rx_cmd_buf[i] = 0;
    }
    bcxx_rx_cnt = 0;
}

//����������ݻ���
int bcxx_available(void)
{
	return bcxx_net_buf->available(&bcxx_net_buf);
}

void bcxx_print_rx_buf(void)
{
	UsartSendString(USART1,(unsigned char *)bcxx_rx_cmd_buf,bcxx_rx_cnt);
}

void bcxx_print_cmd(CMD_STATE_E cmd)
{

}

CMD_STATE_E bcxx_wait_cmd1(unsigned int wait_time)
{
	unsigned int time = GetSysTick1ms();
	unsigned int time_now = 0;
    bcxx_cmd_state = WAITING;

    while(1)
    {
		time_now = GetSysTick1ms();
        if((time_now - time) > wait_time)
        {
            bcxx_cmd_state = TIMEOUT;
            break;
        }

        if(
            search_str((unsigned char *)bcxx_rx_cmd_buf, "OK"   ) != -1  || \
            search_str((unsigned char *)bcxx_rx_cmd_buf, "FAIL" ) != -1  || \
            search_str((unsigned char *)bcxx_rx_cmd_buf, "ERROR") != -1
        )
        {
            while(GetSysTick1ms() - bcxx_last_time < 20);
				bcxx_cmd_state = RECEIVED;
			break;
        }
		delay_ms(10);
    }
    bcxx_print_cmd(bcxx_cmd_state);

    return bcxx_cmd_state;
}

CMD_STATE_E bcxx_wait_cmd2(const char *spacial_target, unsigned int wait_time)
{
	unsigned int time = GetSysTick1ms();
	unsigned int time_now = 0;
	bcxx_cmd_state = WAITING;

    while(1)
    {
		time_now = GetSysTick1ms();
        if((time_now - time) > wait_time)
        {
            bcxx_cmd_state = TIMEOUT;
            break;
        }

        else if(search_str((unsigned char *)bcxx_rx_cmd_buf, (unsigned char *)spacial_target) != -1)
        {
            while(GetSysTick1ms() - bcxx_last_time < 20);
				bcxx_cmd_state = RECEIVED;
            break;
        }
		else if(bcxx_break_out_wait_cmd == 1)
		{
			bcxx_break_out_wait_cmd = 0;
			break;
		}
		delay_ms(10);
    }
    bcxx_print_cmd(bcxx_cmd_state);

    return bcxx_cmd_state;
}

unsigned char bcxx_wait_mode(BCXX_MODE_E mode)
{
	unsigned char ret = 0;

	ret = 1;
	if(GetSysTick1ms() - bcxx_last_time > 20)
	{
		bcxx_mode = mode;
	}
    else
    {
        while(GetSysTick1ms() - bcxx_last_time < 20);
        bcxx_mode = mode;
    }
    return ret;
}

//�Ӵ��ڻ�ȡһ���ַ�
void bcxx_get_char(void)
{
	unsigned char c;

	c = USART_ReceiveData(bcxx_USARTx);
	bcxx_last_time = GetSysTick1ms();

	if(bcxx_mode == CMD_MODE)
	{
		bcxx_rx_cmd_buf[bcxx_rx_cnt] = c;
		if(bcxx_rx_cnt ++ > CMD_DATA_BUFFER_SIZE)
		{
			bcxx_rx_cnt = 0;
		}
		bcxx_cmd_state = RECEIVING;
	}
	bcxx_net_data_state_process(c);
}

void bcxx_uart_interrupt_event(void)
{
	bcxx_get_char();
}


void bcxx_net_data_state_process(char c)
{
	static NET_DATA_STATE_E net_data_state = NEED_I;

	switch((unsigned char)net_data_state)
	{
		case (unsigned char)NEED_PLUS:
			if(c == '+')
			{
				net_data_state  = NEED_N1;
			}
			else
			{
				net_data_state = NEED_PLUS;
			}
		break;

		case (unsigned char)NEED_N1:
			if(c == 'N')
			{
				net_data_state = NEED_N2;
			}
			else
			{
				net_data_state = NEED_PLUS;
			}
		break;

		case (unsigned char)NEED_N2:
			if(c == 'N')
			{
				net_data_state = NEED_M;
			}
			else
			{
				net_data_state = NEED_PLUS;
			}
		break;

		case (unsigned char)NEED_M:
			if(c == 'M')
			{
				net_data_state = NEED_I;
			}
			else
			{
				net_data_state = NEED_PLUS;
			}
		break;

		case (unsigned char)NEED_I:
			if(c == 'I')
			{
				net_data_state = NEED_MAO;
			}
			else
			{
				net_data_state = NEED_PLUS;
			}
		break;

		case (unsigned char)NEED_MAO:
			if(c == ':')
			{
				net_data_state = NEED_LEN_DATA;
			}
			else
			{
				net_data_state = NEED_PLUS;
			}
		break;

		case (unsigned char)NEED_LEN_DATA:
			if(c >= '0' && c <= '9')
			{
				net_temp_data_rx_buf[bcxx_net_data_rx_cnt++] = c;
			}
			else if(c == ',')
			{
				net_temp_data_rx_buf[bcxx_net_data_rx_cnt++] = '\0';
				bcxx_net_data_len = atoi((const char *)net_temp_data_rx_buf);
				bcxx_net_data_len *= 2;
				net_data_state = NEED_USER_DATA;
				bcxx_net_data_rx_cnt = 0;
			}
			else
			{
				bcxx_net_data_rx_cnt = 0;
				net_data_state = NEED_PLUS;
			}
		break;

		case (unsigned char)NEED_USER_DATA:
			if(bcxx_net_data_rx_cnt < bcxx_net_data_len)
			{
				bcxx_net_buf->write(&bcxx_net_buf,c);
				bcxx_net_data_rx_cnt++;
			}
			else
			{
				bcxx_net_data_rx_cnt = 0;
				net_data_state = NEED_PLUS;
			}
		break;
		default:
			net_data_state = NEED_PLUS;
			bcxx_net_data_rx_cnt = 0;
		break;
	}
}







































