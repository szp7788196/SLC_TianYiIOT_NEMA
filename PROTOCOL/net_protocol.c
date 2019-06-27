#include "net_protocol.h"
#include "rtc.h"
#include "usart.h"
#include "24cxx.h"
#include "common.h"
#include "task_net.h"
#include "error.h"
#include "utils.h"


//读取/处理网络数据
u16 time_out = 0;
s16 NetDataFrameHandle(u8 *outbuf,u8 *hold_reg)
{
	u16 i = 0;
	s16 ret = 0;
	u16 len = 0;
	u8 buf[1024];
	u8 *msg = NULL;
	char tmp[10];

	memset(buf,0,1024);
	
	ret = nbiot_udp_recv(buf,(size_t *)&len);
	
	if(ret == NBIOT_ERR_OK)
	{
		if(len != 0)
		{
			time_out = 0;
			
			msg = (u8 *)strstr((char *)buf,":");
			
			if(msg == NULL)
				return ret;

			msg = msg + 1;
			
			memset(tmp,0,10);
			
			while(*msg != ',')
				tmp[i ++] = *(msg ++);
			
			tmp[i] = '\0';
			len = nbiot_atoi(tmp,strlen(tmp)) * 2;
			
			msg = msg + 1;

			ret = (s16)NetDataAnalysis(msg,len,outbuf,hold_reg);
		}
		else
		{
			time_out ++;

			if(time_out >= 600)		//一分钟内未收到任何数据，强行关闭连接
			{
				time_out = 0;

				ret = -1;
			}
		}
	}

	return ret;
}

//网络数据帧协议解析
u16 NetDataAnalysis(u8 *buf,u16 len,u8 *outbuf,u8 *hold_reg)
{
	u16 ret = 0;

	u8 message_id = 0;
	u16 mid = 0;
	u8 temp_buf[2];

	StrToHex(&message_id, (char *)buf, 1);
	StrToHex(temp_buf, (char *)(buf + 2), 2);
	mid = (((u16)temp_buf[0]) << 8) + (u16)temp_buf[1];

	switch(message_id)
	{
		case 0xAA:			//解析响应包
			UnPackAckPacket(buf,len);
		break;

		case 0xAB:			//调光
			ret = ControlLightLevel(message_id,mid,buf,len,outbuf);
		break;

		case 0xAC:			//对时
			ret = GetTimeDateFromServer(message_id,mid,buf,len,outbuf);
		break;

		case 0xAD:			//设定策略
			ret = SetRegularTimeGroups(message_id,mid,buf,len,outbuf);
		break;

		case 0xAE:			//重启
			ret = ControlDeviceReset(message_id,mid,buf,len,outbuf);
		break;

		case 0xAF:			//修改上传间隔
			ret = SetDeviceUpLoadINCL(message_id,mid,buf,len,outbuf);
		break;

		case 0xB0:			//设置工作模式
			ret = SetDeviceWorkMode(message_id,mid,buf,len,outbuf);
		break;

		case 0xB1:			//设置电源接口
			ret = SetDevicePowerIntfc(message_id,mid,buf,len,outbuf);
		break;

		case 0xB2:			//OTA
			ret = SetUpdateFirmWareInfo(message_id,mid,buf,len,outbuf);
		break;
		
		case 0xB3:			//设定/读取UUID
			ret = Set_GetDeviceUUID(message_id,mid,buf,len,outbuf);
		break;

		default:

		break;
	}

	return ret;
}

//解析ACK包
u8 UnPackAckPacket(u8 *buf,u8 len)
{
	u8 ret = 1;

	if(len == 8)
	{
		if(MyStrstr(buf, "AAAA0001", len, 8) != 0xFFFF)
		{
			ConnectState = ON_SERVER;

			ret = 0;
		}
		else if(MyStrstr(buf, "AAAA0000", len, 8) != 0xFFFF)
		{
			SendDeviceUUID_OK = 1;
			ConnectState = ON_SERVER;
			
			ret = 0;
		}
	}

	return ret;
}

//ACK打包
u16 PackAckPacket(u8 message_id,u8 *data,u8 data_len,u8 *outbuf)
{
	u16 len = 0;

	len = PackNetData(message_id,data,data_len,outbuf);

	return len;
}

