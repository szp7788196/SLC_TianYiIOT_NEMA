#ifndef __TASK_NET_H
#define __TASK_NET_H

#include "sys.h"
#include "rtos_task.h"
#include "bcxx.h"
#include "task_sensor.h"


#define MAX_NTPSERVER_NUM	28

typedef struct 
{
	char ip_adderss[16];
}NTPServerTypeDef;


extern TaskHandle_t xHandleTaskNET;
extern SensorMsg_S *p_tSensorMsgNet;

extern u8 SignalIntensity;				//bg96的信号强度


void vTaskNET(void *pvParameters);
s8 OnServerHandle(void);
void SendSensorDataToIOTPlatform(void);
u8 SyncDataTimeFormNTPServer(u8 *time_flag);































#endif
