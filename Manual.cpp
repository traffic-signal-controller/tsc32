 /***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   LampBoard.cpp
Date:       2013-4-13
Description:�źŻ����ֿز����������ļ�.�������ֿ����ȫ�죬����������,�ֶ��Զ��л���ť�����Ĵ���
Version:    V1.0
History:    2013.05.29 �޸İ�ť���º��ɿ��Ĵ���
		  2013.05.31 ��ӵ��ֶ�״̬ά��5��������ʱ�Զ��л�Ϊ�Զ�״̬.��
		  2013.09.10 �޸İ�ť���մ���ʽ���򻯴��롣
		  2014.10.06 �޸İ�ť���ܴ���ʽ����λ��ѭ���ж��޸�λ��ȡ��ֵ�����ж�
********************************************************************************************/
#include "Manual.h"
#include "TscMsgQueue.h"
#include "ace/Date_Time.h"
#include "MainBoardLed.h"
#include "ManaKernel.h"
#include "GbtMsgQueue.h"
#include "DbInstance.h"  //ADD:201403311050

#define MANUAL_TO_AUTO_TIME 10  //�����ֿ�״̬�л����Զ�����״̬ʱ�䣬��λ����

static Uint directype = 0x0 ;   //�������� 0-���� 1- ���� 2- �Ϸ� 3-����

static int key_value = 0x0;
static int deadmanual = 0x0 ;

CManaKernel * pManaKernel = CManaKernel::CreateInstance();

SThreadMsg sTscMsg;
SThreadMsg sTscMsgSts;	

/**************************************************************
Function:       Manual::Manual
Description:    Manual�๹�캯�����������ʼ������				
Input:          ��              
Output:         ��
Return:         ��
***************************************************************/
Manual::Manual() 
{
	m_ucManualSts     = MAC_CTRL_NOTHING;
	m_ucManual        = 0;

	m_ucLastManual    = 0;
	m_ucLastManualSts = MAC_CTRL_NOTHING;	
	ManualKey = 0 ; 
	m_ucRedFlash = 0 ;
	OpenDev();
	ACE_DEBUG((LM_DEBUG,"%s:%dInit Manual Object ok !\n",__FILE__,__LINE__));
}

/**************************************************************
Function:       Manual::~Manual
Description:    Manual��	��������	
Input:          ��              
Output:         ��
Return:         ��
***************************************************************/
Manual::~Manual() 
{
	ACE_DEBUG((LM_DEBUG,"Destruct Manual Object ok !\n"));
}

/**************************************************************
Function:       Manual::CreateInstance
Description:    ����	Manual��̬����
Input:          ��              
Output:         ��
Return:         ��̬����ָ��
***************************************************************/
Manual* Manual::CreateInstance()
{
	static Manual cManual;
	return &cManual;
}

/**************************************************************
Function:       Manual::OpenDev
Description:    ��ϵͳ��ť�豸�ļ�
Input:          ��              
Output:         ���ð�ť�ļ����
Return:         ��
***************************************************************/
void Manual::OpenDev()
{
	m_iManualFd = open(DEV_MANUAL_KEYINT, O_RDONLY | O_NONBLOCK);
	if(-1 == m_iManualFd)
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d  open DEV_MANUAL_KEYINT error!\n",__FILE__,__LINE__)); //MOD:0517 15:10
	}	
}

