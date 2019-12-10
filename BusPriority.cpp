
/***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   BusPriority.cpp
Date:       2015-5-18
Description:信号机处理公交优先策略
Version:    V1.0
History:
***************************************************************/
#include "SerialCtrl.h"
#include "GbtMsgQueue.h"
#include "ManaKernel.h"
#include <termios.h>
#include <stdlib.h>
#include "BusPriority.h"
#include "GaCountDown.h"
#include "TscMsgQueue.h"
#include "ManaKernel.h"
#include "Rs485.h"
#include "Detector.h"

#define RFID_DATA_LEN 0x11 //RFID自动上传标签字节数17位
/**************************************************************
Function:        CBusPriority::CBusPriority
Description:     CGps类构造函数，初始化类			
Input:          无           
Output:         无
Return:         无
***************************************************************/
CBusPriority::CBusPriority() 
{
	 ACE_DEBUG((LM_DEBUG,"%s:%d Init CBusPriority object !\r\n",__FILE__,__LINE__));	
	 Byte m_iGpsFd  = CSerialCtrl::CreateInstance()->GetSerialFd2();
	 STscConfig* pTscConfig = CManaKernel::CreateInstance()->m_pTscConfig ;  //
	 b_OpenBusPriority = pTscConfig->sSpecFun[FUN_BUS_PRIORITY].ucValue ;    //是否开启公交优先
	 b_HandPriority = false ;   //是否处于公交优先处理过程中

	 p_BusDelayTime    = pTscConfig->sSpecFun[FUN_BUS_PRIORITY_GREENDELAY].ucValue ; //默认绿灯延长时秒
	 p_BusRedLeftTime  = pTscConfig->sSpecFun[FUN_BUS_PRIORITY_NONBUSPHASEREDUCE].ucValue ; //默认红灯早断提前时间秒
	 p_BusStandardTime = pTscConfig->sSpecFun[FUN_BUS_PRIORITY_DEFAULTTIME].ucValue; //默认公交车到达监测点后通过路口需要时间秒
	 p_CurrentRequestBusData.RfId = 0x1; //默认响应读卡器号	 
	 p_CurrentRequestBusData.BusPhaseId= pTscConfig->sSpecFun[FUN_BUS_PRIORITY_BUSPHASEID].ucValue ; //默认响应相位号
	 p_BusRedCutType = pTscConfig->sSpecFun[FUN_BUS_PRIORITY_EARLYRED].ucValue ;  //红灯早断类型 0-其他相位最小绿1-其他相位减少特定参数时间
}

/**************************************************************
Function:       CBusPriority::~CBusPriority
Description:    CBusPriority类析构函数		
Input:          无              
Output:         无
Return:         无
***************************************************************/
CBusPriority::~CBusPriority()
{
	ACE_DEBUG((LM_DEBUG,"%s:%d Destruct CBusPriority object!\r\n",__FILE__,__LINE__));
}

/**************************************************************
Function:       CBusPriority::CBusPriority
Description:    创建CGps类静态对象		
Input:          无              
Output:         无
Return:         CBusPriority静态对象指针
***************************************************************/
CBusPriority* CBusPriority::CreateInstance()
{
	static CBusPriority CBusPriority;
	return &CBusPriority;
}

/**************************************************************
Function:       CBusPriority::GetBoolOpenBusPriority
Description:    返回当前是否启用公交优先控制下	
Input:           无              
Output:         无
Return:         true -启用 false-未启用
Date:            2015-5-19  1035
***************************************************************/

bool CBusPriority::GetBoolOpenBusPriority()
{
	return b_OpenBusPriority ;
}

