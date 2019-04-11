#include "task_sensor.h"
#include "delay.h"
#include "sht2x.h"
#include "bh1750.h"
#include "task_net.h"
#include "common.h"
#include "inventr.h"
#include "rtc.h"
#include "usart.h"


TaskHandle_t xHandleTaskSENSOR = NULL;

SensorMsg_S *p_tSensorMsg = NULL;	//����װ�ڴ��������ݵĽṹ�����
unsigned portBASE_TYPE SENSOR_Satck;
void vTaskSENSOR(void *pvParameters)
{
	time_t times_sec_sim = 0;

	p_tSensorMsg = (SensorMsg_S *)mymalloc(sizeof(SensorMsg_S));

	while(1)
	{
		if(GetSysTick1s() - times_sec_sim >= UpLoadINCL)		//ÿ��UpLoadINCL����������������һ�δ���������
		{
			times_sec_sim = GetSysTick1s();

			InventrOutPutCurrent = InventrGetOutPutCurrent();	//��ȡ��Դ�������
			delay_ms(500);
			InventrOutPutVoltage = InventrGetOutPutVoltage();	//��ȡ��Դ�����ѹ
			
			if(ConnectState == ON_SERVER)					//�豸��ʱ������״̬
			{				
				p_tSensorMsg->out_put_current = (u16)(InventrOutPutCurrent + 0.5f);
				p_tSensorMsg->out_put_voltage = (u16)(InventrOutPutVoltage + 0.5f);
				p_tSensorMsg->signal_intensity = 113 - (SignalIntensity * 2);
				p_tSensorMsg->hour = calendar.hour;
				p_tSensorMsg->minute = calendar.min;
				p_tSensorMsg->second = calendar.sec;

				memset(p_tSensorMsg->gps,0,32);

				if(GpsInfo != NULL && strlen((char *)GpsInfo) <= 32)
				{
					memcpy(p_tSensorMsg->gps,GpsInfo,strlen((char *)GpsInfo));
				}
				else
				{
					memcpy(p_tSensorMsg->gps,"3948.0975N11632.7539E",21);
				}

				if(xQueueSend(xQueue_sensor,(void *)p_tSensorMsg,(TickType_t)10) != pdPASS)
				{
#ifdef DEBUG_LOG
					printf("send p_tSensorMsg fail 1.\r\n");
#endif
				}
			}
		}
		
		delay_ms(1000);
	}
}






































