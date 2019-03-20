#include "task_net.h"
#include "common.h"
#include "delay.h"
#include "net_protocol.h"
#include "rtc.h"


TaskHandle_t xHandleTaskNET = NULL;
SensorMsg_S *p_tSensorMsgNet = NULL;			//用于装在传感器数据的结构体变量

u8 SendDeviceUUID_OK = 0;						//发送UUID成功标识
u8 SignalIntensity = 99;						//bg96的信号强度
char *UdpSockedId = NULL;

const NTPServerTypeDef  NTPServer[] = {			//全球可用的NTP服务器列表
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
		if(GetSysTick1s() - times_sec >= 10)			//每隔10秒钟获取一次信号强度
		{
			times_sec = GetSysTick1s();
			SignalIntensity = bcxx_get_AT_CSQ();
			
			if(ConnectState == UNKNOW_ERROR)			//连接状态错误
			{
				if((net_err_time ++) >= 18)				//3min后复位NB模块
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


//在线处理进程
s8 OnServerHandle(void)
{
	s8 ret = 0;
	s16 len = 0;
	u8 out_buf[512];

	SendSensorDataToIOTPlatform();								//向服务器定时发送传感器数据和心跳包

	len = NetDataFrameHandle(out_buf,HoldReg);					//读取并解析服务器下发的数据包

	if(len > 0)
	{
		if(len % 2 != 0)
		{
			len = len;
		}
		
		len = bcxx_set_AT_NMGS(len / 2,(char *)out_buf);
	}
	else if(len < 0)
	{
		ret = -1;												//一分钟内没有收到数据，强行关闭连接
	}

	return ret;
}

//向服务器定时发送传感器数据和心跳包
void SendSensorDataToIOTPlatform(void)
{
	static time_t times_sec = 0;
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
	else if(GetTimeOK == 0 && ConnectState == ON_SERVER)	//发送对时请求,电信卡无法使用NTP
	{
//		SyncDataTimeFormNTPServer(&GetTimeOK);
		SyncDataTimeFormBcxxModule(&GetTimeOK);
	}
	else if(SendDeviceUUID_OK == 0)
	{
		if(DeviceUUID != NULL)
		{
			if(GetSysTick1s() - times_sec >= 10)
			{
				times_sec = GetSysTick1s();
				
				send_len = PackNetData(0xA9,DeviceUUID,UU_ID_LEN - 2,send_buf);
			}
		}
	}

	if(send_len >= 1)
	{
		if(send_len % 2 != 0)
		{
			send_len = send_len;
		}
		
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

//从指定的NTP服务器获取时间
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






























