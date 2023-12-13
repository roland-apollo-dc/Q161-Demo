/*
 * network.c
 *
 *  Created on: 2021Äê10ÔÂ14ÈÕ
 *      Author: vanstone
 */
#include <stdio.h>
#include <string.h>

#include "def.h"
#include "httpDownload.h"

#include <coredef.h>
#include <struct.h>
#include <poslib.h>

//int wirelessPdpOpen_lib(void);
int wirelessPppOpen_lib(unsigned char *pucApn, unsigned char *pucUserName, unsigned char *pucPassword) ;
int wirelessCheckPppDial_lib(void); //int wirelessCheckPdpDial_lib(int timeout);
int wirelessPppClose_lib(void) ;//int wirelessPdpRelease_lib(void);
int wirelessSocketCreate_lib(int nProtocol);
int wirelessSocketClose_lib(int sockid);
int wirelessTcpConnect_lib(int sockid, char *pucIP, char *pucPort, int timeout);//Q161
//int wirelessTcpConnect_lib(int sockid, char *pucIP, unsigned int uiPort); //Q181
int wirelessSend_lib(int sockid, unsigned char *pucdata, unsigned int iLen);
int wirelessRecv_lib(int sockid, unsigned char *pucdata, unsigned int iLen, unsigned int uiTimeOut);

//int wirelessSslSetTlsVer_lib(int ver);
int wirelessSetSslVer_lib(unsigned char ucVer) ;
void wirelessSslDefault_lib(void);
int wirelessSendSslFile_lib (unsigned char ucType, unsigned char *pucData, int iLen);
int wirelessSetSslMode_lib(unsigned char  ucMode);
int wirelessSslSocketCreate_lib(void);
int wirelessSslSocketClose_lib(int sockid);
//timeout: ms   return :0-success <0-failed
int wirelessSslConnect_lib(int sockid, unsigned char *pucDestIP, char *pucPort, int timeout);  //for Q161
//int wirelessSslConnect_lib(int sockid, char *pucDestIP, unsigned short pucDestPort); //for Q181
int wirelessSslSend_lib(int sockid, unsigned char *pucdata, unsigned int iLen);
int wirelessSslRecv_lib(int sockid, unsigned char *pucdata, unsigned int iLen, unsigned int uiTimeOut);


typedef struct {
	int valid;
	int socket;
	int ssl;
} NET_SOCKET_PRI;

// FIXME: What if the caller doesn't close the socket??
#define	MAX_SOCKETS		10
#define CERTI_LEN  1024*2

static NET_SOCKET_PRI sockets[MAX_SOCKETS];

void net_init(void)
{
	int i;

	for (i = 0; i < MAX_SOCKETS; i++)
		memset(&sockets[i], 0, sizeof(NET_SOCKET_PRI));
}

int getCertificate(char *cerName, unsigned char *cer){
	int Cerlen, Ret;
	u8 CerBuf[CERTI_LEN];

	Cerlen = GetFileSize_Api(cerName);
	if(Cerlen <= 0)
	{
		MAINLOG_L1("get certificate err or not exist - %s",cerName);
		return -1;
	}

	memset(CerBuf, 0 , sizeof(CerBuf));
	if(Cerlen > CERTI_LEN)
	{
		MAINLOG_L1("%s is too large", cerName);
		return -2;
	}

	Ret = ReadFile_Api(cerName, CerBuf, 0, (unsigned int *)&Cerlen);
	if(Ret != 0)
	{
		MAINLOG_L1("read %s failed", cerName);
		return -3;
	}
	memcpy(cer,CerBuf,strlen(CerBuf));

	return 0;
}