/**************************************************************
Function:       CBusPriority::Extract
Description:   处理公交优先数据		
Input:          无              
Output:         无
Return:         0-失败 1-成功
***************************************************************/
void CBusPriority::HandBusPriority(SBusPriorityData* pBusPriorityData)
{
	b_HandPriority = true ;
	int iMinTime = 0x0 ;
	int iAddTime = 0x0 ;
	int iMaxTime = 0xff ;
	
	Byte BusPhaseId = pBusPriorityData->BusPhaseId;
	
	Byte BusPhaseColour = 0x0 ;
	Byte BusPhaseColoruTime = 0x0 ;
	
	CManaKernel *pManaKernel = CManaKernel::CreateInstance();
	
	Byte BusPhaseLeftTime = pManaKernel->m_pRunData->ucStepTime - pManaKernel->m_pRunData->ucElapseTime ;
	int CurrenStepNo = pManaKernel->m_pRunData->ucStepNo ;
	//CGaCountDown::CreateInstance()->GetBusPhaseColourTime(BusPhaseId,BusPhaseColour ,BusPhaseColoruTime);
	BusPhaseColour=pManaKernel->m_pRunData->sStageStepInfo[pManaKernel->m_pRunData->ucStepNo].ucPhaseColor[BusPhaseId-1];
	iMinTime = pManaKernel->GetCurStepMinGreen(CurrenStepNo, &iMaxTime,&iAddTime);
	ACE_OS::printf("%s:%d BusPhaseId=%d BusPhaseColour=%d \r\n",__FILE__,__LINE__,BusPhaseId,BusPhaseColour);
	if(BusPhaseColour == 0x2 && pManaKernel->IsLongStep(CurrenStepNo) )	
    {
		if(p_BusStandardTime<= BusPhaseLeftTime)
		{
			ACE_OS::printf("%s:%d p_BusStandardTime=%d BusPhaseLeftTime=%d StepTime=%d \r\n",__FILE__,__LINE__,p_BusStandardTime,BusPhaseLeftTime,pManaKernel->m_pRunData->ucStepTime);
			sBusPriorityHanded.HandledType = 0x0 ;
			return ;
		}
		else if(p_BusStandardTime>BusPhaseLeftTime)
		{			
			pManaKernel->m_pRunData->ucStepTime += p_BusDelayTime ;			
			if(pManaKernel->m_pRunData->ucStepTime > iMaxTime)
			   pManaKernel->m_pRunData->ucStepTime = iMaxTime ;	
			pManaKernel->m_pRunData->sStageStepInfo[CurrenStepNo].ucStepLen = pManaKernel->m_pRunData->ucStepTime ;
			pManaKernel->RefreshCycle();
			sBusPriorityHanded.HandledType = 0x1 ;
			sBusPriorityHanded.HandledTime = p_BusDelayTime ;			
			ACE_OS::printf("%s:%d BusPriority GreenDelay:NewStepTime=%d GreenDelayTime= %d ,BusPhaseLefTime=%d NewCycle=%d \n",__FILE__,__LINE__,pManaKernel->m_pRunData->ucStepTime,p_BusDelayTime,BusPhaseLeftTime,pManaKernel->m_pRunData->ucCycle);				
		}
	}
	else if(BusPhaseColour==0x0 || BusPhaseColour==0x1)
	{
		Byte BusPhaseStageId = 0x0 ;
		Byte CurrenStageId = pManaKernel->m_pRunData->ucStageNo;
		BusPhaseStageId = pManaKernel->GetPhaseStageNo(BusPhaseId)-1;
			
		Byte StepNum = 0x0 ;
		if(BusPhaseStageId < CurrenStageId)
		{
			for(Byte index = CurrenStageId ; index< MAX_SON_SCHEDULE; index++)
			{
				StepNum = pManaKernel->StageToStep(index);
				if(pManaKernel->IsLongStep(StepNum) && index != BusPhaseStageId)
				{
					if(p_BusRedCutType == 0x0)
					{
						pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen = iMinTime ;
						sBusPriorityHanded.HandledType = 0x2 ;
						sBusPriorityHanded.HandledTime = iMinTime ;
						ACE_OS::printf("%s:%d BusPriority StageId= %d BusRedReduceTime=%d  StepNo=%d Steplen=%d \r\n",__FILE__,__LINE__,index,p_BusRedLeftTime,StepNum,pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen); 			
					}
					else if(p_BusRedCutType == 0x1)	
					{
						pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen -= p_BusRedLeftTime ;
						if(pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen <iMinTime )
							pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen = iMinTime ;
						sBusPriorityHanded.HandledType = 0x3 ;
						sBusPriorityHanded.HandledTime = p_BusRedLeftTime ;
						//ACE_OS::printf("%s:%d BusPriority StageId= %d BusRedReduceTime=%d  StepNo=%d Steplen=%d \r\n",__FILE__,__LINE__,index,p_BusRedLeftTime,StepNum,pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen); 			
					}
					if(StepNum == pManaKernel->m_pRunData->ucStepNo)
						pManaKernel->m_pRunData->ucStepTime = pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen ;
				}
			}			
			pManaKernel->RefreshCycle();
		}
		else if(BusPhaseStageId > CurrenStageId)
		{
			for(Byte index = CurrenStageId ; index< BusPhaseStageId ; index++)
			{
				StepNum = pManaKernel->StageToStep(index);
				if(pManaKernel->IsLongStep(StepNum))
				{
					if(p_BusRedCutType == 0x0)
					{
						pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen = iMinTime ;
						sBusPriorityHanded.HandledType = 0x2 ;
						sBusPriorityHanded.HandledTime = iMinTime ;
					ACE_OS::printf("%s:%d BusPriority StageId= %d BusRedReduceTime=%d  StepNo=%d Steplen=%d \r\n",__FILE__,__LINE__,index,p_BusRedLeftTime,StepNum,pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen); 			
					}
					else if(p_BusRedCutType == 0x1)	
					{
						if(pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen > p_BusRedLeftTime)
							pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen -= p_BusRedLeftTime ;
						else
							pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen = iMinTime ;
							if(pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen <iMinTime )
								pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen = iMinTime ;
						
						sBusPriorityHanded.HandledType = 0x3 ;
						sBusPriorityHanded.HandledTime = pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen ;
						ACE_OS::printf("%s:%d BusPriority StageId= %d BusRedReduceTime=%d  StepNo=%d Steplen=%d \r\n",__FILE__,__LINE__,index,p_BusRedLeftTime,StepNum,pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen); 			
					}					
					if(StepNum == pManaKernel->m_pRunData->ucStepNo)
						pManaKernel->m_pRunData->ucStepTime = pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen ;
				}
			}			
			pManaKernel->RefreshCycle(); 
		}
	}		
	   sBusPriorityHanded.CycleTime = pManaKernel->m_pRunData->ucCycle ;
	 
}