//控制灯的亮度
u16 ControlLightLevel(u8 message_id,u16 mid,u8 *buf,u8 len,u8 *outbuf)
{
	u8 out_len = 0;
	u8 level = 0;
	u8 data_buf[4] = {0,0,0,0};

	data_buf[0] = (u8)(mid >> 8);
	data_buf[1] = (u8)mid;

	if(len == 8)
	{
		StrToHex(&level, (char *)(buf + 6), 1);

		if(level <= 100)
		{
			LightLevelPercent = 2 * level;

			DeviceWorkMode = MODE_MANUAL;		//强制转换为手动模式

			memcpy(&HoldReg[LIGHT_LEVEL_ADD],&LightLevelPercent,LIGHT_LEVEL_LEN - 2);
			WriteDataFromHoldBufToEeprom(&HoldReg[LIGHT_LEVEL_ADD],LIGHT_LEVEL_ADD, LIGHT_LEVEL_LEN - 2);
		}
		else
		{
			data_buf[2] = 1;
		}
	}
	else
	{
		data_buf[2] = 2;
	}

	out_len = PackAckPacket(message_id,data_buf,4,outbuf);

	return out_len;
}

//远程重启
u16 ControlDeviceReset(u8 message_id,u16 mid,u8 *buf,u8 len,u8 *outbuf)
{
	u8 out_len = 0;
	u8 reset = 0;
	u8 data_buf[4] = {0,0,0,0};

	data_buf[0] = (u8)(mid >> 8);
	data_buf[1] = (u8)mid;

	if(len == 8)
	{
		StrToHex(&reset, (char *)(buf + 6), 1);

		if(reset == 0x01)
		{
			NeedToReset = 1;
		}
		else
		{
			data_buf[2] = 1;
		}
	}
	else
	{
		data_buf[2] = 2;
	}

	out_len = PackAckPacket(message_id,data_buf,4,outbuf);

	return out_len;
}

//设置设备数据上传时间间隔
u16 SetDeviceUpLoadINCL(u8 message_id,u16 mid,u8 *buf,u8 len,u8 *outbuf)
{
	u8 out_len = 0;
	u8 incl[2];
	u16 up_load_incl = 0;
	u8 data_buf[4] = {0,0,0,0};

	data_buf[0] = (u8)(mid >> 8);
	data_buf[1] = (u8)mid;

	if(len == 10)
	{
		StrToHex(incl, (char *)(buf + 6), 2);
		up_load_incl = (((u16)incl[0]) << 8) + (u16)incl[1];

		if(up_load_incl <= MAX_UPLOAD_INVL)
		{
			UpLoadINCL = up_load_incl;

			memcpy(&HoldReg[UPLOAD_INVL_ADD],incl,2);
			WriteDataFromHoldBufToEeprom(&HoldReg[UPLOAD_INVL_ADD],UPLOAD_INVL_ADD, UPLOAD_INVL_LEN - 2);
		}
		else
		{
			data_buf[2] = 1;
		}
	}
	else
	{
		data_buf[2] = 2;
	}

	out_len = PackAckPacket(message_id,data_buf,4,outbuf);

	return out_len;
}

