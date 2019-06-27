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

	InventrSetLightLevel(INIT_LIGHT_LEVEL);

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

		if(NeedToReset == 1)								//���յ�����������
		{
			NeedToReset = 0;
			delay_ms(3000);

			__disable_fault_irq();							//����ָ��
			NVIC_SystemReset();
		}

		delay_ms(100);
//		MAIN_Satck = uxTaskGetStackHighWaterMark(NULL);
	}
}

//��ѯʱ�����
void AutoLoopRegularTimeGroups(u8 *percent)
{
	u8 ret = 0;
	u16 gate0 = 0;
	u16 gate1 = 0;
	u16 gate2 = 0;
	u16 gate24 = 1440;	//24*60;
	u16 gate_n = 0;
	
	pRegularTime tmp_time = NULL;

	if(GetTimeOK != 0)
	{
		xSemaphoreTake(xMutex_STRATEGY, portMAX_DELAY);
		
		if(calendar.week <= 6)	//�ж��Ƿ��ǹ�����
		{
			if(RegularTimeWeekDay->next != NULL)		//�жϲ����б��Ƿ�Ϊ��
			{
				for(tmp_time = RegularTimeWeekDay->next; tmp_time != NULL; tmp_time = tmp_time->next)	//��ѵ�����б�
				{
					if(tmp_time->hour 	== calendar.hour &&
					   tmp_time->minute == calendar.min)		//�жϵ�ǰʱ���Ƿ�ͬ��������ʱ����ͬ
					{
						ret = 1;
					}
					else if(tmp_time->next != NULL)				//���������ǲ������һ��
					{
						if(tmp_time->next->hour   == calendar.hour &&
					       tmp_time->next->minute == calendar.min)		//�жϸ������Ե�next��ʱ���Ƿ��뵱ǰʱ����ͬ
						{
							tmp_time = tmp_time->next;
							
							ret = 1;
						}
						else
						{
							gate1 = tmp_time->hour * 60 + tmp_time->minute;					//�������Եķ�����
							gate2 = tmp_time->next->hour * 60 + tmp_time->next->minute;		//�������Ե�next�ķ�����
							gate_n = calendar.hour * 60 + calendar.min;						//��ǰʱ��ķ�����

							if(gate1 < gate2)												//��������ʱ������next��ʱ��
							{
								if(gate1 <= gate_n && gate_n <= gate2)						//�жϵ�ǰʱ���Ƿ�����������ʱ����м�
								{
									ret = 1;
								}
							}
							else if(gate1 > gate2)											//��������ʱ������next��ʱ��
							{
								if(gate1 <= gate_n && gate_n <= gate24)						//�жϵ�ǰʱ���Ƿ��ڸ�������ʱ���24��ʱ����м�
								{
									ret = 1;
								}
								else if(gate0 <= gate_n && gate_n <= gate2)					//�жϵ�ǰʱ���Ƿ���0���next��ʱ����м�
								{
									ret = 1;
								}
							}
						}
					}
					else
					{
						gate1 = tmp_time->hour * 60 + tmp_time->minute;					//�������Եķ�����
						gate_n = calendar.hour * 60 + calendar.min;						//��ǰʱ��ķ�����
						
						if(gate_n >= gate1)
						{
							ret = 1;
						}
					}

					if(ret == 1)
					{
						*percent = tmp_time->percent * 2;

						break;
					}
				}
			}
		}
		
		if(RegularTimeHoliday->next != NULL)
		{
			for(tmp_time = RegularTimeHoliday->next; tmp_time != NULL; tmp_time = tmp_time->next)
			{
				if(HolodayRange.year_s <= HolodayRange.year_e)				//��ʼ�����ڵ��ڽ�����
				{
					if(HolodayRange.year_s <= calendar.w_year - 2000 && 
					   calendar.w_year - 2000 <= HolodayRange.year_e)		//��ǰ�괦����ʼ�ͽ�����֮��
					{
						if(HolodayRange.month_s <= HolodayRange.month_e)
						{
							if(HolodayRange.month_s <= calendar.w_month && 
							   calendar.w_month <= HolodayRange.month_e)
							{
								if(HolodayRange.date_s <= HolodayRange.date_e)
								{
									if(HolodayRange.date_s <= calendar.w_date && 
									   calendar.w_date <= HolodayRange.date_e)
									{
										if(tmp_time->next != NULL)
										{
											if(tmp_time->next->hour   == calendar.hour &&
											   tmp_time->next->minute == calendar.min)
											{
												tmp_time = tmp_time->next;
												
												ret = 1;
											}
											else
											{
												gate1 = tmp_time->hour * 60 + tmp_time->minute;
												gate2 = tmp_time->next->hour * 60 + tmp_time->next->minute;
												gate_n = calendar.hour * 60 + calendar.min;

												if(gate1 < gate2)
												{
													if(gate1 <= gate_n && gate_n <= gate2)
													{
														ret = 1;
													}
												}
												else if(gate1 > gate2)
												{
													if(gate1 <= gate_n && gate_n <= gate24)
													{
														ret = 1;
													}
													else if(gate0 <= gate_n && gate_n <= gate2)
													{
														ret = 1;
													}
												}
											}
										}
										else
										{
											gate1 = tmp_time->hour * 60 + tmp_time->minute;					//�������Եķ�����
											gate_n = calendar.hour * 60 + calendar.min;						//��ǰʱ��ķ�����
											
											if(gate_n >= gate1)
											{
												ret = 1;
											}
										}
									}
								}
							}
						}
					}
				}

				if(ret == 1)
				{
					*percent = tmp_time->percent * 2;

					break;
				}
			}
		}
		
		xSemaphoreGive(xMutex_STRATEGY);
	}
}



































