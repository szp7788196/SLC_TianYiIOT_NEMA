#include "task_main.h"
#include "common.h"
#include "delay.h"
#include "usart.h"
#include "inventr.h"


TaskHandle_t xHandleTaskMAIN = NULL;

u8 MirrorLightLevelPercent = 0;
unsigned portBASE_TYPE MAIN_Satck;

void vTaskMAIN(void *pvParameters)
{
	time_t times_sec = 0;
	time_t times_sync = 0;

	times_sync = GetSysTick1s();

	InventrSetLightLevel(LightLevelPercent);				//�ϵ�Ĭ���ϴιػ�ǰ������

	while(1)
	{
		if(DeviceWorkMode == MODE_AUTO)						//ֻ�����Զ�ģʽ�²Ž��в����ж�
		{
			if(GetSysTick1s() - times_sec >= 1)
			{
				times_sec = GetSysTick1s();

				AutoLoopRegularTimeGroups(&LightLevelPercent);
			}
		}

		if(MirrorLightLevelPercent != LightLevelPercent)
		{
			MirrorLightLevelPercent = LightLevelPercent;

			InventrSetLightLevel(LightLevelPercent);
		}

		if(GetSysTick1s() - times_sync >= 3600)				//ÿ��1hͬ��һ��ʱ��
		{
			times_sync = GetSysTick1s();

			GetTimeOK = 0;
		}

		if(NeedToReset == 1)								//���յ�����������
		{
			NeedToReset = 0;
			delay_ms(1000);

			__disable_fault_irq();							//����ָ��
			NVIC_SystemReset();
		}

		delay_ms(100);
		MAIN_Satck = uxTaskGetStackHighWaterMark(NULL);
	}
}

//��ѯʱ�����
void AutoLoopRegularTimeGroups(u8 *percent)
{
	u8 i = 0;
	time_t seconds_now = 0;
	time_t seconds_24h = 86400;
	time_t seconds_00h = 0;

//	if(GetTimeOK != 0)
	{
		seconds_now = calendar.hour * 3600 + calendar.min * 60 + calendar.sec;	//��ȡ��ǰʱ�����Ӧ������

		for(i = 0; i < MAX_GROUP_NUM; i ++)
		{
			switch(RegularTimeStruct[i].type)
			{
				case TYPE_WEEKDAY:		//��һ������
					if(calendar.week >= 1 && calendar.week <= 5)		//�ж������Ƿ��ǹ�����
					{
						if(RegularTimeStruct[i].s_seconds > RegularTimeStruct[i].e_seconds)			//��ʼʱ��Ƚ���ʱ����һ��
						{
							if((RegularTimeStruct[i].s_seconds <= seconds_now && seconds_now <= seconds_24h) || \
								(seconds_00h <= seconds_now && seconds_now <= RegularTimeStruct[i].e_seconds))
							{
								*percent = RegularTimeStruct[i].percent * 2;

								i = MAX_GROUP_NUM;
							}
							else
							{
								*percent = 0;
							}
						}
						else if(RegularTimeStruct[i].s_seconds < RegularTimeStruct[i].e_seconds)	//��ʼʱ��ͽ���ʱ����ͬһ��
						{
							if(RegularTimeStruct[i].s_seconds <= seconds_now && seconds_now <= RegularTimeStruct[i].e_seconds)
							{
								*percent = RegularTimeStruct[i].percent * 2;

								i = MAX_GROUP_NUM;
							}
							else
							{
								*percent = 0;
							}
						}
					}
				break;

				case TYPE_WEEKEND:		//����������
					if(calendar.week >= 6 && calendar.week <= 7)		//�ж������Ƿ�������������
					{
						if(RegularTimeStruct[i].s_seconds > RegularTimeStruct[i].e_seconds)			//��ʼʱ��Ƚ���ʱ����һ��
						{
							if((RegularTimeStruct[i].s_seconds <= seconds_now && seconds_now <= seconds_24h) || \
								(seconds_00h <= seconds_now && seconds_now <= RegularTimeStruct[i].e_seconds))
							{
								*percent = RegularTimeStruct[i].percent * 2;

								i = MAX_GROUP_NUM;
							}
							else
							{
								*percent = 0;
							}
						}
						else if(RegularTimeStruct[i].s_seconds < RegularTimeStruct[i].e_seconds)	//��ʼʱ��ͽ���ʱ����ͬһ��
						{
							if(RegularTimeStruct[i].s_seconds <= seconds_now && seconds_now <= RegularTimeStruct[i].e_seconds)
							{
								*percent = RegularTimeStruct[i].percent * 2;

								i = MAX_GROUP_NUM;
							}
							else
							{
								*percent = 0;
							}
						}
					}
				break;

				case TYPE_HOLIDAY:		//�ڼ���
					if((RegularTimeStruct[i].s_year + 2000 <= calendar.w_year && calendar.w_year <= RegularTimeStruct[i].e_year + 2000) && \
						(RegularTimeStruct[i].s_month <= calendar.w_month && calendar.w_month <= RegularTimeStruct[i].e_month) && \
						(RegularTimeStruct[i].s_date <= calendar.w_date && calendar.w_date <= RegularTimeStruct[i].e_date))		//�ж������Ƿ��ǽڼ���ʱ��������
					{
						if(RegularTimeStruct[i].s_seconds > RegularTimeStruct[i].e_seconds)			//��ʼʱ��Ƚ���ʱ����һ��
						{
							if((RegularTimeStruct[i].s_seconds <= seconds_now && seconds_now <= seconds_24h) || \
								(seconds_00h <= seconds_now && seconds_now <= RegularTimeStruct[i].e_seconds))
							{
								*percent = RegularTimeStruct[i].percent * 2;

								i = MAX_GROUP_NUM;
							}
							else
							{
								*percent = 0;
							}
						}
						else if(RegularTimeStruct[i].s_seconds < RegularTimeStruct[i].e_seconds)	//��ʼʱ��ͽ���ʱ����ͬһ��
						{
							if(RegularTimeStruct[i].s_seconds <= seconds_now && seconds_now <= RegularTimeStruct[i].e_seconds)
							{
								*percent = RegularTimeStruct[i].percent * 2;

								i = MAX_GROUP_NUM;
							}
							else
							{
								*percent = 0;
							}
						}
					}
				break;

				default:

				break;
			}
		}
	}
}



































