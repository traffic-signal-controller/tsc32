/***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   MainBoardLed.cpp
Date:       2013-1-1
Description:�źŻ�����ָʾ�ƺ�LED��ʾ����ļ�
Version:    V1.0
History:
***************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "MainBoardLed.h"
#include "ManaKernel.h"
#include "Can.h"
#include "FlashMac.h"
#include "Gps.h"
#include "MainBackup.h"

#define DEV_PATH "/sys/class/gpio/"



/**************************************************************
Function:       CMainBoardLed
Description:    CMainBoardLed ���캯��
Input:          ��
Output:         ��
Return:         ��
***************************************************************/
CMainBoardLed::CMainBoardLed()
{	
	for(Byte LedIndex = 0 ; LedIndex < MAXLED ;LedIndex++)
		LedBoardStaus[LedIndex] = LED_STATUS_OFF;
	
	LedBoardStaus[LED_YWFLASH] = LED_STATUS_ON;//��������Ϊ����ģ��Ĭ�ϵ���
	can1Bool = true;
	can0Bool = true;

	system("echo 112 >"DEV_PATH"export");
	system("echo out >"DEV_PATH"gpio112/direction");
	
	system("echo 116 >"DEV_PATH"export");
	system("echo out >"DEV_PATH"gpio116/direction");
	ACE_DEBUG((LM_DEBUG,"%s:%d Init MainBoardLed object ok !\n",__FILE__,__LINE__));
}
/**************************************************************
Function:       ~CMainBoardLed
Description:    CMainBoardLed ��������
Input:          ��
Output:         ��
Return:         ��
***************************************************************/
CMainBoardLed::~CMainBoardLed()
{
	system("echo 112 >"DEV_PATH"unexport");
	system("echo 116 >"DEV_PATH"unexport");
	ACE_DEBUG((LM_DEBUG,"%s:%d Destruct MainBoardLed object ok !\n",__FILE__,__LINE__));
}
/**************************************************************
Function:       CreateInstance
Description:    CMainBoardLed ������������
Input:          ��
Output:         ��
Return:         ��
***************************************************************/
CMainBoardLed* CMainBoardLed::CreateInstance()
{
	static CMainBoardLed cMainBoardLed;
	return &cMainBoardLed;
}



/****************
*����MODEָʾ����ʾ״̬, ʹ��MainBackup ���еķ���
		����:	  �̣�����     0 0
                  �ƣ��������� 0 1
                  �죺�޷����� 1 0
***************************************************************/
void CMainBoardLed::DoModeLed(bool bLed3Value,bool bLed4Value)
{
	if(bLed3Value == false && bLed4Value == false)
	{
		MainBackup::CreateInstance()->DoWriteLED(LED_MODE_GREEN);
	}
	else if(bLed3Value == false && bLed4Value == true)
	{
		MainBackup::CreateInstance()->DoWriteLED(LED_MODE_YELLOW);
	}
 	else if(bLed3Value == true && bLed4Value == false)
	{
		MainBackup::CreateInstance()->DoWriteLED(LED_MODE_RED);
	}
}

/***********
*����TSC/PSCָʾ����ʾ״̬,  
*ʹ��MainBackup ���еķ���
***********/
void CMainBoardLed::DoTscPscLed(bool bValue)
{
	if(bValue)
		MainBackup::CreateInstance()->DoWriteLED(MODE_TSC);
	else
		MainBackup::CreateInstance()->DoWriteLED(MODE_PSC1);
}

/***********
*����Autoָʾ����ʾ״̬,  
*ʹ��MainBackup ���еķ���
**************/
void CMainBoardLed::DoAutoLed(bool bValue)
{
	if(bValue)
		MainBackup::CreateInstance()->DoWriteLED(CTRL_SCHEDULE);
	else
		MainBackup::CreateInstance()->DoWriteLED(CTRL_PANEL);
}

/*****
*	ϵͳ�Զ�����
*	linux ϵͳ����
******/
void CMainBoardLed::DoRunLed()
{

}


