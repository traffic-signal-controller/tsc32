 /***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   LampBoard.cpp
Date:       2013-4-13
Description:信号机灯手控操作处理类文件.包含对手控面板全红，黄闪，步进,手动自动切换按钮操作的处理。
Version:    V1.0
History:    2013.05.29 修改按钮按下和松开的处理。
		  2013.05.31 添加当手动状态维护5分钟以上时自动切换为自动状态.。
		  2013.09.10 修改按钮接收处理方式，简化代码。
		  2014.10.06 修改按钮接受处理方式，由位移循环判断修改位读取键值进行判断
		  2014.10.23 �����֧�����߰����ֿؿ���
********************************************************************************************/
#include "WirelessButtons.h"
#include "TscMsgQueue.h"
#include "ace/Date_Time.h"
#include "MainBoardLed.h"
#include "TimerManager.h"
#include "GbtMsgQueue.h"
#include "Can.h"
enum{
	 BUTTON_MANUAL_SELF 		= 0x0,	//��������
	 BUTTON_MANUAL_MANUAL 		= 0x1,	//�ֶ�����	
	 BUTTON_MANUAL_YELLOW_FLASH	= 0x2,	//����
	 BUTTON_MANUAL_ALL_RED		= 0x3,	//ȫ��
	 BUTTON_MANUAL_NEXT_STEP	= 0x4,	//��һ��
	 BUTTON_MANUAL_NEXT_PHASE	= 0x5,	//��һ��λ
	 BUTTON_MANUAL_NEXT_DIREC	= 0x6,	//��һ����
	 BUTTON_MANUAL_DIREC		= 0x7,	//��ָ���������λ��������
	
};

/**************************************************************
Function:       CWirelessBtn::CWirelessBtn
Description:    CWirelessBtn类构造函数，用于类初始化处理				
Input:          无              
Output:         无
Return:         无
***************************************************************/
CWirelessBtn::CWirelessBtn() 
{	
	m_ucLastManualSts = BUTTON_MANUAL_SELF; //��ʼ�������Զ�����״̬	
	m_ucdirectype = 0x0 ; 					//�������� 0-���� 1- ���� 2- �Ϸ� 3-����	
    pManaKernel = CManaKernel::CreateInstance();
}

/**************************************************************
Function:       CWirelessBtn::~CWirelessBtn
Description:    CWirelessBtn类	析构函数	
Input:          无              
Output:         无
Return:         无
***************************************************************/
CWirelessBtn::~CWirelessBtn() 
{
	;
}

/**************************************************************
Function:       Manual::CreateInstance
Description:    创建	Manual静态对象
Input:          无              
Output:         无
Return:         静态对象指针
***************************************************************/
CWirelessBtn* CWirelessBtn::CreateInstance()
{
	static CWirelessBtn cWirelessBtn;
	return &cWirelessBtn;
}

/****************************************************************
Function:       CWirelessBtn::EntryWirelessManul
Description:    �˳�����ң�ذ������ָ���������
Input:           ��          
Output:         ��
Return:         ��
Date:           20141023
*****************************************************************/

void CWirelessBtn::BackToAuto()
{
	CMainBoardLed::CreateInstance()->DoAutoLed(true);			
	if( pManaKernel->m_iTimePatternId == 250)
	{
		pManaKernel->bNextDirec = false ;
		pManaKernel->m_iTimePatternId = 0;
		pManaKernel->bTmpPattern = false ;
	}
	CGbtMsgQueue::CreateInstance()->SendTscCommand(OBJECT_SWITCH_SYSTEMCONTROL,0);
	ACE_DEBUG((LM_DEBUG,"%s:%d **************Back to BUTTON_MANUAL_SELF TscMsg! \n",__FILE__,__LINE__));
	m_ucLastManualSts = BUTTON_MANUAL_SELF;
	pManaKernel->m_pRunData->ucManualType = Manual_CTRL_NO;

}



/****************************************************************
*
Function:       CWirelessBtn::EntryWirelessManul
Description:    ���ν������߰����ֶ�״̬
Input:           ��          
Output:         ��
Return:         ��
Date:           20141023
*****************************************************************/

void CWirelessBtn::EntryWirelessManul()
{
	CMainBoardLed::CreateInstance()->DoAutoLed(false);					
	CGbtMsgQueue::CreateInstance()->SendTscCommand(OBJECT_CURTSC_CTRL,1);
	ACE_DEBUG((LM_DEBUG,"%s:%d First Entry  WirelessBtnManual TscMsg! \n",__FILE__,__LINE__));
	m_ucLastManualSts = BUTTON_MANUAL_MANUAL;
	pManaKernel->m_pRunData->ucManualType = Manual_CTRL_WIRELESSBUTTONS;

}

