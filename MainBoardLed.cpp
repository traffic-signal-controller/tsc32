/***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   MainBoardLed.cpp
Date:       2013-1-1
Description:信号机主板指示灯和LED显示板灯文件
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
Description:    CMainBoardLed 构造函数
Input:          无
Output:         无
Return:         无
***************************************************************/
CMainBoardLed::CMainBoardLed()
{
	STscConfig* pSTscConfig = NULL ;
	pSTscConfig =  CManaKernel::CreateInstance()->m_pTscConfig ;
	
	for(int iNumFun = 0 ; iNumFun < 12 ;iNumFun++)
		LedBoardStaus[iNumFun] = 0x1 ;
	if ( 0 !=pSTscConfig->sSpecFun[FUN_GPS].ucValue )
		LedBoardStaus[8] = 0x2;
	if ( 0 != pSTscConfig->sSpecFun[FUN_3G].ucValue )
		LedBoardStaus[4] = 0x2;
	if ( 0 != pSTscConfig->sSpecFun[FUN_MSG_ALARM].ucValue )
		LedBoardStaus[5] = 0x2;
	if ( 0 != pSTscConfig->sSpecFun[FUN_CAM].ucValue )
		LedBoardStaus[6] = 0x2;
	if ( 0 != pSTscConfig->sSpecFun[FUN_WLAN].ucValue )
		LedBoardStaus[7] = 0x2;
	LedBoardStaus[9] = 0x2;//黄闪器
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
Description:    CMainBoardLed 析构函数
Input:          无
Output:         无
Return:         无
***************************************************************/
CMainBoardLed::~CMainBoardLed()
{
	system("echo 112 >"DEV_PATH"unexport");
	system("echo 116 >"DEV_PATH"unexport");
	ACE_DEBUG((LM_DEBUG,"%s:%d Destruct MainBoardLed object ok !\n",__FILE__,__LINE__));
}
/**************************************************************
Function:       CreateInstance
Description:    CMainBoardLed 创建单例对象
Input:          无
Output:         无
Return:         无
***************************************************************/
CMainBoardLed* CMainBoardLed::CreateInstance()
{
	static CMainBoardLed cMainBoardLed;
	return &cMainBoardLed;
}



/****************
*设置MODE指示灯显示状态, 使用MainBackup 类中的方法
		修正:	  绿：正常     0 0
                  黄：降级黄闪 0 1
                  红：无法工作 1 0
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
*设置TSC/PSC指示灯显示状态,  
*使用MainBackup 类中的方法
***********/
void CMainBoardLed::DoTscPscLed(bool bValue)
{
	if(bValue)
		MainBackup::CreateInstance()->DoWriteLED(MODE_TSC);
	else
		MainBackup::CreateInstance()->DoWriteLED(MODE_PSC1);
}

/***********
*设置Auto指示灯显示状态,  
*使用MainBackup 类中的方法
**************/
void CMainBoardLed::DoAutoLed(bool bValue)
{
	if(bValue)
		MainBackup::CreateInstance()->DoWriteLED(CTRL_SCHEDULE);
	else
		MainBackup::CreateInstance()->DoWriteLED(CTRL_PANEL);
}

/*****
*	系统自动运行
*	linux 系统运行
******/
void CMainBoardLed::DoRunLed()
{

}


/**************************************************************
Function:       DoCan1Led
Description:    CMainBoardLed 设置Can指示灯显示状态, CAN总线收发时亮,使用NLED2 IO口
Input:          无
Output:         无
Return:         无
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
Description:    CMainBoardLed can1 指示灯
Input:          无
Output:         无
Return:         无
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
Description:    CMainBoardLed 网口灯亮
Input:          无
Output:         无
Return:         无
***************************************************************/
bool CMainBoardLed::IsEthLinkUp()
{
	Byte rusult = 0 ;
	FILE *fpsys = NULL ;             //定义管道符句柄
	fpsys = popen("ifconfig eth0|grep -c RUNNING", "r");  //打开管道,执行脚本或命令
	fread(&rusult, 1 , 1,fpsys) ;            //读取管道符,获得执行结果
	pclose(fpsys);                     //关闭管道符句柄
	fpsys = NULL;
	if(rusult ==(Byte)'1')
	{
		//printf("\nEth0 link up!\n");
		//LedBoardStaus[3] = 0x2 ;
		return true ;
	}
	else
	
	{
		//printf("\nEth0 link down!\n");
		//LedBoardStaus[3] = 0x1 ;
		return false ;
	}
	

}
/**************************************************************
Function:       DoLedBoardShow
Description:    CMainBoardLed  LED面板灯亮
Input:          无
Output:         无
Return:         无
***************************************************************/
void CMainBoardLed::DoLedBoardShow()
{
	if(IsEthLinkUp())
		LedBoardStaus[3] = 0x2 ;
	else
		LedBoardStaus[3] = 0x1 ;
	if(CFlashMac::CreateInstance()->GetHardwareFlash())
		LedBoardStaus[9] = 0x3 ;
	if(CGps::CreateInstance()->m_bGpsTime)
		LedBoardStaus[8] = 0x3 ;
	SetLedBoardShow();
}


/**************************************************************
Function:       SetLedBoardShow
Description:    CMainBoardLed 设置led面板灯亮
Input:          无
Output:         无
Return:         无
***************************************************************/
void CMainBoardLed::SetLedBoardShow()
{		
		SCanFrame sSendFrameTmp;
		ACE_OS::memset(&sSendFrameTmp , 0 , sizeof(SCanFrame));	
		//Can::BuildCanId(CAN_MSG_TYPE_100 , BOARD_ADDR_MAIN  , FRAME_MODE_P2P , BOARD_ADDR_LED  , &(sSendFrameTmp.ulCanId));
		//ACE_DEBUG((LM_DEBUG,"%s:%d LEDCANID:%x \n",__FILE__,__LINE__,sSendFrameTmp.ulCanId));
		sSendFrameTmp.ulCanId = 0x91032ff0 ;
		sSendFrameTmp.pCanData[0] = ( DATA_HEAD_RESEND<< 6) | LED_BOARD_SHOW;				
		sSendFrameTmp.pCanData[1] = LedBoardStaus[0]&0x3 ;
		sSendFrameTmp.pCanData[1] |= (LedBoardStaus[1]&0x3)<<2 ;
		sSendFrameTmp.pCanData[1] |= (LedBoardStaus[2]&0x3)<<4 ;
		sSendFrameTmp.pCanData[1] |= (LedBoardStaus[3]&0x3)<<6 ;

		sSendFrameTmp.pCanData[2] = LedBoardStaus[4]&0x3 ;
		sSendFrameTmp.pCanData[2] |= (LedBoardStaus[5]&0x3)<<2 ;
		sSendFrameTmp.pCanData[2] |= (LedBoardStaus[6]&0x3)<<4 ;
		sSendFrameTmp.pCanData[2] |= (LedBoardStaus[7]&0x3)<<6 ;

		sSendFrameTmp.pCanData[3] = LedBoardStaus[8]&0x3 ;
		sSendFrameTmp.pCanData[3] |= (LedBoardStaus[9]&0x3)<<2 ;		
		sSendFrameTmp.ucCanDataLen = 4;	
		Can::CreateInstance()->Send(sSendFrameTmp);
		//ACE_DEBUG((LM_DEBUG,"%s:%d pCanData[1]= %02x , pCanData[2]= %02x ,pCanData[3]= %02x \n ",__FILE__,__LINE__,sSendFrameTmp.pCanData[1],sSendFrameTmp.pCanData[2],sSendFrameTmp.pCanData[3]));

}