/**************************************************************
Function:       DoCan1Led
Description:    CMainBoardLed ����Canָʾ����ʾ״̬, CAN�����շ�ʱ��,ʹ��NLED2 IO��
Input:          ��
Output:         ��
Return:         ��
***************************************************************/
void CMainBoardLed::DoCan0Led()
{
	

	if(can0Bool)
	{
		can0Bool = false;
		system("echo 1 >> "DEV_PATH"gpio112/value");
		//ACE_DEBUG((LM_DEBUG,"%s:%d gpio112   :false \n",__FILE__,__LINE__));
	}
	else
	{
		system("echo 0 >> "DEV_PATH"gpio112/value");
		can0Bool = true;
		//ACE_DEBUG((LM_DEBUG,"%s:%d gpio112   :true \n",__FILE__,__LINE__));
	}

	
}
/**************************************************************
Function:       DoCan1Led
Description:    CMainBoardLed can1 ָʾ��
Input:          ��
Output:         ��
Return:         ��
***************************************************************/
void CMainBoardLed::DoCan1Led()
{
	

	if(can1Bool)
		{
		system("echo 1 >> "DEV_PATH"gpio116/value");
		can1Bool = false;
		}
	else
		{
		system("echo 0 >> "DEV_PATH"gpio116/value");
		can1Bool = true;
		}

	
}
/**************************************************************
Function:       IsEthLinkUp
Description:    CMainBoardLed ���ڵ���
Input:          ��
Output:         ��
Return:         ��
***************************************************************/
bool CMainBoardLed::IsEthLinkUp()
{
	Byte rusult = 0 ;
	FILE *fpsys = NULL ;             //����ܵ������
	fpsys = popen("ifconfig eth0|grep -c RUNNING", "r");  //�򿪹ܵ�,ִ�нű�������
	fread(&rusult, 1 , 1,fpsys) ;            //��ȡ�ܵ���,���ִ�н��
	pclose(fpsys);                     //�رչܵ������
	fpsys = NULL;
	if(rusult ==(Byte)'1')
	{
		return true ;
	}
	else
	
	{
		return false ;
	}
	

}
/**************************************************************
Function:       DoLedBoardShow
Description:    CMainBoardLed  LED������
Input:          ��
Output:         ��
Return:         ��
***************************************************************/
void CMainBoardLed::DoLedBoardShow()
{
	STscConfig* pSTscConfig =CManaKernel::CreateInstance()->m_pTscConfig ;
	if(IsEthLinkUp())
		LedBoardStaus[LED_NETWORK] = LED_STATUS_ON;
	else
		LedBoardStaus[LED_NETWORK] = LED_STATUS_OFF;	
	if(CFlashMac::CreateInstance()->GetHardwareFlash())
		LedBoardStaus[LED_YWFLASH] = LED_STATUS_FLASH ;
	else
		LedBoardStaus[LED_YWFLASH] = LED_STATUS_ON ;
	SetLedBoardShow();
}


/**************************************************************
Function:       SetLedBoardShow
Description:    CMainBoardLed ����led������
Input:          ��
Output:         ��
Return:         ��
***************************************************************/
void CMainBoardLed::SetLedBoardShow()
{		
		SCanFrame sSendFrameTmp;
		ACE_OS::memset(&sSendFrameTmp , 0 , sizeof(SCanFrame));	
		//Can::BuildCanId(CAN_MSG_TYPE_100 , BOARD_ADDR_MAIN  , FRAME_MODE_P2P , BOARD_ADDR_LED  , &(sSendFrameTmp.ulCanId));
		//ACE_DEBUG((LM_DEBUG,"%s:%d LEDCANID:%x \n",__FILE__,__LINE__,sSendFrameTmp.ulCanId));
		sSendFrameTmp.ulCanId = 0x91032ff0 ;
		sSendFrameTmp.pCanData[0] = ( DATA_HEAD_RESEND<< 6) | LED_BOARD_SHOW;				
		sSendFrameTmp.pCanData[1] = LedBoardStaus[LED_RADIATOR]&0x3 ;
		sSendFrameTmp.pCanData[1] |= (LedBoardStaus[LED_HEATER]&0x3)<<2 ;
		sSendFrameTmp.pCanData[1] |= (LedBoardStaus[LED_ILLUMINATOR]&0x3)<<4 ;
		sSendFrameTmp.pCanData[1] |= (LedBoardStaus[LED_NETWORK]&0x3)<<6 ;

		sSendFrameTmp.pCanData[2] = LedBoardStaus[LED_3G]&0x3 ;
		sSendFrameTmp.pCanData[2] |= (LedBoardStaus[LED_MSG]&0x3)<<2 ;
		sSendFrameTmp.pCanData[2] |= (LedBoardStaus[LED_CAM]&0x3)<<4 ;
		sSendFrameTmp.pCanData[2] |= (LedBoardStaus[LED_WIRELESSBUTTON]&0x3)<<6 ;

		sSendFrameTmp.pCanData[3] =  LedBoardStaus[LED_GPS]&0x3 ;
		sSendFrameTmp.pCanData[3] |= (LedBoardStaus[LED_YWFLASH]&0x3)<<2 ;		
		sSendFrameTmp.ucCanDataLen = 4;	
		Can::CreateInstance()->Send(sSendFrameTmp);
		//ACE_DEBUG((LM_DEBUG,"%s:%d pCanData[1]= %02x , pCanData[2]= %02x ,pCanData[3]= %02x \n ",__FILE__,__LINE__,sSendFrameTmp.pCanData[1],sSendFrameTmp.pCanData[2],sSendFrameTmp.pCanData[3]));

}

/**************************************************************
Function:       SetSignalLed
Description:   ���õ�������ʾ�ư�LED��״̬
Input:           LedIndex - ģ��LED������
			LedStatus - ָ��LED��״̬
Output:         ��
Return:         ��
Date:           20141106
***************************************************************/

void CMainBoardLed::SetSignalLed(Byte LedIndex ,Byte LedStatus)
{
	if(LedIndex>(MAXLED-1) && LedIndex<0)
		return ;
	LedBoardStaus[LedIndex] = LedStatus ;
}
