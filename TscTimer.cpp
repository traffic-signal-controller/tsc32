
/***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   TscTimer.cpp
Date:       2013-1-1
Description:信号机定时器相关操作处理。
Version:    V1.0
History:
***************************************************************/

#include "TscTimer.h"
#include "ManaKernel.h"
#include "MainBoardLed.h"
#include "Detector.h"
#include "PowerBoard.h"
#include "WatchDog.h"
#include "FlashMac.h"
#include "PscMode.h"
#include "LampBoard.h"
#include "MacControl.h"
#include "WirelessButtons.h"
#include "GaCountDown.h"
#include "MainBackup.h"



/************************ADD:201309231530***************************/
static CMainBoardLed* pMainBoardLed = CMainBoardLed::CreateInstance();
static CLampBoard*    pLamp   = CLampBoard::CreateInstance(); 
static CPowerBoard*   pPower = CPowerBoard::CreateInstance();
static CManaKernel*   pWorkParaManager = CManaKernel::CreateInstance();
static CMacControl*   pMacControl      = CMacControl::CreateInstance();  	 // MOD:0514 9:41
static MainBackup*        pMainBackup      = MainBackup::CreateInstance();               // ADD:0514 9:42	
static CDetector*      pDetector    = CDetector::CreateInstance() ;  		 //ADD: 20130709 945	
static CPscMode * pCPscMode = CPscMode::CreateInstance() ;	
static CGaCountDown *pGaCountDown = CGaCountDown::CreateInstance();
static STscRunData* pRunData = pWorkParaManager->m_pRunData ;
/************************ADD:201309231530***************************/	

	
/**************************************************************
Function:       CTscTimer::CTscTimer
Description:    CTscTimer类构造函数
Input:          ucMaxTick                 
Output:         无
Return:         无
***************************************************************/
CTscTimer::CTscTimer(Byte ucMaxTick)
{
	m_ucTick    = 0;
	m_ucMaxTick = ucMaxTick;
	m_bWatchdog = true;    		//开启看门狗
   	//m_bWatchdog = false;      //临时关闭看门狗   //MOD:20130522 1135
	if ( m_bWatchdog )
	{
		WatchDog::CreateInstance()->OpenWatchdog();
	}
	ACE_DEBUG((LM_DEBUG,"%s:%d Init TscTimer object ok !\n",__FILE__,__LINE__));

}

/**************************************************************
Function:       CTscTimer::~CTscTimer
Description:    CTscTimer类析构函数
Input:          无               
Output:         无
Return:         无
***************************************************************/
CTscTimer::~CTscTimer()
{
	ACE_DEBUG((LM_DEBUG,"%s:%d Destruct TscTimer object ok !\n",__FILE__,__LINE__));
}


