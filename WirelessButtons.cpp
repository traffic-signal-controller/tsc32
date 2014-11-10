 /***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   LampBoard.cpp
Date:       2013-4-13
Description:淇″彿鏈虹伅鎵嬫帶鎿嶄綔澶勭悊绫绘枃浠?鍖呭惈瀵规墜鎺ч潰鏉垮叏绾紝榛勯棯锛屾杩?鎵嬪姩鑷姩鍒囨崲鎸夐挳鎿嶄綔鐨勫鐞嗐�?
Version:    V1.0
History:    2013.05.29 淇敼鎸夐挳鎸変笅鍜屾澗寮�鐨勫鐞嗐�?
		  2013.05.31 娣诲姞褰撴墜鍔ㄧ姸鎬佺淮鎶?鍒嗛挓浠ヤ笂鏃惰嚜鍔ㄥ垏鎹负鑷姩鐘舵�?銆?
		  2013.09.10 淇敼鎸夐挳鎺ユ敹澶勭悊鏂瑰紡锛岀畝鍖栦唬鐮併�?
		  2014.10.06 淇敼鎸夐挳鎺ュ彈澶勭悊鏂瑰紡锛岀敱浣嶇Щ寰幆鍒ゆ柇淇敼浣嶈鍙栭敭鍊艰繘琛屽垽鏂?
		  2014.10.23 改造成支持无线按键手控控制
********************************************************************************************/
#include "WirelessButtons.h"
#include "TscMsgQueue.h"
#include "ace/Date_Time.h"
#include "MainBoardLed.h"
#include "TimerManager.h"
#include "GbtMsgQueue.h"
#include "Can.h"
#include "LampBoard.h"
enum{
	 BUTTON_MANUAL_SELF 		= 0x0,	//自主运行
	 BUTTON_MANUAL_MANUAL 		= 0x1,	//手动运行	
	 BUTTON_MANUAL_YELLOW_FLASH	= 0x2,	//黄闪
	 BUTTON_MANUAL_ALL_RED		= 0x3,	//全红
	 BUTTON_MANUAL_NEXT_STEP	= 0x4,	//下一步
	 BUTTON_MANUAL_NEXT_PHASE	= 0x5,	//下一相位
	 BUTTON_MANUAL_NEXT_DIREC	= 0x6,	//下一方向
	 BUTTON_SPECIAL_DIREC		= 0x7	//按特殊组合指定方向和相位车道方形
	
};

enum{

	LAMP_OFF = 0x0 ,     //熄灭
	LAMP_GreenOn = 0x1 , //绿灯亮
	LAMP_GreenFLASH = 0x2 , //绿闪
	LAMP_YELLOW = 0x3 ,   //黄灯亮
	LAMP_YELLOWFLASH = 0x4 , //黄闪
	LAMP_RED = 0x5        //红灯亮
	

};

/**************************************************************
Function:       CWirelessBtn::CWirelessBtn
Description:    CWirelessBtn绫绘瀯閫犲嚱鏁帮紝鐢ㄤ簬绫诲垵濮嬪寲澶勭悊				
Input:          鏃?             
Output:         鏃?
Return:         鏃?
***************************************************************/
CWirelessBtn::CWirelessBtn() 
{	
	m_ucLastManualSts = BUTTON_MANUAL_SELF; //初始化处于自动运行状态	
	m_ucdirectype = 0x0 ; 					//方向类型 0-北方 1- 东方 2- 南方 3-西方	
	bFirstOperate = true ;
	bTransitSetp  = false ;
	RecvoldDirec = 0 ;
	pManaKernel = CManaKernel::CreateInstance();
}

/**************************************************************
Function:       CWirelessBtn::~CWirelessBtn
Description:    CWirelessBtn绫?鏋愭瀯鍑芥暟	
Input:          鏃?             
Output:         鏃?
Return:         鏃?
***************************************************************/
CWirelessBtn::~CWirelessBtn() 
{
	;
}

