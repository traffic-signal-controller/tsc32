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
		  2014.05.22 沈阳16相位特殊处理，添加北东南西4个方向的按键。
********************************************************************************************/
#include "Manual.h"
#include "TscMsgQueue.h"
#include "ace/Date_Time.h"
#include "MainBoardLed.h"
#include "ManaKernel.h"
#include "GbtMsgQueue.h"
#include "DbInstance.h"  //ADD:201403311050

#define MANUAL_TO_AUTO_TIME 10  //无人手控状态切换到自动运行状态时间，单位分钟

#define NORTH 0
#define EAST  1
#define SOUTH 2
#define WEST  3
static Byte directype = 0x0 ;        //方向类型 0-北方 1- 东方 2- 南方 3-西方
static Byte  lastdirectype = 0xff;  //保存上次方向按键值
static bool bDirecControl = false ;

static int key_value = 0x0;
static int deadmanual = 0x0 ;


CManaKernel * pManaKernel = CManaKernel::CreateInstance();

SThreadMsg sTscMsg;
SThreadMsg sTscMsgSts;	

/**************************************************************
Function:       Manual::Manual
Description:    Manual类构造函数，用于类初始化处理				
Input:          无              
Output:         无
Return:         无
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
Description:    Manual类	析构函数	
Input:          无              
Output:         无
Return:         无
***************************************************************/
Manual::~Manual() 
{
	ACE_DEBUG((LM_DEBUG,"Destruct Manual Object ok !\n"));
}

/**************************************************************
Function:       Manual::CreateInstance
Description:    创建	Manual静态对象
Input:          无              
Output:         无
Return:         静态对象指针
***************************************************************/
Manual* Manual::CreateInstance()
{
	static Manual cManual;
	return &cManual;
}