/**************************************************************
Function:       Manual::DoManual
Description:    ��ȡ��������û���ť����������������û���ť������
				100ms����һ��
Input:          ��              
Output:         ��
Return:         ��
***************************************************************/
void Manual::DoManual() 
{	
	int ret;
   	fd_set rfds;
  	int last_kval = key_value;	
	if ( m_iManualFd < 0 )
	{
		close(m_iManualFd);
		OpenDev();
	}
	
    FD_ZERO(&rfds);      
    FD_SET(m_iManualFd,&rfds);

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 1000;
	ret = select(m_iManualFd + 1,&rfds,NULL,NULL,&tv);
	if(ret == -1)
    {
       	ACE_DEBUG((LM_DEBUG,"%s:%dGet the button error!\n",__FILE__,__LINE__));        	
       	return ;
   	}
	else if(ret == 0)
		return ;
	if(FD_ISSET(m_iManualFd,&rfds))
   	{
       int iNum = read(m_iManualFd, &key_value, sizeof(key_value));
	   if(iNum == -1)
	   {
			ACE_DEBUG((LM_DEBUG,"%s:%d Get the button error!\n",__FILE__,__LINE__));
			return ;
	   }
		//ACE_DEBUG((LM_DEBUG,"%s:%d key_value =  %d\n",__FILE__,__LINE__,key_value));		
		switch(key_value)
		{
			case 0:
			{
									
				if(last_kval == 2 )
				{
					if (m_ucManual==1)
					{
						ManualKey = 0 ;
						m_ucManual = 0;								
						m_ucManualSts = MAC_CTRL_NOTHING;
						CMainBoardLed::CreateInstance()->DoAutoLed(true);						
						pManaKernel->SndMsgLog(LOG_TYPE_MANUAL,1,0,0,0);								
								
					}				
				}
				else if(last_kval == 4)
				{
					m_ucManualSts = MAC_CTRL_NOTHING;					
					pManaKernel->SndMsgLog(LOG_TYPE_MANUAL,3,0,0,0);	
					//ACE_DEBUG((LM_DEBUG,"%s:%d Exit PANEL FLASH!\n",__FILE__,__LINE__));
				}
				else if(last_kval == 8)
				{
					m_ucManualSts = MAC_CTRL_NOTHING;				
					pManaKernel->SndMsgLog(LOG_TYPE_MANUAL,5,0,0,0);		
					//ACE_DEBUG((LM_DEBUG,"%s:%d Exit PANEL ALLREAD!\n",__FILE__,__LINE__));
				}
				else
					return ;
				break ;
			}
			case 3:
				{
					if(m_ucManual==1)
					{
						if(ManualKey != 1)
							ManualKey = 1 ;
						m_ucManualSts = MAC_CTRL_NEXT_STEP;							
					}
					//ACE_DEBUG((LM_DEBUG,"%s:%d Manual: PANEL Next Step button be pushed !\n",__FILE__,__LINE__));
					break ;
				}
			case 2:
				{									
					if(last_kval == 0)
					{					
						m_ucManual = 1;
						CMainBoardLed::CreateInstance()->DoAutoLed(false);						
						pManaKernel->SndMsgLog(LOG_TYPE_MANUAL,0,0,0,0);								
						//ACE_DEBUG((LM_DEBUG,"%s:%d PANEL Manual button be pushed!\n",__FILE__,__LINE__));			
						
					}	
					else
						return ;
					break ;
				}
			case 4:
			{				
				if(last_kval == 6)
					m_ucManual = 0 ;
				if(m_ucManualSts != MAC_CTRL_FLASH )
				{
					m_ucManualSts = MAC_CTRL_FLASH;					
					pManaKernel->SndMsgLog(LOG_TYPE_MANUAL,2,0,0,0);
				}
				
				//ACE_DEBUG((LM_DEBUG,"%s:%d PANEL FLASH button be pushed!\n",__FILE__,__LINE__));			
				break ;			
			
			}
			case 8:
			{
				if(last_kval == 10)
					m_ucManual = 0 ;
				if(m_ucManualSts != MAC_CTRL_ALLRED )
				{
					m_ucManualSts = MAC_CTRL_ALLRED;				
					pManaKernel->SndMsgLog(LOG_TYPE_MANUAL,4,0,0,0);
				}		
				//ACE_DEBUG((LM_DEBUG,"%s:%d PANEL ALLRED button be pushed!\n",__FILE__,__LINE__));
				break;			
			
			}
			case 18:
			{				
				if(m_ucManual==1)
				{
					if(ManualKey != 4)	
						ManualKey =4 ;
					m_ucManualSts = MAC_CTRL_NEXT_PHASE;
				}				
				//ACE_DEBUG((LM_DEBUG,"%s:%d PANEL Next Stage button be pushed! \n",__FILE__,__LINE__));	
				break ;
			}			
			case 34 :
			{							
				if(m_ucManual==1)
				{
					if(ManualKey != 5)	
						ManualKey =5 ;
					m_ucManualSts =MAC_CTRL_NEXT_DIR;					
				}	
				//ACE_DEBUG((LM_DEBUG,"%s:%d PANEL Next Direc button be pushed! \n",__FILE__,__LINE__));
				break;				
			
			}
		
			default:
				last_kval = key_value; 
				return ;
					
		}
		last_kval = key_value;  	

		if ( m_ucManualSts == m_ucLastManualSts  && m_ucManual == m_ucLastManual )
		{
		 //ACE_DEBUG((LM_DEBUG,"%s:%d PANEL Control Mode don't changed!\n",__FILE__,__LINE__));
		 if(m_ucManual ==1)
		 {
			deadmanual++ ;
			if(deadmanual >MANUAL_TO_AUTO_TIME*600) //10����
			{
				 m_ucManual = 0 ;
				 m_ucManualSts =  MAC_CTRL_NOTHING;
				 deadmanual = 0;
				 CMainBoardLed::CreateInstance()->DoAutoLed(true);  //ADD: 201309231500
			}
			
		 }			
		else
			return ;	
		}

	if(m_ucManualSts == MAC_CTRL_FLASH)
		(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_COMMU_PARA+1 , MAC_CTRL_FLASH);
	else if(m_ucManualSts ==MAC_CTRL_ALLRED)
		(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_COMMU_PARA+1 , MAC_CTRL_ALLRED);
	else if(m_ucManual == 1)
		(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_COMMU_PARA+1 , MAC_CTRL_MANUL);
	else
		(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_COMMU_PARA+1 , MAC_CTRL_NOTHING);
	//ACE_DEBUG((LM_DEBUG,"%s:%d Manual:%d lastManal:%d ManualSts:%d  lastManualSts:%d	\n",__FILE__,__LINE__,m_ucManual,m_ucLastManual,m_ucManualSts,m_ucLastManualSts));
	CGbtMsgQueue *pGbtMsgQueue = CGbtMsgQueue::CreateInstance();
	ACE_OS::memset( &sTscMsg    , 0 , sizeof(SThreadMsg));
	ACE_OS::memset( &sTscMsgSts , 0 , sizeof(SThreadMsg));
    if ( MAC_CTRL_ALLRED == m_ucManualSts )  //���ȫ��
	{
		if ( MAC_CTRL_ALLRED == m_ucLastManualSts ) //��ǰ�Ѿ������ȫ��
		{
			return;
		}	
		pGbtMsgQueue->SendTscCommand(OBJECT_SWITCH_MANUALCONTROL,253);
		//ACE_DEBUG((LM_DEBUG,"%s:%d Send CTRL_PANEL ALLRED TscMsg!\n",__FILE__,__LINE__));
	}
	else if ( MAC_CTRL_FLASH == m_ucManualSts )  //������
	{
		if ( MAC_CTRL_FLASH == m_ucLastManualSts ) //��ǰ�Ѿ���������
		{
			return;
		}	
		pGbtMsgQueue->SendTscCommand(OBJECT_SWITCH_MANUALCONTROL,254);
		
		pManaKernel->m_pRunData->flashType = CTRLBOARD_FLASH_MANUALCTRL; 
		
		//ACE_DEBUG((LM_DEBUG,"%s:%d ManualFlash type=%d !\n",__FILE__,__LINE__,pManaKernel->m_pRunData->flashType));
	}
	else if ( 1 == m_ucManual )  //�ֶ�
	{
		if ( 0 == m_ucLastManual )  //�ϴη��ֶ�
		{				
			pGbtMsgQueue->SendTscCommand(OBJECT_CURTSC_CTRL,4); //���������ֿ�ֻ�ı���Ʒ�ʽ
			//ACE_DEBUG((LM_DEBUG,"%s:%d Begin change to  Manual Control! \n",__FILE__,__LINE__));
		}		
		if ( MAC_CTRL_NEXT_STEP == m_ucManualSts )  //����
		{
			sTscMsgSts.ulType       = TSC_MSG_LOCK_STEP;  //������ǰ��
			sTscMsgSts.ucMsgOpt     = 0;
			sTscMsgSts.uiMsgDataLen = 1;
			sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
			*((Byte*)sTscMsgSts.pDataBuf) = 0;
			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			ACE_DEBUG((LM_DEBUG,"%s:%d Send Next Step TscMsg ! \n",__FILE__,__LINE__));
			pManaKernel->SndMsgLog(LOG_TYPE_MANUAL,6,0,0,0);
		}
		else if ( MAC_CTRL_NEXT_PHASE == m_ucManualSts )  //��һ�׶�  ��һ��λ
		{
			if(pManaKernel->m_bNextPhase == true)
				return ;
			else
			{
				pManaKernel->m_bNextPhase = true ;
			}
			sTscMsg.ulType       = TSC_MSG_NEXT_STAGE;  //���׶�ǰ��
			sTscMsg.ucMsgOpt     = 0;
			sTscMsg.uiMsgDataLen = 1;
			sTscMsg.pDataBuf     = NULL;
			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));
			ACE_DEBUG((LM_DEBUG,"%s:%d Send MAC_CTRL_NEXT_PHASE TscMsg !\n",__FILE__,__LINE__));
			//pManaKernel->SndMsgLog(LOG_TYPE_MANUAL,7,0,0,0);  ��λ����ʱ��֧����һ�׶���־
		}
		else if ( MAC_CTRL_NEXT_DIR == m_ucManualSts )  //ң�����������������������ⷽ��
		{			
			if(pManaKernel->m_iTimePatternId == 0)
			{	
				pManaKernel->bTmpPattern = true ;
				pManaKernel->m_iTimePatternId = 250;//�����ķ����л�				
			}			
			sTscMsg.ulType       = TSC_MSG_PATTER_RECOVER;  
			sTscMsg.ucMsgOpt     = (directype++)%4;
			sTscMsg.uiMsgDataLen = 0;			
			sTscMsg.pDataBuf     = NULL; 			
			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));	
	
					
		}
	}		
	else if (0 == m_ucManual)  //�޲����ҷ��ֶ�״̬��
	{		
		if( MAC_CTRL_NOTHING == m_ucManualSts)
		{	
			//ACE_DEBUG((LM_DEBUG,"%s:%d MAC_CTRL_NOTHING == m_ucManualSts \n",__FILE__,__LINE__));
			if( pManaKernel->m_iTimePatternId == 250)
			{
				pManaKernel->bNextDirec = false ;
				pManaKernel->m_iTimePatternId = 0;
				pManaKernel->bTmpPattern = false ;
			}
				pGbtMsgQueue->SendTscCommand(OBJECT_SWITCH_MANUALCONTROL,0); //����ť������������ȫ�츴λ	
				ACE_DEBUG((LM_DEBUG,"\n%s:%d Set back to normal status !\n",__FILE__,__LINE__));	
		}

	}
	m_ucLastManualSts = m_ucManualSts;  //���浱ǰ���ֿ�״̬
	m_ucLastManual    = m_ucManual;     //���浱ǰ�Ŀ��Ʒ�ʽ

	}

	
}
void Manual::SetPanelStaus(Byte ucStatus)
{
	m_ucManualSts = ucStatus ;

}