/**************************************************************
Function:       Manual::CreateInstance
Description:    鍒涘缓	Manual闈欐�佸璞?
Input:          鏃?             
Output:         鏃?
Return:         闈欐�佸璞℃寚閽?
***************************************************************/
CWirelessBtn* CWirelessBtn::CreateInstance()
{
	static CWirelessBtn cWirelessBtn;
	return &cWirelessBtn;
}

/****************************************************************
Function:       CWirelessBtn::EntryWirelessManul
Description:    退出无线遥控按键，恢复自主控制.定时退出
Input:           无          
Output:         无
Return:         无
Date:           20141023
*****************************************************************/

void CWirelessBtn::BackToAuto()
{
	CTimerManager::CreateInstance()->CloseWirelessBtn();
	/*if(m_ucLastManualSts==BUTTON_MANUAL_NEXT_DIREC || m_ucLastManualSts==BUTTON_SPECIAL_DIREC) //特殊方向自动返回的时候有过渡步
	{		
		bTransitSetp = true ;	
		pManaKernel->SetWirelessBtnDirecCfg(RecvoldDirec,0x2);						
		ACE_OS::sleep(2);
		pManaKernel->SetWirelessBtnDirecCfg(RecvoldDirec,0x1);
		ACE_OS::sleep(2);
		bTransitSetp = false ;
		
	}	
	else if(m_ucLastManualSts==BUTTON_MANUAL_NEXT_STEP) //当从手动下一步退出 则黄灯过渡3秒
	{
		
		bTransitSetp = true ;
		SetCurrenStepFlash(0x1);		
		ACE_OS::sleep(2);
		bTransitSetp = false ;
	}
	*/
	CMainBoardLed::CreateInstance()->DoAutoLed(true);
	CGbtMsgQueue::CreateInstance()->SendTscCommand(OBJECT_SWITCH_SYSTEMCONTROL,0);	
	pManaKernel->SetCycleStepInfo(0); //单单构造整个周期的步伐信息即可
	
	CLampBoard::CreateInstance()->SetLamp(pManaKernel->m_pRunData->sStageStepInfo[pManaKernel->m_pRunData->ucStepNo].ucLampOn,pManaKernel->m_pRunData->sStageStepInfo[pManaKernel->m_pRunData->ucStepNo].ucLampFlash); //退出来的时候重新给灯控板灯色信息
	bFirstOperate = true ;
	m_ucLastManualSts = BUTTON_MANUAL_SELF;
	pManaKernel->m_pRunData->ucManualType = Manual_CTRL_NO;
	CMainBoardLed::CreateInstance()->SetSignalLed(LED_WIRELESSBUTTON,LED_STATUS_ON);
	
	ACE_DEBUG((LM_DEBUG,"%s:%d Back to Auto-Ctrl from WirelessButtons! \n",__FILE__,__LINE__));
}

/****************************************************************
Function:       CWirelessBtn::EntryWirelessManul
Description:    退出无线遥控按键，恢复自主控制。手动按
Input:           无          
Output:         无
Return:         无
Date:           20141023
*****************************************************************/