/****************************************************************
*
Function:       CWirelessBtn::RecvMacCan
Description:    �Դ�ң������������İ���CAN���ݽ��д���
Input:            Can���߽��յ������߰�������֡            
Output:         ��
Return:         ��
Date:           20141023
*****************************************************************/
void CWirelessBtn::RecvMacCan(SCanFrame sRecvCanTmp)
{

	if(pManaKernel->m_pRunData->ucManualType != Manual_CTRL_NO && pManaKernel->m_pRunData->ucManualType != Manual_CTRL_WIRELESSBUTTONS)
		return ;	
	CGbtMsgQueue *pGbtMsgQueue = CGbtMsgQueue::CreateInstance();
	Byte RecvType = sRecvCanTmp.pCanData[0] & 0x3F ;
	Byte RecvButtonType = sRecvCanTmp.pCanData[1] ;
	Uint DirecButtonCfg = ((sRecvCanTmp.pCanData[2]|sRecvCanTmp.pCanData[3]<<8)|sRecvCanTmp.pCanData[4]<<16)|sRecvCanTmp.pCanData[5]<<24;
	if(RecvType == 0x2)
	{		
		Byte iAuto_Manul = RecvButtonType&0x1 ;		
		if(iAuto_Manul == 0x0) //�Զ�״̬
		{
			if(m_ucLastManualSts == BUTTON_MANUAL_SELF) //����ϴ�״̬�Ѿ�����������
				return ;
			else
			{
				BackToAuto();
				CTimerManager::CreateInstance()->CloseWirelessBtn();				
				CWirelessBtnTimer::CreateInstance()->Resettimeout();  //�˳���ʱ����ʱ������Ϊ0
			}
		}
		else if(iAuto_Manul == 0x1) //�ֿ�״̬
		{				
			if(m_ucLastManualSts == BUTTON_MANUAL_SELF) //����ϴ�״̬����������
			{
				EntryWirelessManul();
				CTimerManager::CreateInstance()->StartWirelessBtnTimer();
				
				//ACE_OS::printf("%s:%d WirelessBtn timeout = %d seconds!\r\n",CManaKernel::CreateInstance()->m_pTscConfig->sSpecFun[FUN_WIRELESSBTN_TIMEOUT].ucValue*60);
			}		
			else
			{									
				CWirelessBtnTimer::CreateInstance()->Resettimeout();  //���ڽ��������ֿز����򣬳�ʱ������Ϊ0
				if((RecvButtonType>>1)&0x1)  //�����������
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
					}
				}
				else if((RecvButtonType>>2)&0x1)//����ȫ�����
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
					}
				}
				else if((RecvButtonType>>3)&0x1)//�����һ������
				{
					pGbtMsgQueue->SendTscCommand(OBJECT_GOSTEP,0);
					ACE_DEBUG((LM_DEBUG,"%s:%d Send WirelessBtn Next Step TscMsg ! \n",__FILE__,__LINE__));
					pManaKernel->SndMsgLog(LOG_TYPE_MANUAL,6,0,0,0);
					if(m_ucLastManualSts != BUTTON_MANUAL_NEXT_STEP)
						m_ucLastManualSts = BUTTON_MANUAL_NEXT_STEP;
				}
				else if((RecvButtonType>>4)&0x1) //��һ��λ
				{
					pGbtMsgQueue->SendTscCommand(OBJECT_SWITCH_STAGE,0);
					ACE_DEBUG((LM_DEBUG,"%s:%d Send WirelessBtn Next Phase TscMsg !\n",__FILE__,__LINE__));
					if(m_ucLastManualSts != BUTTON_MANUAL_NEXT_PHASE)
						m_ucLastManualSts = BUTTON_MANUAL_NEXT_PHASE;	
				}
				else if((RecvButtonType>>5)&0x1)//��һ����
				{
					if(pManaKernel->m_iTimePatternId == 0)
					{	
						pManaKernel->bTmpPattern = true ;
						pManaKernel->m_iTimePatternId = 250;//�����ķ����л�				
						ACE_DEBUG((LM_DEBUG,"%s:%d Send WirelessBtn TSC_MSG_TIMEPATTERN -> 250 TscMsg !\n",__FILE__,__LINE__));
					}			
					sTscMsg.ulType       = TSC_MSG_PATTER_RECOVER; 
					sTscMsg.ucMsgOpt     = (m_ucdirectype++)%4;
					sTscMsg.uiMsgDataLen = 0;			
					sTscMsg.pDataBuf     = NULL; 			
					CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));	
					ACE_DEBUG((LM_DEBUG,"%s:%d Send Next WirelessBtn DIRE=%d !\n",__FILE__,__LINE__,m_ucdirectype-1));
					if(m_ucLastManualSts != BUTTON_MANUAL_NEXT_DIREC)
						m_ucLastManualSts = BUTTON_MANUAL_NEXT_DIREC;
				}
				else if(DirecButtonCfg != 0x0)
				{
					pManaKernel->SetWirelessBtnDirecCfg(DirecButtonCfg);
				}		
			}
		}
	}
	return ;
}


	
/**************************************************************
Function:       CWirelessBtnTimer::CWirelessBtnTimer
Description:    CWirelessBtnTimer�๹�캯��
Input:           ��             
Output:         ��
Return:         ��
Date:            20141023
***************************************************************/
CWirelessBtnTimer::CWirelessBtnTimer()
{
	m_uctimeout = 0 ;
	ACE_DEBUG((LM_DEBUG,"%s:%d Init WirelessBtnTimer object ok !\n",__FILE__,__LINE__));

}

/**************************************************************
Function:       CWirelessBtnTimer::~CWirelessBtnTimer
Description:    CTscTimer����������
Input:          ��               
Output:         ��
Return:         ��
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
Description:    �źŻ�ң������ť��ʱ����ʱ�ص�������1sִ��һ�Ρ������źŻ�
				������ʱ������
Input:           Ĭ�ϴ����û���������              
Output:         ��
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
		CTimerManager::CreateInstance()->CloseWirelessBtn();		
		m_uctimeout = 0 ;
		ACE_OS::printf("%s:%d WirelessBtn Manual Control timeout ,Back to AutoSelf Control!\r\n",__FILE__,__LINE__);
	}
	return 0 ;
}

/**************************************************************
Function:       CWirelessBtnTimer::Resettimeout
Description:    ���ó�ʱ����

Input:           ��           
Output:         ��
Return:         ��
Date:            20141023
***************************************************************/

void CWirelessBtnTimer::Resettimeout()
{
	m_uctimeout = 0 ;
}


