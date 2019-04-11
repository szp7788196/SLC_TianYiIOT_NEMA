#include "task_net.h"
#include "common.h"
#include "delay.h"
#include "net_protocol.h"
#include "rtc.h"


TaskHandle_t xHandleTaskNET = NULL;
SensorMsg_S *p_tSensorMsgNet = NULL;			//����װ�ڴ��������ݵĽṹ�����

CONNECT_STATE_E ConnectState = UNKNOW_STATE;
u8 SendDeviceUUID_OK = 0;						//����UUID�ɹ���ʶ
u8 SignalIntensity = 99;						//bg96���ź�ǿ��


unsigned portBASE_TYPE NET_Satck;
void vTaskNET(void *pvParameters)
{
	s8 ret = 0;
	u8 err_cnt = 0;
	time_t times_sec = 0;

	p_tSensorMsgNet = (SensorMsg_S *)mymalloc(sizeof(SensorMsg_S));

	bcxx_hard_init();
	
	RE_INIT:
	
	ConnectState = UNKNOW_STATE;
	err_cnt = 0;
	bcxx_soft_init();

	while(1)
	{
		if(GetSysTick1s() - times_sec >= 10)			//ÿ��10���ӻ�ȡһ���ź�ǿ��
		{
			times_sec = GetSysTick1s();
			SignalIntensity = bcxx_get_AT_CSQ();
			
			SyncDataTimeFormBcxxModule(3600);
			
			if(ConnectState == UNKNOW_STATE)
			{
				if((err_cnt ++) >= 30)
				{
					goto RE_INIT;
				}
			}
			else
			{
				err_cnt = 0;
			}
		}

		ret = OnServerHandle();
		
		if(ret != 1)
		{
			goto RE_INIT;
		}

		delay_ms(100);
	}
}


//���ߴ������
s8 OnServerHandle(void)
{
	s8 ret = 1;
	s16 len = 0;
	u8 out_buf[512];

	ret = SendSensorDataToIOTPlatform();							//���������ʱ���ʹ��������ݺ�������

	if(ret == 2)
	{
		return ret;
	}

	len = NetDataFrameHandle(out_buf,HoldReg);					//��ȡ�������������·������ݰ�

	if(len >= 2)
	{
		ret = bcxx_set_AT_NMGS(len / 2,(char *)out_buf);
	}

	return ret;
}

//���������ʱ���ʹ��������ݺ�������
s8 SendSensorDataToIOTPlatform(void)
{
	static time_t times_sec = 0;
	BaseType_t xResult;
	s8 ret = 1;
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
	else if(SendDeviceUUID_OK == 0 && ConnectState == ON_SERVER)
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

	if(send_len >= 2)
	{
		ret = bcxx_set_AT_NMGS(send_len / 2,(char *)send_buf);
	}
	
	return ret;
}

//��ָ����NTP��������ȡʱ��
u8 SyncDataTimeFormBcxxModule(time_t sync_cycle)
{
	u8 ret = 0;
	struct tm tm_time;
	static time_t time_s = 0;
	char buf[32];

	if((GetSysTick1s() - time_s >= sync_cycle))
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

			GetTimeOK = 1;
			
			ret = 1;
		}
	}

	return ret;
}






























