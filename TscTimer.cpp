
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
#include "Manual.h"
#include "GaCountDown.h"
//#include "ComStruct.h"
/************************ADD:201309231530***************************/
static CMainBoardLed* pMainBoardLed = CMainBoardLed::CreateInstance();
static CLampBoard*    pLamp   = CLampBoard::CreateInstance(); 
static CPowerBoard*   pPower = CPowerBoard::CreateInstance();
static CManaKernel*   pWorkParaManager = CManaKernel::CreateInstance();
static CMacControl*   pMacControl      = CMacControl::CreateInstance();  	 // MOD:0514 9:41
static Manual*        pManual      = Manual::CreateInstance();               // ADD:0514 9:42	
static CDetector*      pDetector    = CDetector::CreateInstance() ;  		 //ADD: 20130709 945	
static CPscMode * pCPscMode = CPscMode::CreateInstance() ;	
static STscRunData* pRunData = pWorkParaManager->m_pRunData ;
static CGaCountDown *pGaCountDown = CGaCountDown::CreateInstance();
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
	
	pManual->DoManual() ;     // ADD:0514 9:42	
	switch ( m_ucTick )
	{
	case 0: 		
		ChooseDecTime();		
		pLamp->SendLamp();
		pMainBoardLed->DoRunLed();  
		break;
	case 1:
		//pMacControl->GetEnvSts(); 
		//pFlashMac->FlashHeartBeat(); //ADD: 0604 17 28		
		if((pRunData->uiCtrl == CTRL_VEHACTUATED || pRunData->uiCtrl == CTRL_MAIN_PRIORITY || pRunData->uiCtrl == CTRL_SECOND_PRIORITY || pRunData->uiCtrl == CTRL_ACTIVATE )&&  pRunData->uiWorkStatus == STANDARD)
			pDetector->SearchAllStatus();  //ADD: 2013 0723 1620
		pMacControl->SndLcdShow() ; //ADD:201309281710
		break;
	case 2: 		

		pPower->CheckVoltage();
		break;

	case 3:
		
		if(ucModeType != MODE_TSC &&  pWorkParaManager->m_bFinishBoot)
		{			
			pCPscMode->DealButton();
			pMacControl->GetEnvSts();			
			
		}
		break;

	case 4:
		//if((pRunData->uiCtrl == CTRL_VEHACTUATED ||pRunData->uiCtrl == CTRL_ACTIVATE )&&  pRunData->uiWorkStatus == STANDARD)
		//	pDetector->GetOccupy();  //
		if((pRunData->uiCtrl == CTRL_VEHACTUATED || pRunData->uiCtrl == CTRL_MAIN_PRIORITY || pRunData->uiCtrl == CTRL_SECOND_PRIORITY ||pRunData->uiCtrl == CTRL_ACTIVATE )&&  pRunData->uiWorkStatus == STANDARD)
		{
			pDetector->IsVehileHaveCar(); //濡傛灉鏈夎溅鍒欏鍔犻暱姝ユ斁琛岀浉浣嶇殑缁跨伅鏃堕棿 鏈�澶т负鏈�澶х豢鏃堕棿
		}
		break;

	case 5://500ms 执行一�		
	    if( pWorkParaManager->m_pTscConfig->sSpecFun[FUN_COUNT_DOWN].ucValue == 2 )
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
			
		pMainBoardLed->DoRunLed();	
		break;
	case 6:
		pMainBoardLed->DoLedBoardShow();   //ADD :2013 0809 1600
		
		break;
	case 7://700??????????ì????????????????????can×??????
	
		pPower->HeartBeat();  //????????		
		break;

	case 8:	
		
		if(ucModeType != MODE_TSC && pWorkParaManager->m_bFinishBoot)
		{
			pCPscMode->DealButton();
			pMacControl->GetEnvSts();
		}	
		break;

	case 9:
		if(pWorkParaManager->m_pRunData->bIsChkLght == true )
			pLamp->CheckLight();// check Lampboard status and red&green conflict
		
		break;

	default:
	
		break;
	}
	m_ucTick++;
	if ( m_ucTick >= m_ucMaxTick )  //100??????????????±??÷,10????1????
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