void CWirelessBtn::BackToAutoByManual()
{
	CTimerManager::CreateInstance()->CloseWirelessBtn();
	if(m_ucLastManualSts==BUTTON_MANUAL_NEXT_DIREC || m_ucLastManualSts==BUTTON_SPECIAL_DIREC) //特殊方向自动返回的时候有过渡步
	{		
		bTransitSetp = true ;	
		pManaKernel->SetWirelessBtnDirecCfg(RecvoldDirec,0x2);						
		ACE_OS::sleep(2);
		pManaKernel->SetWirelessBtnDirecCfg(RecvoldDirec,0x1);
		ACE_OS::sleep(3);
		bTransitSetp = false ;
		
	}	
	else if(m_ucLastManualSts==BUTTON_MANUAL_NEXT_STEP) //当从手动下一步退出 则黄灯过渡3秒
	{
		
		bTransitSetp = true ;
		SetCurrenStepFlash(0x1);		
		ACE_OS::sleep(2);
		bTransitSetp = false ;
	}
	
	CMainBoardLed::CreateInstance()->DoAutoLed(true);
	CGbtMsgQueue::CreateInstance()->SendTscCommand(OBJECT_SWITCH_SYSTEMCONTROL,0);	
	pManaKernel->SetCycleStepInfo(0); //单单构造整个周期的步伐信息即可
	
	CLampBoard::CreateInstance()->SetLamp(pManaKernel->m_pRunData->sStageStepInfo[pManaKernel->m_pRunData->ucStepNo].ucLampOn,pManaKernel->m_pRunData->sStageStepInfo[pManaKernel->m_pRunData->ucStepNo].ucLampFlash); //退出来的时候重新给灯控板灯色信息
	bFirstOperate = true ;
	m_ucLastManualSts = BUTTON_MANUAL_SELF;
	pManaKernel->m_pRunData->ucManualType = Manual_CTRL_NO;
	CMainBoardLed::CreateInstance()->SetSignalLed(LED_WIRELESSBUTTON,LED_STATUS_ON);
	
	ACE_DEBUG((LM_DEBUG,"%s:%d Back to Auto-Ctrl from WirelessButtons! \n",__FILE__,__LINE__));
}


/****************************************************************
*
Function:       CWirelessBtn::EntryWirelessManul
Description:    初次进入无线按键手动状态
Input:           无          
Output:         无
Return:         无
Date:           20141023
*****************************************************************/

void CWirelessBtn::EntryWirelessManul()
{
	CMainBoardLed::CreateInstance()->DoAutoLed(false);					
	CGbtMsgQueue::CreateInstance()->SendTscCommand(OBJECT_CURTSC_CTRL,1);
	ACE_DEBUG((LM_DEBUG,"%s:%d First Entry  WirelessBtnManual TscMsg! \n",__FILE__,__LINE__));
	bTransitSetp = true ;
	if(!SetCurrenStepFlash(0x2)) //当前方向相位如果是绿灯则绿闪
	//如果当前是非绿灯状态，绿闪操作无效，则进入绿闪黄灯进入绿闪操作判断
	{
		SetCurrenStepFlash(0x3); //当前方向相位黄灯则黄闪
	}
	bTransitSetp = false ;
	RecvoldDirec = 0 ;	
	m_ucLastManualSts = BUTTON_MANUAL_MANUAL;
	pManaKernel->m_pRunData->ucManualType = Manual_CTRL_WIRELESSBUTTONS;	
	CMainBoardLed::CreateInstance()->SetSignalLed(LED_WIRELESSBUTTON,LED_STATUS_FLASH);

}