void *net_connect(void* attch, const char *host,const char *port, int timerOutMs, int ssl, int *errCode)
{
	int ret;
	int port_int;
	int sock;
	int i;
	u8 CerBuf[CERTI_LEN];
	int timeid = 0;
	
	timeid = TimerSet_Api();
	while(1)
	{
		ret = wirelessCheckPppDial_lib();
		MAINLOG_L1("wirelessCheckPppDial_lib = %d" , ret );
		if(ret == 0)
			break;
		else{
			ret = wirelessPppOpen_lib(NULL, NULL, NULL);
			Delay_Api(1000);
		}
		if(TimerCheck_Api(timeid , timerOutMs) == 1)
		{
			MAINLOG_L1("wirelessCheckPppDial_lib = %d" , ret);
			*errCode = -1;
			return NULL;
		}
	}
	//ret =  wirelessSetDNS_lib("114.114.114.114", "8.8.8.8" );
	//MAINLOG_L1("wirelessSetDNS_lib = %d" , ret);

	// Find an empty socket slot
	for (i = 0; i < MAX_SOCKETS; i++) {
		if (sockets[i].valid == 0)
			break;
	}

	if (i >= MAX_SOCKETS) {
		*errCode = -3;

		return NULL;
	}

	if (ssl == 0) {
		sock = wirelessSocketCreate_lib(0);
		if (sock < 0) {
			*errCode = -4;

			return NULL;
		}
	}
	else {
		wirelessSslDefault_lib();		
		wirelessSetSslVer_lib(0);//wirelessSslSetTlsVer_lib(0);
		
		if (ssl == 2) {

			ret = wirelessSetSslMode_lib(1);
			memset(CerBuf, 0 , sizeof(CerBuf));
			ret = getCertificate(FILE_CERT_ROOT, CerBuf);
			MAINLOG_L1("getCertificate === %d", ret);
			ret = wirelessSendSslFile_lib(2, CerBuf, strlen((char *)CerBuf));
			memset(CerBuf, 0 , sizeof(CerBuf));
			ret = getCertificate(FILE_CERT_PRIVATE, CerBuf);
			MAINLOG_L1("getCertificate === %d", ret);
			ret = wirelessSendSslFile_lib(1, CerBuf, strlen((char *)CerBuf));
			memset(CerBuf, 0 , sizeof(CerBuf));
			ret = getCertificate(FILE_CERT_CHAIN, CerBuf);
			MAINLOG_L1("getCertificate === %d", ret);
			ret = wirelessSendSslFile_lib(0, CerBuf, strlen((char *)CerBuf));
		}
		else
			ret = wirelessSetSslMode_lib(0);

		sock = wirelessSslSocketCreate_lib();
		if (sock == -1) {
			*errCode = -4;

			return NULL;
		}
	}

	if (ssl == 0)
		ret = wirelessTcpConnect_lib(sock, (char *)host, port, 60*1000);
	else
		ret = wirelessSslConnect_lib(sock, (char *)host, port, 60*1000);

	if (ret != 0) {
		if (ssl == 0)
			wirelessSocketClose_lib(sock);
		else
			wirelessSslSocketClose_lib(sock);

		*errCode = -5;

		return NULL;
	}

	sockets[i].valid = 1;
	sockets[i].ssl = ssl;
	sockets[i].socket = sock;

	*errCode = 0;

	return &sockets[i];
}

int net_close(void *netContext)
{
	NET_SOCKET_PRI *sock = (NET_SOCKET_PRI *)netContext;

	if (sock == NULL)
		return -1;

	if (sock->valid == 0)
		return 0;

	if (sock->ssl == 0)
		wirelessSocketClose_lib(sock->socket);
	else
		wirelessSslSocketClose_lib(sock->socket);

	sock->valid = 0;

	return 0;
}

int net_read(void *netContext, unsigned char* recvBuf, int needLen, int timeOutMs)
{
	int ret;
	NET_SOCKET_PRI *sock = (NET_SOCKET_PRI *)netContext;

	if (sock == NULL)
		return -1;

	if (sock->valid == 0)
		return -1;

	if (sock->ssl == 0)
		ret = wirelessRecv_lib(sock->socket, recvBuf, 1, timeOutMs);
	else
		ret = wirelessSslRecv_lib(sock->socket, recvBuf, 1, timeOutMs);

	if (ret == 0)
		return 0;

	if (ret != 1)
		return -1;

	if (needLen == 1)
		return 1;

	if (sock->ssl == 0)
		ret = wirelessRecv_lib(sock->socket, recvBuf + 1, needLen - 1, 10);
	else
		ret = wirelessSslRecv_lib(sock->socket, recvBuf + 1, needLen - 1, 10);

	if(ret < 0)
		return -1;

	return ret + 1;
}

int net_write(void *netContext, unsigned char* sendBuf, int sendLen, int timeOutMs)
{
	NET_SOCKET_PRI *sock = (NET_SOCKET_PRI *)netContext;

	if (sock == NULL)
		return -1;

	if (sock->valid == 0)
		return -1;

	if (sock->ssl == 0)
		return wirelessSend_lib(sock->socket, sendBuf, sendLen);
	else
		return wirelessSslSend_lib(sock->socket, sendBuf, sendLen);
}