/**************************************************************
Function:        RunRecevBusPriority
Description:    处理公交优先信息
Input:           arg - 线程函数参数        
Output:         无
Return:         0
Date:           20150518
***************************************************************/

 void *CBusPriority::RunRecevBusPriority(void *arg)
{
	ACE_OS::sleep(60); //先等待其他线程运行	
	ACE_DEBUG((LM_DEBUG,"\n%s:%d ***THREAD*** Begin to run RunRecevBusPriority thread!\r\n",__FILE__,__LINE__));
	STscConfig* pTscConfig = CManaKernel::CreateInstance()->m_pTscConfig ;
	Byte bBusPriority = pTscConfig->sSpecFun[FUN_BUS_PRIORITY].ucValue ;
	Byte EpcDataBytes[RFID_DATA_LEN]={0};
	Byte ReadResult = 0x0 ;
	if(bBusPriority == false)
		return NULL ;
	//tcflush(CRs485::CreateInstance()->getPortFd(), TCIFLUSH); //清空输入缓冲区
	while (true) 
	{	
		//ReadResult = CRs485::CreateInstance()->ReadComPort(EpcDataBytes,RFID_DATA_LEN);
		//srand((unsigned)time(NULL));
		
  		//if(ReadResult == RFID_DATA_LEN && EpcDataBytes[0]==0x0 && EpcDataBytes[RFID_DATA_LEN-1]==0xFF)
		ACE_OS::sleep(ACE_Time_Value(0, 1000000)); //暂停10毫秒
		if(CDetector::CreateInstance()->IsHaveCarPhase(pTscConfig->sSpecFun[FUN_BUS_PRIORITY_BUSPHASEID].ucValue))
		{  			
		
			ACE_DEBUG((LM_DEBUG,"\r\n%s:%d Have bus RunRecevBusPriority r\n",__FILE__,__LINE__));
			SThreadMsg sTscMsg ;
			sTscMsg.ulType       = TSC_MSG_BUSPRIORITY_HANDLE; 
			sTscMsg.ucMsgOpt     = 0;
			sTscMsg.uiMsgDataLen = 0;			
			sTscMsg.pDataBuf     = ACE_OS::malloc(sizeof(SBusPriorityData)) ;
			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));	
			((SBusPriorityData*)(sTscMsg.pDataBuf))->BusPhaseId = pTscConfig->sSpecFun[FUN_BUS_PRIORITY_BUSPHASEID].ucValue;
		}			
	}	
	
	return NULL ;
}