/**************************************************************
Function:       Manual::OpenDev
Description:    打开系统按钮设备文件
Input:          无              
Output:         设置按钮文件句柄
Return:         无
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
Description:    获取控制面板用户按钮操作结果，并处理用户按钮操作。
				100ms调用一次
Input:          无              
Output:         无
Return:         无
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
		ACE_DEBUG((LM_DEBUG,"%s:%d key_value =  %d\n",__FILE__,__LINE__,key_value));		
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
					ACE_DEBUG((LM_DEBUG,"%s:%d Exit PANEL FLASH!\n",__FILE__,__LINE__));
				}
				else if(last_kval == 8)
				{
					m_ucManualSts = MAC_CTRL_NOTHING;				
					pManaKernel->SndMsgLog(LOG_TYPE_MANUAL,5,0,0,0);		
					ACE_DEBUG((LM_DEBUG,"%s:%d Exit PANEL ALLREAD!\n",__FILE__,__LINE__));
				}
				else
					return ;
				break ;
			}
			case 13:
				{
					if(m_ucManual==1)
					{
						if(ManualKey != 1)
							ManualKey = 1 ;
						m_ucManualSts = MAC_CTRL_NEXT_STEP;							
					}
					ACE_DEBUG((LM_DEBUG,"%s:%d Manual: PANEL Next Step button be pushed !\n",__FILE__,__LINE__));
					break ;
				}
			case 2:
			case 240:
				{	
					if(key_value == 240)
					{				
					 	bDirecControl = !bDirecControl ;
						if(!bDirecControl)
						{
							if (m_ucManual==1)
							{
								ManualKey = 0 ;
								m_ucManual = 0;	
								lastdirectype = 0xff;							
								m_ucManualSts = MAC_CTRL_NOTHING;
								CMainBoardLed::CreateInstance()->DoAutoLed(true);						
								pManaKernel->SndMsgLog(LOG_TYPE_MANUAL,1,0,0,0);	
								//ACE_DEBUG((LM_DEBUG,"%s:%d Get back to normal from wireless control! !\n",__FILE__,__LINE__));							
							 break ; 	
							}
						
						}				 
					}					
					if(last_kval == 0)
					{					
						m_ucManual = 1;
						CMainBoardLed::CreateInstance()->DoAutoLed(false);						
						pManaKernel->SndMsgLog(LOG_TYPE_MANUAL,0,0,0,0);								
						ACE_DEBUG((LM_DEBUG,"%s:%d PANEL Manual button be pushed!\n",__FILE__,__LINE__));			
						
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
				
				ACE_DEBUG((LM_DEBUG,"%s:%d PANEL FLASH button be pushed!\n",__FILE__,__LINE__));			
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
				ACE_DEBUG((LM_DEBUG,"%s:%d PANEL ALLRED button be pushed!\n",__FILE__,__LINE__));
				break;			
			
			}				
			case 16:    //北方按键
			case 32:    //东方按键
			case 64:    //南方按键
			case 128:   //西方按键
			case 3 :   //面板下一方向
			//case 3:
			{										
				if(m_ucManual==1)
				{					
					m_ucManualSts =MAC_CTRL_NEXT_DIR;
					switch(key_value)
					{
						case 16:						
							directype = NORTH ;
							ACE_DEBUG((LM_DEBUG,"%s:%d North Allow!! \n",__FILE__,__LINE__));
							//break ;
						case 32:						
							directype = EAST ;
							ACE_DEBUG((LM_DEBUG,"%s:%d East Allow!! \n",__FILE__,__LINE__));
							//break ;
						case 64:						
							directype = SOUTH ;
							//ACE_DEBUG((LM_DEBUG,"%s:%d South Allow!! \n",__FILE__,__LINE__));
							break;
						case 128:
							directype = WEST ;
							//ACE_DEBUG((LM_DEBUG,"%s:%d West Allow!! \n",__FILE__,__LINE__));
							break ;
						default:
							break;
					}
				}	
				
				break;
			}
		
			default:
				last_kval = key_value; 
				return ;
					
		}
		last_kval = key_value;  	

		if ( m_ucManualSts == m_ucLastManualSts  && m_ucManual == m_ucLastManual )
		{
		 ACE_DEBUG((LM_DEBUG,"%s:%d PANEL Control Mode don't changed!\n",__FILE__,__LINE__));
		 if(m_ucManual ==1)
		 {
			deadmanual++ ;
			if(deadmanual >MANUAL_TO_AUTO_TIME*600) //10分钟
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
	ACE_DEBUG((LM_DEBUG,"%s:%d Manual:%d lastManal:%d ManualSts:%d  lastManualSts:%d	\n",__FILE__,__LINE__,m_ucManual,m_ucLastManual,m_ucManualSts,m_ucLastManualSts));
	CGbtMsgQueue *pGbtMsgQueue = CGbtMsgQueue::CreateInstance();
	ACE_OS::memset( &sTscMsg    , 0 , sizeof(SThreadMsg));
	ACE_OS::memset( &sTscMsgSts , 0 , sizeof(SThreadMsg));
    if ( MAC_CTRL_ALLRED == m_ucManualSts )  //面板全红
	{
		if ( MAC_CTRL_ALLRED == m_ucLastManualSts ) //当前已经是面板全红
		{
			return;
		}	
		pGbtMsgQueue->SendTscCommand(OBJECT_SWITCH_MANUALCONTROL,253);
		ACE_DEBUG((LM_DEBUG,"%s:%d Send CTRL_PANEL ALLRED TscMsg!\n",__FILE__,__LINE__));
	}
	else if ( MAC_CTRL_FLASH == m_ucManualSts )  //面板黄闪
	{
		if ( MAC_CTRL_FLASH == m_ucLastManualSts ) //当前已经是面板黄闪
		{
			return;
		}	
		pGbtMsgQueue->SendTscCommand(OBJECT_SWITCH_MANUALCONTROL,254);
		ACE_DEBUG((LM_DEBUG,"%s:%d Send CTRL_PANEL FLASH! TscMsg!\n",__FILE__,__LINE__));
	}
	else if ( 1 == m_ucManual )  //手动
	{
		if ( 0 == m_ucLastManual )  //上次非手动
		{	
			if(bDirecControl == true)	
				pGbtMsgQueue->SendTscCommand(OBJECT_SWITCH_MANUALCONTROL,253); //第一次进入方向按键手控，要全红
			else
				pGbtMsgQueue->SendTscCommand(OBJECT_CURTSC_CTRL,4); //其他进入手控只改变控制方式
				ACE_DEBUG((LM_DEBUG,"%s:%d Begin change to  Manual Control! \n",__FILE__,__LINE__));
		}		
		if ( MAC_CTRL_NEXT_STEP == m_ucManualSts )  //步进
		{
			sTscMsgSts.ulType       = TSC_MSG_LOCK_STEP;  //按步伐前进
			sTscMsgSts.ucMsgOpt     = 0;
			sTscMsgSts.uiMsgDataLen = 1;
			sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
			*((Byte*)sTscMsgSts.pDataBuf) = 0;
			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			ACE_DEBUG((LM_DEBUG,"%s:%d Send Next Step TscMsg ! \n",__FILE__,__LINE__));
			pManaKernel->SndMsgLog(LOG_TYPE_MANUAL,6,0,0,0);
		}
		else if ( MAC_CTRL_NEXT_PHASE == m_ucManualSts )  //下一阶段  下一相位
		{
			if(pManaKernel->m_bNextPhase == true)
				return ;
			else
			{
				pManaKernel->m_bNextPhase = true ;
			}
			sTscMsg.ulType       = TSC_MSG_NEXT_STAGE;  //按阶段前进
			sTscMsg.ucMsgOpt     = 0;
			sTscMsg.uiMsgDataLen = 1;
			sTscMsg.pDataBuf     = NULL;
			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));
			ACE_DEBUG((LM_DEBUG,"%s:%d Send MAC_CTRL_NEXT_PHASE TscMsg !\n",__FILE__,__LINE__));
			//pManaKernel->SndMsgLog(LOG_TYPE_MANUAL,7,0,0,0);  上位机暂时不支持下一阶段日志
		}
		else if ( MAC_CTRL_NEXT_DIR == m_ucManualSts )  //遥控器北东南西按键控制特殊方案
		{
			if(lastdirectype == directype)
				return ;
			else
				lastdirectype = directype ;
			
			if(pManaKernel->m_iTimePatternId == 0)
			{	
				pManaKernel->bTmpPattern = true ;
				pManaKernel->m_iTimePatternId = 250;//运行四方向切换				
				ACE_DEBUG((LM_DEBUG,"%s:%d Send TSC_MSG_TIMEPATTERN -> 250 TscMsg !\n",__FILE__,__LINE__));
			}			
			sTscMsg.ulType       = TSC_MSG_PATTER_RECOVER; 
			if(key_value == 3) 
				sTscMsg.ucMsgOpt     = (directype++)%4;
			else
				sTscMsg.ucMsgOpt     = directype;
			sTscMsg.uiMsgDataLen = 0;			
			sTscMsg.pDataBuf     = NULL; 			
			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));	
			ACE_DEBUG((LM_DEBUG,"%s:%d Send DIRE=%d !\n",__FILE__,__LINE__,directype-1));
					
		}
	}		
	else if (0 == m_ucManual)  //无操作且非手动状态下
	{		
		if( MAC_CTRL_NOTHING == m_ucManualSts)
		{	
			//ACE_DEBUG((LM_DEBUG,"%s:%d MAC_CTRL_NOTHING == m_ucManualSts \n",__FILE__,__LINE__));
			if( pManaKernel->m_iTimePatternId == 250)
			{
				//pManaKernel->bNextDirec = false ;
				pManaKernel->m_iTimePatternId = 0;
				pManaKernel->bTmpPattern = false ;
							
			}		
				pGbtMsgQueue->SendTscCommand(OBJECT_SWITCH_MANUALCONTROL,0); //当按钮从面板黄闪或者全红复位	
				ACE_DEBUG((LM_DEBUG,"\n%s:%d Set back to normal status !\n",__FILE__,__LINE__));	
				
		}

	}
	m_ucLastManualSts = m_ucManualSts;  //保存当前的手控状态
	m_ucLastManual    = m_ucManual;     //保存当前的控制方式

	}

	
}
void Manual::SetPanelStaus(Byte ucStatus)
{
	m_ucManualSts = ucStatus ;

}