/**************************************************************
Function:       CTscTimer::handle_timeout
Description:    信号机定时器定时回调函数，100ms执行一次。处理信号机
				多数定时工作。
Input:           默认处理，用户无需输入              
Output:         无
Return:         0
***************************************************************/
int CTscTimer::handle_timeout(const ACE_Time_Value &tCurrentTime, const void * /* = 0 */)
{
	
	Byte ucModeType = pWorkParaManager->m_pTscConfig->sSpecFun[FUN_CROSS_TYPE].ucValue ; //ADD: 2013 0828 0931

	if((pRunData->uiCtrl == CTRL_VEHACTUATED ||pRunData->uiCtrl == CTRL_ACTIVATE )&&  pRunData->uiWorkStatus == STANDARD)
		pDetector->SearchAllStatus();  //ADD: 2013 0723 1620
		
	//手控按钮每100ms侦查一次  // ADD:0514 9:42
	//pMainBackup->DoManual();
	//pMainBackup->Recevie();
	
	switch ( m_ucTick )
	{
	case 0: 
		//核心板发送心跳给，备份单片机。500ms   。另外 在case 5调用
		pMainBackup->HeartBeat();
		ChooseDecTime();
		pLamp->SendLamp();//4	////4个灯控板信息发送
		//pMainBoardLed->DoRunLed();  
		break;
	case 1:
		//pMacControl->GetEnvSts(); 
		//pFlashMac->FlashHeartBeat(); //ADD: 0604 17 28
		if((pRunData->uiCtrl == CTRL_VEHACTUATED || pRunData->uiCtrl == CTRL_MAIN_PRIORITY || pRunData->uiCtrl == CTRL_SECOND_PRIORITY || pRunData->uiCtrl == CTRL_ACTIVATE )&&  pRunData->uiWorkStatus == STANDARD)
			pDetector->SearchAllStatus();  //ADD: 2013 0723 1620
		pMacControl->SndLcdShow() ; //ADD:201309281710
		
		break;
	case 2: 		
		CPowerBoard::iHeartBeat++;
		if(CPowerBoard::iHeartBeat >2)
		{
			//ACE_DEBUG((LM_DEBUG,"\n%s:%d 重起Can总线!\n",__FILE__,__LINE__));
			//ACE_OS::system("up link set can0 up type can restart");
			CPowerBoard::iHeartBeat = 0;	
			//pWorkParaManager->SndMsgLog(LOG_TYPE_CAN,0,0,0,0);			
		}
		pPower->CheckVoltage();

		//手控按钮每100ms侦查一次  // ADD:0514 9:42
		pMainBackup->DoManual();
		break;

	case 3:
		if(ucModeType != MODE_TSC &&  pWorkParaManager->m_bFinishBoot)
		{
			pCPscMode->DealButton();
		}
		break;

	case 4:
		//if((pRunData->uiCtrl == CTRL_VEHACTUATED ||pRunData->uiCtrl == CTRL_ACTIVATE ) &&  pRunData->uiWorkStatus == STANDARD)
		//	pDetector->GetOccupy();  //
		if((pRunData->uiCtrl == CTRL_VEHACTUATED || pRunData->uiCtrl == CTRL_MAIN_PRIORITY || pRunData->uiCtrl == CTRL_SECOND_PRIORITY ||pRunData->uiCtrl == CTRL_ACTIVATE )&&  pRunData->uiWorkStatus == STANDARD)
		{
			pDetector->IsVehileHaveCar(); //如果有车则增加长步放行相位的绿灯时间 最大为最大绿时间
		}
		break;
	case 5://500ms 执行一次
		if( pWorkParaManager->m_pTscConfig->sSpecFun[FUN_COUNT_DOWN].ucValue == 2 ) //这里2表示闪断式倒计时
		{	
			if ( (SIGNALOFF == pRunData->uiWorkStatus)|| (ALLRED== pRunData->uiWorkStatus) 
			|| (FLASH   == pRunData->uiWorkStatus)|| (CTRL_MANUAL == pRunData->uiCtrl) 
			|| (CTRL_PANEL == pRunData->uiCtrl ))
			{
				pLamp->SendLamp();
			}
			else
			{
				Byte ucMode = 0 ;
				Byte ucPhaseId = 0 ;
				Byte ucOverPhaseId = 0 ;
				for(Byte nIndex = 0 ; nIndex< MAX_CNTDOWNDEV;nIndex++)
				{	
				    ucMode = pWorkParaManager->m_pTscConfig->sCntDownDev[nIndex].ucMode ;
					ucPhaseId = pWorkParaManager->m_pTscConfig->sCntDownDev[nIndex].usPhase ;
					ucOverPhaseId = pWorkParaManager->m_pTscConfig->sCntDownDev[nIndex].ucOverlapPhase ;
					if(((ucMode>>7)&0x1) ==0x1 &&(ucPhaseId != 0 || ucOverPhaseId !=0 ) )
					{					
						Byte ucSignalGrpNum = 0;					
						Byte ucSignalGrp[MAX_CHANNEL] = {0};
						Byte nChannelIndex = 0 ;
				
						pWorkParaManager->GetSignalGroupId(ucPhaseId?true:false,ucPhaseId?ucPhaseId:ucOverPhaseId,&ucSignalGrpNum,ucSignalGrp);
						while(nChannelIndex<ucSignalGrpNum)
						{
							Byte ucChannelId = ucSignalGrp[nChannelIndex] ;
							if(ucChannelId >0)
								pGaCountDown->m_ucLampBoardFlashBreak[(ucChannelId-1)/MAX_LAMPGROUP_PER_BOARD]|= ((1<<((ucChannelId-1)%MAX_LAMPGROUP_PER_BOARD))|0x60);
							nChannelIndex++ ;						
						}
						pWorkParaManager->m_pTscConfig->sCntDownDev[nIndex].ucMode &=0x7f ;
						
					}
				}	
				
				for(Byte index = 0 ;index < MAX_LAMP_BOARD-3 ;index++)
				{				
					pLamp->SendSingleLamp(index,pGaCountDown->m_ucLampBoardFlashBreak[index]);
					if(pGaCountDown->m_ucLampBoardFlashBreak[index] != 0x0)
					{
						pGaCountDown->m_ucLampBoardFlashBreak[index] = 0x0 ;
					}					
					
				}
			}
		}
		else
		{
			pLamp->SendLamp(); 
		}
			
		//pLamp->SendLamp();		//给所有灯控板发送灯色数据
		//pMainBoardLed->DoRunLed();
		//核心板发送心跳给，备份单片机。500ms   。另外 在case 1调用
		pMainBackup->HeartBeat();
		break;
	case 6:
		//pFlashMac->FlashHeartBeat() ;
		pMainBoardLed->DoLedBoardShow();   //ADD :2013 0809 1600
		
		break;
	case 7://700ms 发送心跳数据给电源板
		pPower->HeartBeat();
		//手控按钮每100ms侦查一次  // ADD:0514 9:42
		pMainBackup->DoManual();
		break;

	case 8:	
		if(ucModeType != MODE_TSC && pWorkParaManager->m_bFinishBoot)
			pCPscMode->DealButton();
		break;
	case 9:
		if(pWorkParaManager->m_pRunData->bIsChkLght == true )
			pLamp->CheckLight();// check Lampboard status and red&green conflict
		//ACE_DEBUG((LM_DEBUG,"\n%s:%d pMainBackup->bSendStep = %d!\n",__FILE__,__LINE__,pMainBackup->bSendStep));
		if(pMainBackup->bSendStep)
			pMainBackup->SendStep();
		break;
	default:
	
		break;
	}
	m_ucTick++;
	if ( m_ucTick >= m_ucMaxTick )  ////100毫秒定时器,10次1秒
	{
		if ( m_bWatchdog )
		{
			WatchDog::CreateInstance()->FillWatchdog('A');
		}
		m_ucTick = 0;
	}
	
	return 0;
}


