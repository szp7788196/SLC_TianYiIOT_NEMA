#include "task_net.h"
#include "common.h"
#include "delay.h"
#include "net_protocol.h"
#include "rtc.h"


TaskHandle_t xHandleTaskNET = NULL;
SensorMsg_S *p_tSensorMsgNet = NULL;			//����װ�ڴ��������ݵĽṹ�����

u8 SignalIntensity = 99;						//bg96���ź�ǿ��
char *UdpSockedId = NULL;

const NTPServerTypeDef  NTPServer[] = {			//ȫ����õ�NTP�������б�
{"120.25.115.20\0"},
{"203.107.6.88\0"},
{"182.92.12.11\0"},
{"120.25.108.11\0"},
{"202.108.6.95\0"},
{"118.24.236.43\0"},
{"118.24.195.65\0"},
{"202.118.1.81\0"},
{"202.118.1.130\0"},
{"202.112.29.82\0"},
{"202.112.31.197\0"},
{"103.18.128.60\0"},
{"158.69.48.97\0"},
{"216.218.254.202\0"},
{"208.53.158.34\0"},
{"66.228.42.59\0"},
{"103.11.143.248\0"},
{"202.73.57.107\0"},
{"128.199.134.40\0"},
{"218.186.3.36\0"},
{"211.233.40.78\0"},
{"106.247.248.106\0"},
{"106.186.122.232\0"},
{"133.100.11.8\0"},
{"129.250.35.251\0"},
{"131.188.3.220\0"},
{"131.188.3.223\0"},
{"203.114.74.17\0"},

};


unsigned portBASE_TYPE NET_Satck;
void vTaskNET(void *pvParameters)
{
	time_t times_sec = 0;
	u8 net_err_time = 0;

	p_tSensorMsgNet = (SensorMsg_S *)mymalloc(sizeof(SensorMsg_S));

	RE_INIT:
	bcxx_init();

	while(1)
	{
		if(GetSysTick1s() - times_sec >= 10)			//ÿ��10���ӻ�ȡһ���ź�ǿ��
		{
			times_sec = GetSysTick1s();
			SignalIntensity = bcxx_get_AT_CSQ();
			
			if(ConnectState == UNKNOW_ERROR)			//����״̬����
			{
				if((net_err_time ++) >= 18)				//3min��λNBģ��
				{
					net_err_time = 0;
					
					goto RE_INIT;
				}
			}
			else
			{
				net_err_time = 0;
			}
		}

		OnServerHandle();

		delay_ms(100);

//		NET_Satck = uxTaskGetStackHighWaterMark(NULL);
	}
}


//���ߴ������
s8 OnServerHandle(void)
{
	s8 ret = 0;
	s16 len = 0;
	u8 out_buf[512];

	SendSensorDataToIOTPlatform();								//���������ʱ���ʹ��������ݺ�������

	len = NetDataFrameHandle(out_buf,HoldReg);	//��ȡ�������������·������ݰ�

	if(len > 0)
	{
		len = bcxx_set_AT_NMGS(len / 2,(char *)out_buf);
	}
	else if(len < 0)
	{
		ret = -1;													//һ������û���յ����ݣ�ǿ�йر�����
	}

	return ret;
}

//���������ʱ���ʹ��������ݺ�������
void SendSensorDataToIOTPlatform(void)
{
	BaseType_t xResult;
	u8 send_len = 0;
	u8 sensor_data_len = 0;

	u8 sensor_buf[128];
	u8 send_buf[512];

	xResult = xQueueReceive(xQueue_sensor,
							(void *)p_tSensorMsgNet,
							(TickType_t)pdMS_TO_TICKS(50));
	if(xResult == pdPASS)
	{
		memset(send_buf,0,512);
		memset(sensor_buf,0,128);

		sensor_data_len = UnPackSensorData(p_tSensorMsgNet,sensor_buf);
		send_len = PackNetData(0xAA,sensor_buf,sensor_data_len,send_buf);
	}
	else if(GetTimeOK == 0 && ConnectState == ON_SERVER)	//���Ͷ�ʱ����,���ſ��޷�ʹ��NTP
	{
//		SyncDataTimeFormNTPServer(&GetTimeOK);
		SyncDataTimeFormBcxxModule(&GetTimeOK);
	}

	if(send_len >= 1)
	{
		send_len = bcxx_set_AT_NMGS(send_len / 2,(char *)send_buf);
	}
}

u8 SyncDataTimeFormNTPServer(u8 *time_flag)
{
	u8 ret = 0;
	u8 socket = 0;
	static u8 i = 0;
	char outbuf[128];
	u8 ntp_buf[64];
	char inbuf[] = "E30006EC0000000000000000494E49520000000000000000000000000000000000000000000000000000000000000000\0";

	ret = bcxx_get_AT_CGPADDR(&UdpSockedId);

	if(ret == 1)
	{
		socket = bcxx_set_AT_NSCOR("DGRAM", "17","1");
	}

	if(socket != 255)
	{
		memset(outbuf,0,128);

		ret = bcxx_set_AT_NSOFT(socket, (char *)&NTPServer[i],"123",48,inbuf,outbuf);

		bcxx_set_AT_NSOCL(socket);

		if(ret == 96)
		{
			StrToHex(ntp_buf, outbuf, 48);

			ret = NTP_TimeSync(0,ntp_buf);
		}

		if(ret != 1)
		{
			i ++;

			if(i == MAX_NTPSERVER_NUM)
			{
				i = 0;
			}
		}
		else
		{
			i = 0;
		}
	}

	if(ret == 1)
	{
		*time_flag = 1;
	}

	return ret;
}

//��ָ����NTP��������ȡʱ��
u8 SyncDataTimeFormBcxxModule(u8 *time_flag)
{
	u8 ret = 0;
	struct tm tm_time;
	time_t time_s = 0;
	char buf[32];

	if(*time_flag != 1)
	{
		memset(buf,0,32);

		if(bcxx_get_AT_CCLK(buf))
		{
			tm_time.tm_year = 2000 + (buf[0] - 0x30) * 10 + buf[1] - 0x30 - 1900;
			tm_time.tm_mon = (buf[3] - 0x30) * 10 + buf[4] - 0x30 - 1;
			tm_time.tm_mday = (buf[6] - 0x30) * 10 + buf[7] - 0x30;

			tm_time.tm_hour = (buf[9] - 0x30) * 10 + buf[10] - 0x30;
			tm_time.tm_min = (buf[12] - 0x30) * 10 + buf[13] - 0x30;
			tm_time.tm_sec = (buf[15] - 0x30) * 10 + buf[16] - 0x30;

			time_s = mktime(&tm_time);

			time_s += 28800;

			SyncTimeFromNet(time_s);

			*time_flag = 1;
			ret = 1;
		}
	}

	return ret;
}






























