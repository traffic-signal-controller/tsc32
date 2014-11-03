 /***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   LampBoard.cpp
Date:       2013-4-13
Description:‰ø°Âè∑Êú∫ÁÅØÊâãÊéßÊìç‰ΩúÂ§ÑÁêÜÁ±ªÊñá‰ª∂.ÂåÖÂê´ÂØπÊâãÊéßÈù¢ÊùøÂÖ®Á∫¢ÔºåÈªÑÈó™ÔºåÊ≠•Ëøõ,ÊâãÂä®Ëá™Âä®ÂàáÊç¢ÊåâÈíÆÊìç‰ΩúÁöÑÂ§ÑÁêÜ„ÄÇ
Version:    V1.0
History:    2013.05.29 ‰øÆÊîπÊåâÈíÆÊåâ‰∏ãÂíåÊùæÂºÄÁöÑÂ§ÑÁêÜ„ÄÇ
		  2013.05.31 Ê∑ªÂä†ÂΩìÊâãÂä®Áä∂ÊÄÅÁª¥Êä§5ÂàÜÈíü‰ª•‰∏äÊó∂Ëá™Âä®ÂàáÊç¢‰∏∫Ëá™Âä®Áä∂ÊÄÅ.„ÄÇ
		  2013.09.10 ‰øÆÊîπÊåâÈíÆÊé•Êî∂Â§ÑÁêÜÊñπÂºèÔºåÁÆÄÂåñ‰ª£Á†Å„ÄÇ
		  2014.10.06 ‰øÆÊîπÊåâÈíÆÊé•ÂèóÂ§ÑÁêÜÊñπÂºèÔºåÁî±‰ΩçÁßªÂæ™ÁéØÂà§Êñ≠‰øÆÊîπ‰ΩçËØªÂèñÈîÆÂÄºËøõË°åÂà§Êñ≠
		  2014.10.23 ∏ƒ‘Ï≥…÷ß≥÷Œﬁœﬂ∞¥º¸ ÷øÿøÿ÷∆
********************************************************************************************/
#include "WirelessButtons.h"
#include "TscMsgQueue.h"
#include "ace/Date_Time.h"
#include "MainBoardLed.h"
#include "TimerManager.h"
#include "GbtMsgQueue.h"
#include "Can.h"
enum{
	 BUTTON_MANUAL_SELF 		= 0x0,	//◊‘÷˜‘À––
	 BUTTON_MANUAL_MANUAL 		= 0x1,	// ÷∂Ø‘À––	
	 BUTTON_MANUAL_YELLOW_FLASH	= 0x2,	//ª∆…¡
	 BUTTON_MANUAL_ALL_RED		= 0x3,	//»´∫Ï
	 BUTTON_MANUAL_NEXT_STEP	= 0x4,	//œ¬“ª≤Ω
	 BUTTON_MANUAL_NEXT_PHASE	= 0x5,	//œ¬“ªœ‡Œª
	 BUTTON_MANUAL_NEXT_DIREC	= 0x6,	//œ¬“ª∑ΩœÚ
	 BUTTON_MANUAL_DIREC		= 0x7,	//∞¥÷∏∂®∑ΩœÚ∫Õœ‡Œª≥µµ¿∑Ω–Œ
	
};

/**************************************************************
Function:       CWirelessBtn::CWirelessBtn
Description:    CWirelessBtnÁ±ªÊûÑÈÄ†ÂáΩÊï∞ÔºåÁî®‰∫éÁ±ªÂàùÂßãÂåñÂ§ÑÁêÜ				
Input:          Êó†              
Output:         Êó†
Return:         Êó†
***************************************************************/
CWirelessBtn::CWirelessBtn() 
{	
	m_ucLastManualSts = BUTTON_MANUAL_SELF; //≥ı ºªØ¥¶”⁄◊‘∂Ø‘À––◊¥Ã¨	
	m_ucdirectype = 0x0 ; 					//∑ΩœÚ¿‡–Õ 0-±±∑Ω 1- ∂´∑Ω 2- ƒœ∑Ω 3-Œ˜∑Ω	
    pManaKernel = CManaKernel::CreateInstance();
}

/**************************************************************
Function:       CWirelessBtn::~CWirelessBtn
Description:    CWirelessBtnÁ±ª	ÊûêÊûÑÂáΩÊï∞	
Input:          Êó†              
Output:         Êó†
Return:         Êó†
***************************************************************/
CWirelessBtn::~CWirelessBtn() 
{
	;
}

/**************************************************************
Function:       Manual::CreateInstance
Description:    ÂàõÂª∫	ManualÈùôÊÄÅÂØπË±°
Input:          Êó†              
Output:         Êó†
Return:         ÈùôÊÄÅÂØπË±°ÊåáÈíà
***************************************************************/
CWirelessBtn* CWirelessBtn::CreateInstance()
{
	static CWirelessBtn cWirelessBtn;
	return &cWirelessBtn;
}