//设置策略时间
u16 SetRegularTimeGroups(u8 message_id,u16 mid,u8 *buf,u8 len,u8 *outbuf)
{
	u8 out_len = 0;
	u8 group_num = 0;
	u16 i = 0;
	u8 temp_buf[32];
	u8 time_group[32];
	u16 crc16 = 0;
	u8 data_buf[4] = {0,0,0,0};

	data_buf[0] = (u8)(mid >> 8);
	data_buf[1] = (u8)mid;

	if(len == 38)
	{
		memset(temp_buf,0,32);
		memset(time_group,0,32);

		StrToHex(temp_buf, (char *)(buf + 6), 32 / 2);

		for(i = 0; i < 16; i ++)
		{
			temp_buf[i] -= 0x30;
		}

		group_num = temp_buf[0] * 10 + temp_buf[1];

		if(group_num <= MAX_GROUP_NUM)
		{
			time_group[0] = temp_buf[2];												//type
			time_group[1] = temp_buf[3] * 10 + temp_buf[4];								//year
			time_group[2] = temp_buf[5] * 10 + temp_buf[6];								//month
			time_group[3] = temp_buf[7] * 10 + temp_buf[8];								//date
			time_group[4] = temp_buf[9] * 10 + temp_buf[10];							//hour
			time_group[5] = temp_buf[11] * 10 + temp_buf[12];							//minute
			time_group[6] = temp_buf[13] * 100 + temp_buf[14] * 10 + temp_buf[15];		//percent

			if(time_group[0] != 0)
			{
				pRegularTime tmp_time = NULL;
				
				tmp_time = (pRegularTime)mymalloc(sizeof(RegularTime_S));

				tmp_time->prev = NULL;
				tmp_time->next = NULL;
				
				crc16 = CRC16(&time_group[0],7);
				time_group[7] = (u8)(crc16 >> 8);
				time_group[8] = (u8)(crc16 & 0x00FF);

				tmp_time->number	= group_num;
				tmp_time->type 		= time_group[0];
				tmp_time->year 		= time_group[1];
				tmp_time->month 	= time_group[2];
				tmp_time->date 		= time_group[3];
				tmp_time->hour 		= time_group[4];
				tmp_time->minute 	= time_group[5];
				tmp_time->percent 	= time_group[6];
				
				switch(tmp_time->type)
				{
					case TYPE_WEEKDAY:
						RegularTimeGroupAdd(TYPE_WEEKDAY,tmp_time);
					break;
					
					case TYPE_HOLIDAY_START:
						RegularTimeGroupAdd(TYPE_HOLIDAY_START,tmp_time);
					break;
					
					case TYPE_HOLIDAY_END:
						RegularTimeGroupAdd(TYPE_HOLIDAY_END,tmp_time);
					break;
					
					default:
						
					break;
				}
			}
			else
			{
				RemoveAllStrategy();
			}
			
			for(i = 0; i < TIME_RULE_LEN; i ++)
			{
				AT24CXX_WriteOneByte(TIME_RULE_ADD + group_num * TIME_RULE_LEN + i,time_group[i]);
			}
		}
	}
	else
	{
		data_buf[2] = 2;
	}

	out_len = PackAckPacket(message_id,data_buf,4,outbuf);

	return out_len;
}


//控制设备的工作模式
u16 SetDeviceWorkMode(u8 message_id,u16 mid,u8 *buf,u8 len,u8 *outbuf)
{
	u8 out_len = 0;
	u8 mode = 0;
	u8 data_buf[4] = {0,0,0,0};

	data_buf[0] = (u8)(mid >> 8);
	data_buf[1] = (u8)mid;

	if(len == 8)
	{
		StrToHex(&mode, (char *)(buf + 6), 1);

		if(mode == 0 || mode == 1)
		{
			DeviceWorkMode = mode;			//置工作模式标志

		}
		else
		{
			data_buf[2] = 1;
		}
	}
	else
	{
		data_buf[2] = 2;
	}

	out_len = PackAckPacket(message_id,data_buf,4,outbuf);

	return out_len;
}

//从服务器获取时间戳
u16 GetTimeDateFromServer(u8 message_id,u16 mid,u8 *buf,u8 len,u8 *outbuf)
{
	u8 out_len = 0;
	u16 i = 0;
	u8 temp_buf[13];
	u8 year = 0;
	u8 mon = 0;
	u8 day = 0;
	u8 hour = 0;
	u8 min = 0;
	u8 sec = 0;
	u8 data_buf[4] = {0,0,0,0};

	data_buf[0] = (u8)(mid >> 8);
	data_buf[1] = (u8)mid;

	if(len == 30)												//数据长度必须是64
	{
		memset(temp_buf,0,13);

		StrToHex(temp_buf, (char *)(buf + 6), 12);

		for(i = 0; i < 12; i ++)
		{
			temp_buf[i] -= 0x30;
		}

		year = temp_buf[0] * 10 + temp_buf[1];
		mon  = temp_buf[2] * 10 + temp_buf[3];
		day  = temp_buf[4] * 10 + temp_buf[5];
		hour = temp_buf[6] * 10 + temp_buf[7];
		min  = temp_buf[8] * 10 + temp_buf[9];
		sec  = temp_buf[10] * 10 + temp_buf[11];

		if(year >= 18 && mon <= 12 && day <= 31 && hour <= 23 && min <= 59 && sec <= 59)
		{
			RTC_Set(year + 2000,mon,day,hour,min,sec);

			GetTimeOK = 2;
		}
		else
		{
			data_buf[2] = 1;
		}
	}
	else
	{
		data_buf[2] = 2;
	}

	out_len = PackAckPacket(message_id,data_buf,4,outbuf);

	return out_len;
}

