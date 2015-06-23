
/***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   BusPriority.cpp
Date:       2015-5-18
Description:�źŻ����������Ȳ���
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

#define RFID_DATA_LEN 0x11 //RFID�Զ��ϴ���ǩ�ֽ���17λ
/**************************************************************
Function:        CBusPriority::CBusPriority
Description:     CGps�๹�캯������ʼ����			
Input:          ��           
Output:         ��
Return:         ��
***************************************************************/
CBusPriority::CBusPriority()
{
	ACE_DEBUG((LM_DEBUG,"%s:%d Init CBusPriority object !\r\n",__FILE__,__LINE__));
	
	 Byte m_iGpsFd  = CSerialCtrl::CreateInstance()->GetSerialFd2();
	 STscConfig* pTscConfig = CManaKernel::CreateInstance()->m_pTscConfig ;
	 b_OpenBusPriority = pTscConfig->sSpecFun[FUN_BUS_PRIORITY].ucValue ; ; //�Ƿ�����������
	 b_HandPriority = false ;   //�Ƿ��ڹ������ȴ��������

	 p_BusDelayTime    = pTscConfig->sSpecFun[FUN_BUS_PRIORITY_GREENDELAY].ucValue ; //Ĭ���̵��ӳ�ʱ��
	 p_BusRedLeftTime  = pTscConfig->sSpecFun[FUN_BUS_PRIORITY_NONBUSPHASEREDUCE].ucValue ; //Ĭ�Ϻ�������ǰʱ����
	 p_BusStandardTime = pTscConfig->sSpecFun[FUN_BUS_PRIORITY_DEFAULTTIME].ucValue; //Ĭ�Ϲ�������������ͨ��·����Ҫʱ����
	 p_CurrentRequestBusData.RfId = 0x1; //Ĭ����Ӧ��������	 
	 p_CurrentRequestBusData.BusPhaseId= pTscConfig->sSpecFun[FUN_BUS_PRIORITY_BUSPHASEID].ucValue ; //Ĭ����Ӧ��λ��
	 p_BusRedCutType = pTscConfig->sSpecFun[FUN_BUS_PRIORITY_EARLYRED].ucValue ;  //���������� 0-������λ��С��1-������λ�����ض�����ʱ��
}

/**************************************************************
Function:       CBusPriority::~CBusPriority
Description:    CBusPriority����������		
Input:          ��              
Output:         ��
Return:         ��
***************************************************************/
CBusPriority::~CBusPriority()
{
	ACE_DEBUG((LM_DEBUG,"%s:%d Destruct CBusPriority object!\r\n",__FILE__,__LINE__));
}

/**************************************************************
Function:       CBusPriority::CBusPriority
Description:    ����CGps�ྲ̬����		
Input:          ��              
Output:         ��
Return:         CBusPriority��̬����ָ��
***************************************************************/
CBusPriority* CBusPriority::CreateInstance()
{
	static CBusPriority CBusPriority;
	return &CBusPriority;
}

/**************************************************************
Function:       CBusPriority::GetBoolOpenBusPriority
Description:    ���ص�ǰ�Ƿ����ù������ȿ�����	
Input:           ��              
Output:         ��
Return:         true -���� false-δ����
Date:            2015-5-19  1035
***************************************************************/

bool CBusPriority::GetBoolOpenBusPriority()
{
	return b_OpenBusPriority ;
}