/****************************************************************
*
Function:       CWirelessBtn::RecvMacCan
Description:    对从遥控器返回主板的按键CAN数据进行处理
Input:            Can总线接收到的无线按键数据帧            
Output:         无
Return:         无
Date:           20141023
*****************************************************************/
void CWirelessBtn::RecvMacCan(SCanFrame sRecvCanTmp)
{

	if(pManaKernel->m_pRunData->ucManualType != Manual_CTRL_NO && pManaKernel->m_pRunData->ucManualType != Manual_CTRL_WIRELESSBUTTONS)
		return ;
	
	Byte RecvType = sRecvCanTmp.pCanData[0] & 0x3F ;
	if(RecvType == 0x2 && bTransitSetp == false) //手控非过渡步
	{		
		CGbtMsgQueue *pGbtMsgQueue = CGbtMsgQueue::CreateInstance();
		Byte RecvButtonType = sRecvCanTmp.pCanData[1] ;
		Uint DirecButtonCfg = ((sRecvCanTmp.pCanData[2]|sRecvCanTmp.pCanData[3]<<8)|sRecvCanTmp.pCanData[4]<<16)|sRecvCanTmp.pCanData[5]<<24;
		ACE_DEBUG((LM_DEBUG,"%s:%d Send %x %x %x %x %x %x !\n",__FILE__,__LINE__, sRecvCanTmp.pCanData[0],sRecvCanTmp.pCanData[1],sRecvCanTmp.pCanData[2],sRecvCanTmp.pCanData[3],sRecvCanTmp.pCanData[4],sRecvCanTmp.pCanData[5]));
		Byte iAuto_Manul = RecvButtonType&0x1 ;		
		if(iAuto_Manul == 0x0) //自动状态
		{
			if(m_ucLastManualSts == BUTTON_MANUAL_SELF) //如果上次状态已经是自主运行
				return ;
			else
			{
				BackToAutoByManual();
								
				CWirelessBtnTimer::CreateInstance()->Resettimeout();  //退出定时器超时计数置为0
			}
		}
		else if(iAuto_Manul == 0x1) //手控状态
		{				
			if(m_ucLastManualSts == BUTTON_MANUAL_SELF) //如果上次状态是自主运行
			{
				EntryWirelessManul();
				CTimerManager::CreateInstance()->StartWirelessBtnTimer();
				
				//ACE_OS::printf("%s:%d WirelessBtn timeout = %d seconds!\r\n",CManaKernel::CreateInstance()->m_pTscConfig->sSpecFun[FUN_WIRELESSBTN_TIMEOUT].ucValue*60);
			}		
			else
			{									
				CWirelessBtnTimer::CreateInstance()->Resettimeout();  //有在进行无线手控操作则，超时计数置为0
				if((RecvButtonType>>1)&0x1)  //进入黄闪控制
				{
					if(m_ucLastManualSts == BUTTON_MANUAL_YELLOW_FLASH)
					{
						return ;
					}
					else
					{
						pGbtMsgQueue->SendTscCommand(OBJECT_SWITCH_SYSTEMCONTROL,254);
						
						pManaKernel->SndMsgLog(LOG_TYPE_MANUAL,2,0,0,0); //ADD:20141031
						ACE_DEBUG((LM_DEBUG,"%s:%d Send WirelessBtn FLASH! TscMsg!\n",__FILE__,__LINE__));
						m_ucLastManualSts = BUTTON_MANUAL_YELLOW_FLASH;
						pManaKernel->m_pRunData->flashType = CTRLBOARD_FLASH_MANUALCTRL;
						bFirstOperate = true ;
					}
				}
				else if((RecvButtonType>>2)&0x1)//进入全红控制
				{
					if (m_ucLastManualSts == BUTTON_MANUAL_ALL_RED)
					{
						return ;
					}
					else
					{
						pGbtMsgQueue->SendTscCommand(OBJECT_SWITCH_SYSTEMCONTROL,253);
						pManaKernel->SndMsgLog(LOG_TYPE_MANUAL,4,0,0,0); //ADD:20141031
						ACE_DEBUG((LM_DEBUG,"%s:%d Send WirelessBtn  ALLRED TscMsg!\n",__FILE__,__LINE__));
						m_ucLastManualSts = BUTTON_MANUAL_ALL_RED;
						bFirstOperate = true ;
					}
				}
				else if((RecvButtonType>>3)&0x1)//点击下一步按键
				{
					if(m_ucLastManualSts == BUTTON_MANUAL_MANUAL || m_ucLastManualSts == BUTTON_MANUAL_NEXT_STEP)  //下一步只能由手控后直接进入，其他操作后无法直接进入
					{
						pGbtMsgQueue->SendTscCommand(OBJECT_GOSTEP,0);
						ACE_DEBUG((LM_DEBUG,"%s:%d Send WirelessBtn Next Step TscMsg ! \n",__FILE__,__LINE__));
						pManaKernel->SndMsgLog(LOG_TYPE_MANUAL,6,0,0,0);
						m_ucLastManualSts = BUTTON_MANUAL_NEXT_STEP;
						bFirstOperate = true ;
					}
				}
				else if((RecvButtonType>>4)&0x1) //下一相位
				{
					if(m_ucLastManualSts == BUTTON_MANUAL_MANUAL || m_ucLastManualSts == BUTTON_MANUAL_NEXT_PHASE)  //下一步只能由手控后直接进入，其他操作后无法直接进入
					{
						pGbtMsgQueue->SendTscCommand(OBJECT_SWITCH_STAGE,0);
						ACE_DEBUG((LM_DEBUG,"%s:%d Send WirelessBtn Next Phase TscMsg !\n",__FILE__,__LINE__));
						if(m_ucLastManualSts != BUTTON_MANUAL_NEXT_PHASE)
						m_ucLastManualSts = BUTTON_MANUAL_NEXT_PHASE;	
						bFirstOperate = true ;
					}
				}
				else if((RecvButtonType>>5)&0x1)//下一方向
				{
					if(m_ucLastManualSts == BUTTON_MANUAL_MANUAL && m_ucAllowLampStatus == LAMP_GreenFLASH) 
					{
						SetCurrenStepFlash(0x1); //如果手动进入是绿闪则黄灯过渡
						ACE_OS::sleep(2);
					}
					else if(m_ucLastManualSts == BUTTON_MANUAL_NEXT_STEP)
					{
						bTransitSetp = true ;	
						SetCurrenStepFlash(0x2); //当前方向相位绿闪?
						ACE_OS::sleep(2);	
						SetCurrenStepFlash(0x1); //当前方向相位黄灯
						bTransitSetp = false ;
					}
					sTscMsg.ulType       = TSC_MSG_MANUALBUTTON_HANDLE; 
					sTscMsg.ucMsgOpt     = 0;
					sTscMsg.uiMsgDataLen = 0;			
					sTscMsg.pDataBuf     = ACE_OS::malloc(4);
					ACE_OS::memset((Byte*)sTscMsg.pDataBuf,0x0,4);
					((Byte*)sTscMsg.pDataBuf)[(m_ucdirectype++)%4] = 0x7 ;
					CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));	
					ACE_DEBUG((LM_DEBUG,"%s:%d Send Next WirelessBtn DIRE=%d !\n",__FILE__,__LINE__,m_ucdirectype-1));
					if(m_ucLastManualSts != BUTTON_MANUAL_NEXT_DIREC)
						m_ucLastManualSts = BUTTON_MANUAL_NEXT_DIREC;
					pManaKernel->m_pRunData->uiWorkStatus = STANDARD ;
				}
				else if(DirecButtonCfg != 0x0)
				{
					if(m_ucLastManualSts == BUTTON_MANUAL_MANUAL && m_ucAllowLampStatus == LAMP_GreenFLASH) 
					{
						SetCurrenStepFlash(0x1); //如果手动进入是绿闪则黄灯过渡
						ACE_OS::sleep(2);
					}
					else if(m_ucLastManualSts == BUTTON_MANUAL_NEXT_STEP)
					{
						bTransitSetp = true ;	
						SetCurrenStepFlash(0x2); //当前方向相位绿闪?
						ACE_OS::sleep(2);	
						SetCurrenStepFlash(0x1); //当前方向相位黄灯
						bTransitSetp = false ;
					}
					sTscMsg.ulType       = TSC_MSG_MANUALBUTTON_HANDLE; 
					sTscMsg.ucMsgOpt     = 0;
					sTscMsg.uiMsgDataLen = 4;			
					sTscMsg.pDataBuf     = ACE_OS::malloc(4);
					ACE_OS::memcpy((Byte*)sTscMsg.pDataBuf,&DirecButtonCfg,4);
					CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));
					if(m_ucLastManualSts != BUTTON_SPECIAL_DIREC)
						m_ucLastManualSts = BUTTON_SPECIAL_DIREC;
					pManaKernel->m_pRunData->uiWorkStatus = STANDARD ; //避免无法再次进入黄闪或全红
					}		
			}
		}
	}
	else
	{		
		ACE_OS::printf("%s:%d Being transit setp wait.........!\n",__FILE__,__LINE__);
	}
	return ;
}


