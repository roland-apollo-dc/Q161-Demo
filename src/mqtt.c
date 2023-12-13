/*
 * mqtt.c
 *
 *  Created on: 2021Äê10ÔÂ14ÈÕ
 *      Author: vanstone
 */

#include <string.h>

#include <MQTTFreeRTOS.h>

#include "def.h"

#include <coredef.h>
#include <struct.h>
#include <poslib.h>

#include <MQTTClient.h>
#include <MQTTFreeRTOS.h>
#include <cJSON.h>

void initMqttOs(void)
{
	MQTTOS os;

	memset(&os, 0, sizeof(os));

	os.timerSet = (unsigned long(*)())TimerSet_Api;
	MQTTClientOSInit(os);
}

static void onDefaultMessageArrived(MessageData* md)
{
	MAINLOG_L1("defaultMessageArrived");
}

/*
 * Message comes in the following JSON format:
 * 1. transaction broadcasting		{"type":"TB", "msg":"Hello"}
 * 2. update notification			{"type":"UN"}
 * 3. synchronize time				{"type":"ST", "time":"20211016213200"}
 * 4. play MP3 file					{"type":"MP", "file":"2.mp3+1.mp3+3.mp3+2.mp3"}
 * 5. play amount					{"type":"TS", "amount":"12.33"}
 * Invalid messages are just ignored.
 *
 * Message should be less than 1024 bytes
*/
static void onTopicMessageArrived(MessageData* md)
{
	unsigned char buf[1024];
	unsigned char bcdtime[64],asc[12] = {0};
	char *pChar, *pDot = NULL;
	int countMP3 = 0, loc = 0, num = 0, ret,amount;
	unsigned char mp3File[10][10];

	MQTTMessage* m = md->message;

	MAINLOG_L1("message arrived");

	memcpy(buf, md->topicName->lenstring.data, md->topicName->lenstring.len);
	buf[md->topicName->lenstring.len] = 0;
	MAINLOG_L1("recv topic:%s", buf);

	memcpy(buf, m->payload, m->payloadlen);
	buf[m->payloadlen] = 0;
	MAINLOG_L1("recv message:%s", buf);

	AppPlayTip(buf);

	// Analyze the message body
	/*{
		cJSON *root;
		cJSON *obj;

		root = cJSON_Parse((char *)buf);
		if (root == NULL)
			return;

		obj = cJSON_GetObjectItem(root, "type");
		if (obj == NULL) {
			cJSON_Delete(root);
			return;
		}

		if (strcmp(obj->valuestring, "TB") == 0) {
			// Transaction Broadcast
			obj = cJSON_GetObjectItem(root, "msg");
			if (obj != NULL)
				AppPlayTip(obj->valuestring);
		}
		else if (strcmp(obj->valuestring, "UN") == 0) {
			// Update Notification
			set_tms_download_flag(1);
		}
		else if (strcmp(obj->valuestring, "ST") == 0) {
			// Synchronize Time
			struct DATE_USER date;
			struct TIME_USER time;

			obj = cJSON_GetObjectItem(root, "time");
			if(obj != NULL){

				memset(&date, 0, sizeof(date));
				memset(&time, 0, sizeof(time));
				date.year = (short)AscToLong_Api(obj->valuestring, 2);
				date.mon = (char)AscToLong_Api(obj->valuestring+2, 1);
				date.day = (char)AscToLong_Api(obj->valuestring+3, 1);
				time.hour = (char)AscToLong_Api(obj->valuestring+4, 1);
				time.min = (char)AscToLong_Api(obj->valuestring+5, 1);
				time.sec = (char)AscToLong_Api(obj->valuestring+6, 1);

				SetTime_Api(&date, &time);
				GetSysTime_Api((unsigned char*)bcdtime);
				CommPrintHex("system time", bcdtime, 8);
			}
		}
		else if (strcmp(obj->valuestring, "MP") == 0) {
			obj = cJSON_GetObjectItem(root, "file");

			memset(&mp3File[0][0],0,sizeof(mp3File));

			while(1){
				pChar = strstr(obj->valuestring + num,"+");
				if (pChar != NULL) {
					loc = pChar - obj->valuestring - num;
				}

				memcpy(&mp3File[countMP3][0], obj->valuestring+num, loc);
				MAINLOG_L1("file name: %s",mp3File[countMP3]);
				PlayMP3File(mp3File[countMP3]);
				num=num+loc+1;
				countMP3++;

				if (pChar==NULL) {
					break;
				}
			}
		}
		else if (strcmp(obj->valuestring, "TS") == 0) {
			obj = cJSON_GetObjectItem(root, "amount");
			MAINLOG_L1("obj->valuestring = %s",obj->valuestring);

			memset(bcdtime,0,sizeof(bcdtime));
			sprintf(bcdtime,"%s dollars received", obj->valuestring);
			AppPlayTip(bcdtime);

			pDot = strstr(obj->valuestring,".");
			if (pDot!=NULL) {
				num = pDot - obj->valuestring;
				memcpy(asc,obj->valuestring,num);
				memcpy(asc+num,obj->valuestring+num+1,2);
				amount = atoi(asc);
			}else {
				amount = atoi(obj->valuestring)*100;
			}

			if (Device_Type>=2) {

			}

		}
		cJSON_Delete(root);
	}*/
}

static unsigned char pSendBuf[1024];
static unsigned char pReadBuf[1024];
void mQTTMainThread(void)
{
	u8 Key;
	int ret, err;
	Network n;
	MQTTClient c;
	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

	AppPlayTip("4");

	memset(&c, 0, sizeof(MQTTClient));
	MQTTClientInit(&c, &n, 20000, pSendBuf, 1024, pReadBuf, 1024);

	c.defaultMessageHandler = onDefaultMessageArrived;

	n.mqttconnect = net_connect;
	n.mqttclose = net_close;
	n.mqttread = net_read;
	n.mqttwrite = net_write;

	n.netContext = n.mqttconnect(NULL, G_sys_param.mqtt_server, G_sys_param.mqtt_port, 10000, G_sys_param.mqtt_ssl, &err);
	if (n.netContext == NULL)
	{
		MAINLOG_L1("n.netContext == NULL");
		return;
	}

	data.willFlag = 0;
	data.MQTTVersion = 4; // 3.1.1
	data.clientID.cstring = G_sys_param.mqtt_client_id;
	data.username.cstring = "";
	data.password.cstring = "";
	data.keepAliveInterval = G_sys_param.mqtt_keepalive;
	data.cleansession = 1;

	ret = MQTTConnect(&c, &data);
	if (ret != 0) {
		MQTTDisconnect(&c);
		n.mqttclose(n.netContext);
		return;
	}

	ret = MQTTSubscribe(&c, G_sys_param.mqtt_topic, G_sys_param.mqtt_qos, onTopicMessageArrived);
	if (ret != 0) {
		MQTTDisconnect(&c);
		n.mqttclose(n.netContext);
		return;
	}

	AppPlayTip("Connected");

	while (1) {
		ret = MQTTYield(&c, 200);
		if (ret < 0)
			break;

		// TODO: Check SMS message
		Delay_Api(10);
	}

	MQTTDisconnect(&c);
	n.mqttclose(n.netContext);
}