/**************************************************************
Function:       CTscTimer::ChooseDecTime
Description:    选择由PSC或者TSC处理DecTime操作 1秒钟执行一次 
				在TSC定时器里面1秒钟执行一次 ，TSC模式下调用
				DecTime改变步长				
Input:          无              
Output:         无
Return:         无
***************************************************************/
void CTscTimer::ChooseDecTime()
{
	//static CManaKernel* pWorkParaManager = CManaKernel::CreateInstance(); DEL:201309231530
	static bool bPsc = true;
	if ( CTRL_MANUAL == pWorkParaManager->m_pRunData->uiCtrl 
	  || CTRL_PANEL  == pWorkParaManager->m_pRunData->uiCtrl 
	  || SIGNALOFF   == pWorkParaManager->m_pRunData->uiWorkStatus  
	  || ALLRED      == pWorkParaManager->m_pRunData->uiWorkStatus 
	  || FLASH       == pWorkParaManager->m_pRunData->uiWorkStatus  )
	{
		if ( bPsc )
		{
			pWorkParaManager->SetCycleBit(false);
			bPsc = false;
		}
		pWorkParaManager->DecTime();
		
	}
	else if ( pWorkParaManager->m_bFinishBoot 	&& pWorkParaManager->m_pTscConfig->sSpecFun[FUN_CROSS_TYPE].ucValue != MODE_TSC	)
	{
	if ((MODE_PSC1 == pWorkParaManager->m_pTscConfig->sSpecFun[FUN_CROSS_TYPE].ucValue &&2 != pWorkParaManager->m_pRunData->ucStageCount)
	  ||(MODE_PSC2 == pWorkParaManager->m_pTscConfig->sSpecFun[FUN_CROSS_TYPE].ucValue &&3 != pWorkParaManager->m_pRunData->ucStageCount))
		{
			//ACE_DEBUG((LM_DEBUG,"\n%s:%d SpecFun[FUN_CROSS_TYPE]=%d ,m_pRunData->ucStageCount = %d \n",__FILE__,__LINE__,pWorkParaManager->m_pTscConfig->sSpecFun[FUN_CROSS_TYPE].ucValue, pWorkParaManager->m_pRunData->ucStageCount));
			pWorkParaManager->DecTime();
		}
		else if ( pWorkParaManager->m_bCycleBit || bPsc )
		{
			/*
			if(bPsc)
				ACE_DEBUG((LM_DEBUG,"\n%s:%d bPsc == true!\n",__FILE__,__LINE__));
			else
				ACE_DEBUG((LM_DEBUG,"\n%s:%d bPsc == false!\n",__FILE__,__LINE__));
			if(pWorkParaManager->m_bCycleBit)
				ACE_DEBUG((LM_DEBUG,"\n%s:%d m_bCycleBit == true!\n",__FILE__,__LINE__));
			else
				ACE_DEBUG((LM_DEBUG,"\n%s:%d m_bCycleBit == false!\n",__FILE__,__LINE__));
			*/
			if ( !bPsc ) //手动完后 再走完到下一个周期 重新PSC
			{
				CPscMode::CreateInstance()->InitPara();
				bPsc = true;
			}
			CPscMode::CreateInstance()->DecTime();
		}
		else
		{
			pWorkParaManager->DecTime();
		}
	} 
	else
	{
		pWorkParaManager->DecTime();
	}
}