/**************************************************************
Function:       CWirelessBtn::HandleSpecialDirec
Description:    处理无线手控组合方向的放行灯色
Input:           RecvDircData  -放行方向数据             
Output:         无
Return:         无
Date:            20141105
***************************************************************/
void CWirelessBtn::HandleSpecialDirec(Uint * RecvtmpDircData)
{
	Uint RecvDircData = *RecvtmpDircData ;	
	if(bTransitSetp == false)  //非过渡步
	{	
		if(bFirstOperate)
		{
			pManaKernel->SetWirelessBtnDirecCfg(RecvDircData,0x0);
			bFirstOperate = false ;
			RecvoldDirec = RecvDircData ;
			//ACE_OS::printf("%s:%d First special direc ,no Transit setp !\n",__FILE__,__LINE__);
		}
		else
		{
			bTransitSetp = true ;	
			pManaKernel->SetWirelessBtnDirecCfg(RecvoldDirec,0x2);
						
			//ACE_OS::printf("%s:%d Transit setp Green Flash sleep 2 sec!\n",__FILE__,__LINE__);
			ACE_OS::sleep(2);
			pManaKernel->SetWirelessBtnDirecCfg(RecvoldDirec,0x1);
						
			//ACE_OS::printf("%s:%d Transit setp Yellow  sleep 3 sec!\n",__FILE__,__LINE__);
			ACE_OS::sleep(2);
			pManaKernel->SetWirelessBtnDirecCfg(RecvDircData,0x0);
			bTransitSetp  = false ;
			RecvoldDirec = RecvDircData ;
		}			
	
	}

}

