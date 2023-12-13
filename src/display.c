/*
 * display.c
 *
 *  Created on: Oct 25, 2021
 *      Author: Administrator
 */

#include <string.h>

#include "def.h"
#include <coredef.h>
#include <struct.h>
#include <poslib.h>

#define QRBMP "QRBMP.bmp"

void QRCodeDisp()
{
#ifdef __DISPLAY__
	u8 OutBuf[1024];
	int ret;
	struct _LCDINFO lcdInfo;

	memset(&lcdInfo,0,sizeof(lcdInfo));
	ScrGetInfo_Api(&lcdInfo);

	memset(OutBuf, 0, sizeof(OutBuf));
	strcpy(OutBuf, "https://www.vanstone.com.cn/en/aboutus.html");
	ret = QREncodeString(OutBuf, 3, 3, QRBMP, 2.5);
	ScrCls_Api();
	scrSetBackLightMode_lib(2,1000);
	ScrDispImage_Api(QRBMP, 0, 10);
#endif
}



void DispMainFace(void)
{
	ScrCls_Api();
	ScrDisp_Api(LINE4, 0, "Welcome to use", CDISP);
	ScrDisp_Api(LINE5, 0, "Aisino Q161", CDISP);
}


int WaitEvent(void)
{
	u8 Key;
	int TimerId;

	TimerId = TimerSet_Api();
	while(1)
	{
		if(TimerCheck_Api(TimerId , 30*1000) == 1)
		{
			SysPowerStand_Api();
			return 0xfe;
		}
		Key = GetKey_Api();
		if(Key != 0)
			return Key;
	}

	return 0;
}
#define		TIMEOUT			-2
int ShowMenuItem(char *Title, const char *menu[], u8 ucLines, u8 ucStartKey, u8 ucEndKey, int IsShowX, u8 ucTimeOut)
{
	u8 IsShowTitle, cur_screen, OneScreenLines, Cur_Line, i, t;
	int nkey;
	char dispbuf[50];

	memset(dispbuf, 0, sizeof(dispbuf));

	if(Title != NULL)
	{
		IsShowTitle = 1;
		OneScreenLines = 12;
	}
	else
	{
		IsShowTitle = 0;
		OneScreenLines = 13;
	}
	IsShowX -= 1;
	cur_screen = 0;
	while(1)
	{
		ScrClsRam_Api();
		if(IsShowTitle)
			ScrDisp_Api(LINE1, 0, Title, CDISP);
		Cur_Line = LINE1+IsShowTitle;
		for(i = 0; i < OneScreenLines; i++)
		{
			t = i+cur_screen*OneScreenLines;
			if (t >= ucLines || menu[t] == NULL)//
			{
				break;
			}
			memset(dispbuf, 0, sizeof(dispbuf));
			strcpy(dispbuf, menu[t]);
			ScrDispRam_Api(Cur_Line++, 0, dispbuf, FDISP);
		}
		ScrBrush_Api();
		MAINLOG_L1("after ScrBrush_Api");
		nkey = WaitAnyKey_Api(ucTimeOut);
		MAINLOG_L1("WaitAnyKey_Api aa:%d",nkey);
		switch(nkey)
		{
		case ESC:
		case TIMEOUT:
			return nkey;
		default:
			if( (nkey >= ucStartKey)&&(nkey <= ucEndKey) )
				return nkey;
			break;
		}
	};
}

void MenuThread()
{
	int Result = 0;
	while(1)
	{
		DispMainFace();
		Result = WaitEvent();
		if(Result == 0xfe)
			continue;
		if(Result != 0)
		{
			switch(Result)
			{
				case ENTER:
					SelectMainMenu();
					break;
				default:
					break;
			}
		}
	}
}


void SelectMainMenu(void)
{
	int nSelcItem = 1, ret;

	char *pszTitle = "Menu";
	const char *pszItems[] = {
		"1.Amount Display" /*,
		"2.WriteParam NULL ",
		"3.WriteParam M2MISAFE"*/
		
	};
	while(1)
	{
		nSelcItem = ShowMenuItem(pszTitle, pszItems, sizeof(pszItems)/sizeof(char *), DIGITAL1, DIGITAL3, 0, 60);
		MAINLOG_L1("ShowMenuItem = %d  %d  %d %d",nSelcItem, DIGITAL1, DIGITAL2, DIGITAL3);
		switch(nSelcItem)
		{
		case DIGITAL1:
#ifdef __DISPLAY__
			secscrOpen_lib();
			secscrCls_lib();
			secscrSetAttrib_lib(4,1);
			secscrPrint_lib(0, 0, 0, "114.00");
			WaitAnyKey_Api(60);
#endif			
			break;
		/*case DIGITAL2:
			ret = wirelessPdpWriteParam_lib(NULL, NULL, NULL);
			MAINLOG_L1("wirelessPdpWriteParam_lib NULL = %d", ret);
			SysPowerReBoot_Api();
			break;
		case DIGITAL3:
			ret = wirelessPdpWriteParam_lib("M2MISAFE", "", ""); 
			MAINLOG_L1("wirelessPdpWriteParam_lib M2MISAFE = %d", ret);
			SysPowerReBoot_Api();
			break ;*/
		case ESC:
			return ;
		default:
			break;
		}
	}
}