/****************************************************************
Function:       CWirelessBtn::EntryWirelessManul
Description:    ÕÀ≥ˆŒﬁœﬂ“£øÿ∞¥º¸£¨ª÷∏¥◊‘÷˜øÿ÷∆
Input:           Œﬁ          
Output:         Œﬁ
Return:         Œﬁ
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
Description:    ≥ı¥ŒΩ¯»ÎŒﬁœﬂ∞¥º¸ ÷∂Ø◊¥Ã¨
Input:           Œﬁ          
Output:         Œﬁ
Return:         Œﬁ
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
Description:    ∂‘¥”“£øÿ∆˜∑µªÿ÷˜∞Âµƒ∞¥º¸CAN ˝æ›Ω¯––¥¶¿Ì
Input:            Can◊‹œﬂΩ” ’µΩµƒŒﬁœﬂ∞¥º¸ ˝æ›÷°            
Output:         Œﬁ
Return:         Œﬁ
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
		if(iAuto_Manul == 0x0) //◊‘∂Ø◊¥Ã¨
		{
			if(m_ucLastManualSts == BUTTON_MANUAL_SELF) //»Áπ˚…œ¥Œ◊¥Ã¨“—æ≠ «◊‘÷˜‘À––
				return ;
			else
			{
				BackToAuto();
				CTimerManager::CreateInstance()->CloseWirelessBtn();				
				CWirelessBtnTimer::CreateInstance()->Resettimeout();  //ÕÀ≥ˆ∂® ±∆˜≥¨ ±º∆ ˝÷√Œ™0
			}
		}
		else if(iAuto_Manul == 0x1) // ÷øÿ◊¥Ã¨
		{				
			if(m_ucLastManualSts == BUTTON_MANUAL_SELF) //»Áπ˚…œ¥Œ◊¥Ã¨ «◊‘÷˜‘À––
			{
				EntryWirelessManul();
				CTimerManager::CreateInstance()->StartWirelessBtnTimer();
				
				//ACE_OS::printf("%s:%d WirelessBtn timeout = %d seconds!\r\n",CManaKernel::CreateInstance()->m_pTscConfig->sSpecFun[FUN_WIRELESSBTN_TIMEOUT].ucValue*60);
			}		
			else
			{									
				CWirelessBtnTimer::CreateInstance()->Resettimeout();  //”–‘⁄Ω¯––Œﬁœﬂ ÷øÿ≤Ÿ◊˜‘Ú£¨≥¨ ±º∆ ˝÷√Œ™0
				if((RecvButtonType>>1)&0x1)  //Ω¯»Îª∆…¡øÿ÷∆
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
				else if((RecvButtonType>>2)&0x1)//Ω¯»Î»´∫Ïøÿ÷∆
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
				else if((RecvButtonType>>3)&0x1)//µ„ª˜œ¬“ª≤Ω∞¥º¸
				{
					pGbtMsgQueue->SendTscCommand(OBJECT_GOSTEP,0);
					ACE_DEBUG((LM_DEBUG,"%s:%d Send WirelessBtn Next Step TscMsg ! \n",__FILE__,__LINE__));
					pManaKernel->SndMsgLog(LOG_TYPE_MANUAL,6,0,0,0);
					if(m_ucLastManualSts != BUTTON_MANUAL_NEXT_STEP)
						m_ucLastManualSts = BUTTON_MANUAL_NEXT_STEP;
				}
				else if((RecvButtonType>>4)&0x1) //œ¬“ªœ‡Œª
				{
					pGbtMsgQueue->SendTscCommand(OBJECT_SWITCH_STAGE,0);
					ACE_DEBUG((LM_DEBUG,"%s:%d Send WirelessBtn Next Phase TscMsg !\n",__FILE__,__LINE__));
					if(m_ucLastManualSts != BUTTON_MANUAL_NEXT_PHASE)
						m_ucLastManualSts = BUTTON_MANUAL_NEXT_PHASE;	
				}
				else if((RecvButtonType>>5)&0x1)//œ¬“ª∑ΩœÚ
				{
					if(pManaKernel->m_iTimePatternId == 0)
					{	
						pManaKernel->bTmpPattern = true ;
						pManaKernel->m_iTimePatternId = 250;//‘À––Àƒ∑ΩœÚ«–ªª				
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
Description:    CWirelessBtnTimer¿‡ππ‘Ï∫Ø ˝
Input:           Œﬁ             
Output:         Œﬁ
Return:         Œﬁ
Date:            20141023
***************************************************************/
CWirelessBtnTimer::CWirelessBtnTimer()
{
	m_uctimeout = 0 ;
	ACE_DEBUG((LM_DEBUG,"%s:%d Init WirelessBtnTimer object ok !\n",__FILE__,__LINE__));

}

/**************************************************************
Function:       CWirelessBtnTimer::~CWirelessBtnTimer
Description:    CTscTimer¿‡Œˆππ∫Ø ˝
Input:          Œﬁ               
Output:         Œﬁ
Return:         Œﬁ
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
Description:    –≈∫≈ª˙“£øÿ∆˜∞¥≈•∂® ±∆˜∂® ±ªÿµ˜∫Ø ˝£¨1s÷¥––“ª¥Œ°£¥¶¿Ì–≈∫≈ª˙
				∂‡ ˝∂® ±π§◊˜°£
Input:           ƒ¨»œ¥¶¿Ì£¨”√ªßŒﬁ–Ë ‰»Î              
Output:         Œﬁ
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
Description:    ÷ÿ÷√≥¨ ±º∆ ˝

Input:           Œﬁ           
Output:         Œﬁ
Return:         Œﬁ
Date:            20141023
***************************************************************/

void CWirelessBtnTimer::Resettimeout()
{
	m_uctimeout = 0 ;
}


