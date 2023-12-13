/*
 * param.c
 *
 *  Created on: 2021Äê10ÔÂ14ÈÕ
 *      Author: vanstone
 */
#include <string.h>

#include "def.h"

#include <coredef.h>
#include <struct.h>
#include <poslib.h>

SYS_PARAM G_sys_param;
int Device_Type;

static void readSN(char *sn)
{
	int ret;
	unsigned char buf[40];

	ret = PEDReadPinPadSn_Api(buf);
	if(ret != 0)
	{
		// Serial number lost, warning and poweroff.
		AppPlayTip("Serial number lost, power off now");

		WaitAnyKey_Api(10);
		SysPowerOff_Api();

		while (1);
	}

	ret = AscToLong_Api(buf, 2);
	buf[ret + 2] = 0;

	strcpy(sn, (char *)(buf + 2));
}

void initParam(void){
	int ret;
	unsigned int len;

	ret = GetFileSize_Api(SYS_PARAM_NAME);
	if (ret == sizeof(SYS_PARAM)) {
		len = ret;
		ret = ReadFile_Api(SYS_PARAM_NAME, (unsigned char *)&G_sys_param, 0, &len);
		if ((ret == 0) && (len == sizeof(SYS_PARAM))) {
			readSN(G_sys_param.sn);

			return;
		}
	}

	// Create default param
	memset(&G_sys_param, 0, sizeof(SYS_PARAM));

	readSN(G_sys_param.sn);

	strcpy(G_sys_param.mqtt_server, "broker.hivemq.com");
	strcpy(G_sys_param.mqtt_port, "1883");
	G_sys_param.mqtt_ssl = 0;
	sprintf(G_sys_param.mqtt_topic, "topic-%s", G_sys_param.sn);
	sprintf(G_sys_param.mqtt_client_id, "clientId-%s", G_sys_param.sn);

	MAINLOG_L1("G_sys_param.mqtt_topic:%s", G_sys_param.mqtt_topic);
	MAINLOG_L1("G_sys_param.mqtt_client_id:%s", G_sys_param.mqtt_client_id);
	G_sys_param.mqtt_qos = 0;
	G_sys_param.mqtt_keepalive = 60;

	G_sys_param.sound_level = 1;

	saveParam();
}

void saveParam(void)
{
//	SaveWholeFile_Api(SYS_PARAM_NAME, (unsigned char *)&G_sys_param, sizeof(SYS_PARAM));
}

void initDeviceType(){
	char machineType[26];
	int Result = 0,ret;

	memset(machineType,0,sizeof(machineType));
	Result = sysGetTermType_lib(&machineType);
	if (Result>=0)
	{
		MAINLOG_L1("device type : %s", machineType);

		ret = memcmp(machineType,"Q180D",5);
		if (ret==0)
		{
			MAINLOG_L1("define Q180D");
			Device_Type = 2;
		}

		ret = memcmp(machineType,"Q190",4);
		if (ret==0)
		{
			MAINLOG_L1("define Q190");
			Device_Type = 3;
			QRCodeDisp();
		}
	}

}