/*
#define TEST_CA_FILE "-----BEGIN CERTIFICATE-----\r\n"\
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\r\n"\
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\r\n"\
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\r\n"\
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\r\n"\
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\r\n"\
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\r\n"\
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\r\n"\
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\r\n"\
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\r\n"\
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\r\n"\
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\r\n"\
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\r\n"\
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\r\n"\
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\r\n"\
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\r\n"\
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\r\n"\
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\r\n"\
"rqXRfboQnoZsG4q5WTP468SQvvG5\r\n"\
"-----END CERTIFICATE-----\r\n"

#define TEST_CLIENT_CRT_FILE "-----BEGIN CERTIFICATE-----\n"\
"MIIDWTCCAkGgAwIBAgIULNUXViqeA8DRQtAc+ScnAVECkj4wDQYJKoZIhvcNAQEL\n"\
"BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\n"\
"SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTIyMDQwNjA5NTYw\n"\
"MFoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0\n"\
"ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALzHWBQqRlhXIAaEaeTa\n"\
"IzXCPK1IDrOULhPrSDuZcTyPoPSzkMrgzpMFSJ2BPGlB4tNAWrFjrdQyzaxcBuXw\n"\
"XeOqoRIxCx2DEn8Hq2UEnPHYxIbcFoIMxaorsUV1/Opf1twEx61i8aO8p/7k01VU\n"\
"LHi7uFS7LvqJ6r80iHUTqoTPa1ewcCWOK55eJMkVmbRxuYQpgGpF+wUKKbm7MsfK\n"\
"rmbBj3DPG7IVnx+6gTP48K/Qa8pAddATfxnCQjPQ897zhgIpdk1la8Nt+aH8QEqj\n"\
"juxvptg53pP0FfCSUmosxlxLSJGBpI29n/p3PWwi/HGP81OV9aJtWJiA/bRTI7qo\n"\
"sn8CAwEAAaNgMF4wHwYDVR0jBBgwFoAUTIvvRZfQvyh+6Ee7ekEJzZoXePEwHQYD\n"\
"VR0OBBYEFF1g0GLSKppnVeEV3FkKxGK5i2BpMAwGA1UdEwEB/wQCMAAwDgYDVR0P\n"\
"AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQBnSm1m1gAhXTAgrryImK/yXmRQ\n"\
"PhGoqiF5zYHxX/3xvTUyTVdQrgWRsE9hDtsUla5ROSu3erev2SeIMJYa1hMRuVFi\n"\
"xbADLhN31uPj4IbLXF7gKJ8vVeTGPMGljmRrBtjIcZO8E2YbhxdI5PZ67j8DoRwU\n"\
"9gu54qUKJoYcYJzAhaITBbmLSMKKQmJUGz/PLp/i8cXfJcj4eSB8ytBAyKdlKCdU\n"\
"q38gVVHtwKLR+gEaPbadvVw4+oqY/rAjJlmUZ5dDX3KBFr4NUjnIkOftnpyUKdeW\n"\
"SIL72decBArY9Xy0U0nTiPSmu6C79KkRP6yEdrbqkaPsl1rIpXKW1oQMSkLX\n"\
"-----END CERTIFICATE-----\n"



#define TEST_CLIENT_KEY_FILE "-----BEGIN RSA PRIVATE KEY-----\n"\
"MIIEowIBAAKCAQEAvMdYFCpGWFcgBoRp5NojNcI8rUgOs5QuE+tIO5lxPI+g9LOQ\n"\
"yuDOkwVInYE8aUHi00BasWOt1DLNrFwG5fBd46qhEjELHYMSfwerZQSc8djEhtwW\n"\
"ggzFqiuxRXX86l/W3ATHrWLxo7yn/uTTVVQseLu4VLsu+onqvzSIdROqhM9rV7Bw\n"\
"JY4rnl4kyRWZtHG5hCmAakX7BQopubsyx8quZsGPcM8bshWfH7qBM/jwr9BrykB1\n"\
"0BN/GcJCM9Dz3vOGAil2TWVrw235ofxASqOO7G+m2Dnek/QV8JJSaizGXEtIkYGk\n"\
"jb2f+nc9bCL8cY/zU5X1om1YmID9tFMjuqiyfwIDAQABAoIBAFFOAe9dbcKqc46b\n"\
"BQidssB6kauH91z8mwPVN90DbzPIIGiD1f6q6A2GHwpHGP+0cr7NXsI7zigwYUi0\n"\
"sfvilG1zlb/CA6mIRDUV1onBfN7kn2/95mvImHF8M/NYp79B28YTAPT6Qlxk6m3r\n"\
"m+GKSUUOhItpuwgI0mPbelICUFS2S1kvmgrU/UV7hcE8xWdCjwXJwKhA3mWjBkVd\n"\
"2aDIm8+vI2BwnEy6amNXlS3CROjFZN6tzVvN1qhb2e4syIKwSfpK//i+Ct7fKUbT\n"\
"lHJ/w9WuFaZGLvvx3nhZAQurGv74zs8bHtxwseip1I9ZxNYthF7GedFfzyc3ta0k\n"\
"8T8wsaECgYEA5X+D5N7XwoZhfkbOAtpYVAnEIyTJH9puyUKERVOgVsUbA5jEpRmL\n"\
"NDOy6QeEPzt8Pvynd7iiNOYsqAwvVimWP8Dhi3dMktT1sQW3l0HqdBwlP8wajLvB\n"\
"ZFjTu2jadIxXCHk7BrxVMsPNZCvpznUS54VkGOzLTa1bqoImIXi9CbkCgYEA0pQS\n"\
"CAjP10R+WnZI6Fn/8cOr9V6UJB49vhjjms4rpJgHM3R0ygZG6UOyeucdQXPx9DU0\n"\
"B2HpYtCTUGHMLsqZJW+LoQL94LR6bDfOEnWApvaCyRrs73lSXHaJRAxLYmQU2UOO\n"\
"jqhTkDEiIy0MEMOsRqDpUVQVS4V8PYxN7Zh7WfcCgYAnTedw0xlwn0MjNU0i//IQ\n"\
"snt86VfotKg0n8e3d9MGCUvPGeLTw8QrdI83iaoEilOFFhA4WM6u8JzFSxDwWL76\n"\
"vDXkhNIAc6iAYNJIfWmB6TAX9QS3BZDhdrUMa7C+NSrSsLCDPs34m8AZX8vzJ+nM\n"\
"7PNvsV5AN2hzk0akhOEbQQKBgQCtr3G77MC5VoY9SQjTlMAVggYIaU0ZCVR1wgOh\n"\
"QOIgbUCrQSe/JjRA3BSPaKbpwJ9VhLh4Slr8pPqMt015XqO4i+uID3ala1b6gYDY\n"\
"GtDVZcfz5eB9mPzExQRs+xMGgXPsy9r+cXoTGWOFzfcAsNQcoUYwTYEO/HfAMERq\n"\
"b3V2ewKBgACsQRJleYHUXbNghVr6Q8mr1qaC2gW7+ygwisfTxhJCzvgjq8/4tcZs\n"\
"oSXT4n9KkeoteeTe+2gl4LzPbhpQw5xHmfc3Uq54nSdlp7sRzNtugqhfJxJZPake\n"\
"XzDEmV95bDqOJpR8No+LJaJYhD5mKR9Mta8HOlH4MtPDhJW6ne5V\n"\
"-----END RSA PRIVATE KEY-----\n"
uint8 pssltmpp[93] = {
	0x10, 0x5B, 0x00, 0x04, 0x4D, 0x51, 0x54, 0x54, 0x04, 0x82, 0x02, 0x58, 0x00, 0x34, 0x4D, 0x73,
	0x77, 0x69, 0x70, 0x65, 0x2D, 0x53, 0x6F, 0x75, 0x6E, 0x64, 0x62, 0x6F, 0x78, 0x2D, 0x37, 0x35,
	0x37, 0x66, 0x39, 0x66, 0x61, 0x35, 0x2D, 0x38, 0x35, 0x63, 0x33, 0x2D, 0x34, 0x34, 0x38, 0x31,
	0x2D, 0x39, 0x38, 0x37, 0x33, 0x2D, 0x38, 0x30, 0x64, 0x66, 0x33, 0x30, 0x35, 0x39, 0x63, 0x34,
	0x32, 0x33, 0x00, 0x19, 0x3F, 0x53, 0x44, 0x4B, 0x3D, 0x50, 0x79, 0x74, 0x68, 0x6F, 0x6E, 0x26,
	0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x3D, 0x31, 0x2E, 0x34, 0x2E, 0x39
};

//uint8 p1[] = {
//	0x82, 0x26, 0x00, 0x02, 0x00, 0x21, 0x2F, 0x61, 0x31, 0x48, 0x63, 0x72, 0x6C, 0x4B, 0x6F, 0x55, \
//	0x4F, 0x41, 0x2F, 0x30, 0x30, 0x30, 0x34, 0x34, 0x30, 0x30, 0x30, 0x30, 0x33, 0x36, 0x2F, 0x75, \
//	0x73, 0x65, 0x72, 0x2F, 0x67, 0x65, 0x74, 0x01};

void DR_SSL_testi(void)
{
	//int32 iRet;
	int ret;
	//MAINLOG_L1("[%s] -%s- Line=%d:application thread enter, param 0x%x\r\n", filename(__FILE__), __FUNCTION__, __LINE__, param);
	int timeid = 0;

	timeid = TimerSet_Api();
	while(1)
	{
		ret = wirelessCheckPppDial_lib();
		MAINLOG_L1("wirelessCheckPppDial_lib = %d" , ret );
		if(ret == 0)
			break;
		else{
			ret = wirelessPppOpen_lib(NULL, NULL, NULL);
			Delay_Api(1000);
		}
		if(TimerCheck_Api(timeid , 60*1000) == 1)
		{
			return ;
		}
	}
	//ret =  wirelessSetDNS_lib("114.114.114.114", "8.8.8.8" );
	//MAINLOG_L1("wirelessSetDNS_lib = %d" , ret);


    ret = wirelessSetSslMode_lib((unsigned char)1);
    MAINLOG_L1("wirelessSetSslMode_lib :%d", ret);

	ret = wirelessSendSslFile_lib(2, TEST_CA_FILE, strlen((char *)TEST_CA_FILE));
	MAINLOG_L1("wirelessSendSslFile_lib 1:%d %d", ret, strlen((char *)TEST_CA_FILE));
	ret = wirelessSendSslFile_lib(1, TEST_CLIENT_KEY_FILE, strlen((char *)TEST_CLIENT_KEY_FILE));
	MAINLOG_L1("wirelessSendSslFile_lib 2:%d %d", ret, strlen((char *)TEST_CLIENT_KEY_FILE));
	ret = wirelessSendSslFile_lib(0, TEST_CLIENT_CRT_FILE, strlen((char *)TEST_CLIENT_CRT_FILE));
	MAINLOG_L1("wirelessSendSslFile_lib 3:%d %d", ret, strlen((char *)TEST_CLIENT_CRT_FILE));


	wirelessSslDefault_lib();
	wirelessSetSslVer_lib((unsigned char)0); //wirelessSslSetTlsVer_lib(0);

	Delay_Api(2000);

    int sock = wirelessSslSocketCreate_lib();
    if (sock == -1)
    {
    	MAINLOG_L1("[%s] -%s- Line=%d:<ERR> create ssl sock failed\r\n");
        return;//fibo_thread_delete();
    }

    MAINLOG_L1(":::fibossl fibo_ssl_sock_create %x\r\n", sock);

	ret = wirelessSslConnect_lib(sock,  "a14bkgef0ojrzw-ats.iot.us-west-2.amazonaws.com", "8883", 60*1000);

	MAINLOG_L1(":::fibossl wirelessSslConnect_lib %d\r\n", ret);

    ret = wirelessSslSend_lib(sock, pssltmpp, sizeof(pssltmpp));
	MAINLOG_L1(":::fibossl sys_sock_send %d\r\n", ret);

    ret = wirelessSslRecv_lib(sock, buf, 1, 1000);
	MAINLOG_L1(":::fibossl sys_sock_recv %d\r\n", ret);
	ret = wirelessSslRecv_lib(sock, buf, 32, 10000);
	MAINLOG_L1(":::fibossl sys_sock_recv %d\r\n", ret);

	ret = wirelessSslGetErrcode_lib();
	MAINLOG_L1(":::fibo_get_ssl_errcode, ret = %d\r\n", ret);

	uint32 port = 6500;

	MAINLOG_L1(":::wifiTCPConnect_lib, iRet = %d\r\n", iRet);
}*/