/**************************************************************
Function:       CWirelessBtn::SetCurrenStepFlash
Description:    设置当前步伐处于何种灯色状态，用于进入手控的时候过渡
Input:           LampColor--灯色状态           
Output:         无
Return:         无
Date:            20141105
***************************************************************/

bool  CWirelessBtn::SetCurrenStepFlash(Byte LampColor)
{
	//ACE_OS::printf("%s:%d First WirelessManual Flash!\n",__FILE__,__LINE__);
	bool bSetLampColor = false ;
	Byte LampColorStatus = 0 ;
	Byte m_ucLampOn[MAX_LAMP]={0}; 
	Byte m_ucLampFlash[MAX_LAMP]={0}; 
	ACE_OS::memcpy(m_ucLampOn,pManaKernel->m_pRunData->sStageStepInfo[pManaKernel->m_pRunData->ucStepNo].ucLampOn,MAX_LAMP);	
	ACE_OS::memcpy(m_ucLampFlash,pManaKernel->m_pRunData->sStageStepInfo[pManaKernel->m_pRunData->ucStepNo].ucLampFlash,MAX_LAMP);
	
	for(Byte LampIndex = 0 ;LampIndex< 60 ; LampIndex++)
	{
		LampColorStatus  = pManaKernel->m_pRunData->sStageStepInfo[pManaKernel->m_pRunData->ucStepNo].ucLampOn[LampIndex];
		if(LampColorStatus ==1)
		{
			 if(LampIndex%3 ==2)
			{
				if(LampColor ==0x2)//绿闪
					{
						m_ucLampFlash[LampIndex]=0x1;
						if(!bSetLampColor)
							bSetLampColor = true ;
						m_ucAllowLampStatus = LAMP_GreenFLASH;
					}
				if(LampColor == 0x1)//黄灯
				{		
					m_ucLampOn[LampIndex]=0;
					m_ucLampFlash[LampIndex]=0;
					
					if((LampIndex+1)%12==0) //人行
					{
						m_ucLampOn[LampIndex-2] = 0x1 ;
						
					}
					else
					{
						m_ucLampOn[LampIndex-1]=0x1;
						
					}
					if(!bSetLampColor)
							bSetLampColor = true ;
					m_ucAllowLampStatus = LAMP_YELLOW ;
				}
				if(LampColor == 0x3)//黄闪
				{	
					if(m_ucLampFlash[LampIndex]==0x1)
					{
						m_ucLampOn[LampIndex]=0x0;
						m_ucLampFlash[LampIndex]=0x0;
						if((LampIndex+1)%12==0) //人行不黄灯和闪烁
						{
							
							m_ucLampOn[LampIndex-2] = 0x1 ; 	 //人行亮红灯						
						}
						else
						{
							m_ucLampOn[LampIndex-1]=0x1;    //黄灯闪
							m_ucLampFlash[LampIndex-1]=0x1;
						}
						
						if(!bSetLampColor)
							bSetLampColor = true ;
						m_ucAllowLampStatus = LAMP_YELLOWFLASH ;
					}
				}
			
			}
			if(LampIndex%3 ==1)
			{
				if(LampColor == 0x3)//黄闪
				{
						if((LampIndex+2)%12==0) //人行不黄灯和闪烁
						{
							m_ucLampOn[LampIndex]=0x0;
							m_ucLampFlash[LampIndex]=0x0;
							m_ucLampOn[LampIndex-1] =0x1;
						}
						else
							m_ucLampFlash[LampIndex]=0x1;
					if(!bSetLampColor)
							bSetLampColor = true ;
					m_ucAllowLampStatus = LAMP_YELLOWFLASH;
				}

			}
		}
	}
	
	if(bSetLampColor)
		CLampBoard::CreateInstance()->SetLamp(m_ucLampOn,m_ucLampFlash);
}