/**************************************************************
Function:       CBusPriority::Extract
Description:   ��������������		
Input:          ��              
Output:         ��
Return:         0-ʧ�� 1-�ɹ�
***************************************************************/
void CBusPriority::HandBusPriority(SBusPriorityData* pBusPriorityData)
{
	int iMinTime = 0x0 ;
	int iAddTime = 0x0 ;
	int iMaxTime = 0xff ;
	Byte BusPhaseId = pBusPriorityData->BusPhaseId;
	
	Byte BusPhaseColour = 0x0 ;
	Byte BusPhaseColoruTime = 0x0 ;
	
	CManaKernel *pManaKernel = CManaKernel::CreateInstance();
	Byte BusPhaseLeftTime = pManaKernel->m_pRunData->ucStepTime - pManaKernel->m_pRunData->ucElapseTime ;
	int CurrenStepNo = pManaKernel->m_pRunData->ucStepNo ;
	CGaCountDown::CreateInstance()->GetBusPhaseColourTime(BusPhaseId,BusPhaseColour ,BusPhaseColoruTime);
	
	iMinTime = pManaKernel->GetCurStepMinGreen(CurrenStepNo, &iMaxTime,&iAddTime);
	ACE_OS::printf("%s:%d BusPhaseId=%d BusPhaseColour=%d \r\n",__FILE__,__LINE__,BusPhaseId,BusPhaseColour);
	if(BusPhaseColour == GANEW_COLOR_GREEN)	
    {
		if(p_BusStandardTime<= BusPhaseLeftTime)
		{
			ACE_OS::printf("%s:%d p_BusStandardTime=%d BusPhaseLeftTime=%d StepTime=%d \r\n",__FILE__,__LINE__,p_BusStandardTime,BusPhaseLeftTime,pManaKernel->m_pRunData->ucStepTime);
			return ;
		}
		else if(p_BusStandardTime>BusPhaseLeftTime)
		{
			
			pManaKernel->m_pRunData->ucStepTime += p_BusDelayTime ;			
			if(pManaKernel->m_pRunData->ucStepTime > iMaxTime)
			   pManaKernel->m_pRunData->ucStepTime = iMaxTime ;	
			pManaKernel->m_pRunData->sStageStepInfo[CurrenStepNo].ucStepLen = pManaKernel->m_pRunData->ucStepTime ;
			pManaKernel->RefreshCycle();
			
			ACE_OS::printf("%s:%d BusPriority GreenDelay:NewStepTime=%d GreenDelayTime= %d ,BusPhaseLefTime=%d NewCycle=%d \n",__FILE__,__LINE__,pManaKernel->m_pRunData->ucStepTime,p_BusDelayTime,BusPhaseLeftTime,pManaKernel->m_pRunData->ucCycle);				
		}
	}
	else if(BusPhaseColour==GANEW_COLOR_RED)
	{
		Byte BusPhaseStageId = 0xff ;
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
						
						ACE_OS::printf("%s:%d BusPriority StageId= %d BusRedReduceTime=%d  StepNo=%d Steplen=%d \r\n",__FILE__,__LINE__,index,p_BusRedLeftTime,StepNum,pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen); 			
					}
					else if(p_BusRedCutType == 0x1)	
					{
						pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen -= p_BusRedLeftTime ;
						if(pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen <iMinTime )
							pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen = iMinTime ;
						
						ACE_OS::printf("%s:%d BusPriority StageId= %d BusRedReduceTime=%d  StepNo=%d Steplen=%d \r\n",__FILE__,__LINE__,index,p_BusRedLeftTime,StepNum,pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen); 			
					}
					
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
						
						ACE_OS::printf("%s:%d BusPriority StageId= %d BusRedReduceTime=%d  StepNo=%d Steplen=%d \r\n",__FILE__,__LINE__,index,p_BusRedLeftTime,StepNum,pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen); 			
					}
					else if(p_BusRedCutType == 0x1)	
					{
						pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen -= p_BusRedLeftTime ;
						if(pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen <iMinTime )
							pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen = iMinTime ;
						
						ACE_OS::printf("%s:%d BusPriority StageId= %d BusRedReduceTime=%d  StepNo=%d Steplen=%d \r\n",__FILE__,__LINE__,index,p_BusRedLeftTime,StepNum,pManaKernel->m_pRunData->sStageStepInfo[StepNum].ucStepLen); 			
					}
				}
			}
			
			pManaKernel->RefreshCycle();
		}
	}		
}

/**************************************************************
Function:        RunRecevBusPriority
Description:    ������������Ϣ
Input:           arg - �̺߳�������        
Output:         ��
Return:         0
Date:           20150518
***************************************************************/

 void *CBusPriority::RunRecevBusPriority(void *arg)
{
	ACE_OS::sleep(60); //�ȵȴ������߳�����	
	ACE_DEBUG((LM_DEBUG,"\n%s:%d ***THREAD*** Begin to run RunRecevBusPriority thread!\r\n",__FILE__,__LINE__));
	STscConfig* pTscConfig = CManaKernel::CreateInstance()->m_pTscConfig ;
	Byte bBusPriority = pTscConfig->sSpecFun[FUN_BUS_PRIORITY].ucValue ;
	Byte EpcDataBytes[RFID_DATA_LEN]={0};
	Byte ReadResult = 0x0 ;
	//ACE_OS::printf("%s:%d BusPriority Parameters: %d %d %d %d %d %d \r\n",__FILE__,__LINE__,bBusPriority,
	//	 pTscConfig->sSpecFun[FUN_BUS_PRIORITY_EARLYRED].ucValue, pTscConfig->sSpecFun[FUN_BUS_PRIORITY_GREENDELAY].ucValue,
	//	 pTscConfig->sSpecFun[FUN_BUS_PRIORITY_DEFAULTTIME].ucValue,pTscConfig->sSpecFun[FUN_BUS_PRIORITY_NONBUSPHASEREDUCE].ucValue,
	//	 pTscConfig->sSpecFun[FUN_BUS_PRIORITY_BUSPHASEID].ucValue);
	tcflush(CRs485::CreateInstance()->getPortFd(), TCIFLUSH); //������뻺����
	{		 
		while ( true ) 
		{	
			ReadResult = CRs485::CreateInstance()->ReadComPort(EpcDataBytes,RFID_DATA_LEN);
	
  			if(ReadResult == RFID_DATA_LEN && EpcDataBytes[0]==0x0 && EpcDataBytes[RFID_DATA_LEN-1]==0xFF)
  			{
  				ACE_OS::printf("%s:%d %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n",__FILE__,__LINE__,
				EpcDataBytes[0],EpcDataBytes[1],EpcDataBytes[2],EpcDataBytes[3],EpcDataBytes[4],EpcDataBytes[5],EpcDataBytes[6],EpcDataBytes[7],EpcDataBytes[8],
				EpcDataBytes[9],EpcDataBytes[10],EpcDataBytes[11],EpcDataBytes[12],EpcDataBytes[13],EpcDataBytes[14],
				EpcDataBytes[15],EpcDataBytes[16]);
				SThreadMsg sTscMsg ;
				sTscMsg.ulType       = TSC_MSG_BUSPRIORITY_HANDLE; 
				sTscMsg.ucMsgOpt     = 0;
				sTscMsg.uiMsgDataLen = 0;			
				sTscMsg.pDataBuf     = ACE_OS::malloc(sizeof(SBusPriorityData)) ;
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));	
			   ((SBusPriorityData*)(sTscMsg.pDataBuf))->BusPhaseId = pTscConfig->sSpecFun[FUN_BUS_PRIORITY_BUSPHASEID].ucValue;
  			}					
		}	
	}
	return NULL ;
}