//控制设备电源控制接口方式
u16 SetDevicePowerIntfc(u8 message_id,u16 mid,u8 *buf,u8 len,u8 *outbuf)
{
	u8 out_len = 0;
	u8 intfc = 0;
	u8 data_buf[4] = {0,0,0,0};

	data_buf[0] = (u8)(mid >> 8);
	data_buf[1] = (u8)mid;

	if(len == 8)
	{
		StrToHex(&intfc, (char *)(buf + 6), 1);

		if(intfc <= 3)
		{
			PowerINTFC = intfc;			//置工作模式标志

			memcpy(&HoldReg[POWER_INTFC_ADD],&PowerINTFC,POWER_INTFC_ADD - 2);

			WriteDataFromHoldBufToEeprom(&HoldReg[POWER_INTFC_ADD],POWER_INTFC_ADD, POWER_INTFC_LEN - 2);
		}
		else
		{
			data_buf[2] = 1;
		}
	}
	else
	{
		data_buf[2] = 2;
	}

	out_len = PackAckPacket(message_id,data_buf,4,outbuf);

	return out_len;
}

//下发更新固件命令
u16 SetUpdateFirmWareInfo(u8 message_id,u16 mid,u8 *buf,u8 len,u8 *outbuf)
{
	u8 out_len = 0;
	u8 temp_buf[11];
	u8 data_buf[4] = {0,0,0,0};

	data_buf[0] = (u8)(mid >> 8);
	data_buf[1] = (u8)mid;

	if(len == 16)
	{
		memset(temp_buf,0,11);

		StrToHex(temp_buf, (char *)(buf + 6), 5);

		NewFirmWareVer    = (((u16)temp_buf[0]) << 8) + (u16)temp_buf[1];
		NewFirmWareBagNum = (((u16)temp_buf[2]) << 8) + (u16)temp_buf[3];
		LastBagByteNum    = *(temp_buf + 4);

		if(NewFirmWareBagNum == 0 || NewFirmWareBagNum > MAX_FW_BAG_NUM \
			|| NewFirmWareVer == 0 || NewFirmWareVer > MAX_FW_VER \
			|| LastBagByteNum == 0 || LastBagByteNum > MAX_FW_LAST_BAG_NUM)  //128 + 2 + 4 = 134
		{
			data_buf[1] = 1;
		}
		else
		{
			HaveNewFirmWare = 0xAA;
			if(NewFirmWareAdd == 0xAA)
			{
				NewFirmWareAdd = 0x55;
			}
			else if(NewFirmWareAdd == 0x55)
			{
				NewFirmWareAdd = 0xAA;
			}
			else
			{
				NewFirmWareAdd = 0xAA;
			}

			WriteOTAInfo(HoldReg,0);		//将数据写入EEPROM

			NeedToReset = 1;				//重新启动
		}
	}
	else
	{
		data_buf[1] = 2;
	}

	out_len = PackAckPacket(message_id,data_buf,4,outbuf);

	return out_len;
}

//设定/读取UUID
u16 Set_GetDeviceUUID(u8 message_id,u16 mid,u8 *buf,u8 len,u8 *outbuf)
{
	u8 out_len = 0;
	u8 cmd = 0;
	u8 temp_buf[20];
	u8 data_buf[20];

	data_buf[0] = (u8)(mid >> 8);
	data_buf[1] = (u8)mid;

	if(len == 42)
	{
		memset(temp_buf,0,20);
		memset(&data_buf[2],0,20);

		StrToHex(temp_buf, (char *)(buf + 6), 18);
		
		cmd = temp_buf[0];
		
		if(cmd <= 1)
		{
			if(cmd == 1)
			{
				memcpy(&HoldReg[UU_ID_ADD],&temp_buf[1],UU_ID_LEN - 2);

				GetDeviceUUID();

				WriteDataFromHoldBufToEeprom(&HoldReg[UU_ID_ADD],UU_ID_ADD, UU_ID_LEN - 2);
			}
			
			memcpy(&data_buf[3],&HoldReg[UU_ID_ADD],UU_ID_LEN - 2);
		}
		else
		{
			data_buf[2] = 2;
		}
	}
	else
	{
		data_buf[2] = 2;
	}

	out_len = PackAckPacket(message_id,data_buf,3 + UU_ID_LEN - 2,outbuf);

	return out_len;
}





