/**************************************************************
Function:       CWirelessBtnTimer::CWirelessBtnTimer
Description:    CWirelessBtnTimer类构造函数
Input:           无             
Output:         无
Return:         无
Date:            20141023
***************************************************************/
CWirelessBtnTimer::CWirelessBtnTimer()
{
	m_uctimeout = 0 ;
	ACE_DEBUG((LM_DEBUG,"%s:%d Init WirelessBtnTimer object ok !\n",__FILE__,__LINE__));

}

/**************************************************************
Function:       CWirelessBtnTimer::~CWirelessBtnTimer
Description:    CTscTimer类析构函数
Input:          无               
Output:         无
Return:         无
Date:            20141023
***************************************************************/
CWirelessBtnTimer::~CWirelessBtnTimer()
{
	ACE_DEBUG((LM_DEBUG,"%s:%d Destruct CWirelessBtnTimer object ok !\n",__FILE__,__LINE__));
}

CWirelessBtnTimer* CWirelessBtnTimer::CreateInstance()
{
	static CWirelessBtnTimer cWirelessBtnTimer;
	return &cWirelessBtnTimer;
}


/**************************************************************
Function:       CWirelessBtnTimer::handle_timeout
Description:    信号机遥控器按钮定时器定时回调函数，1s执行一次。处理信号机
				多数定时工作。
Input:           默认处理，用户无需输入              
Output:         无
Return:         0
Date:            20141023
***************************************************************/
int CWirelessBtnTimer::handle_timeout(const ACE_Time_Value &tCurrentTime, const void * /* = 0 */)
{
	m_uctimeout++;
	//ACE_OS::printf("m_uctimeout= %d\r\n",m_uctimeout);
	if(m_uctimeout >= CManaKernel::CreateInstance()->m_pTscConfig->sSpecFun[FUN_WIRELESSBTN_TIMEOUT].ucValue*60)
	{
		CWirelessBtn::CreateInstance()->BackToAuto();
		//CTimerManager::CreateInstance()->CloseWirelessBtn();		
		m_uctimeout = 0 ;
		ACE_OS::printf("%s:%d WirelessBtn Manual Control timeout ,Back to AutoSelf Control!\r\n",__FILE__,__LINE__);
	}
	return 0 ;
}

/**************************************************************
Function:       CWirelessBtnTimer::Resettimeout
Description:    重置超时计数

Input:           无           
Output:         无
Return:         无
Date:            20141023
***************************************************************/

void CWirelessBtnTimer::Resettimeout()
{
	m_uctimeout = 0 ;
}


