/***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   GbtMsgQueue.cpp
Date:       2013-1-1
Description:信号机GBT协议消息处理类
Version:    V1.0
History:
***************************************************************/
#include "ace/Synch.h"
#include "ace/Timer_Heap.h"
#include "ace/OS.h"
#include "ace/Guard_T.h" 
#include "ace/Date_Time.h"
#include "Gsm.h"
#include "DbInstance.h"
#include "GbtMsgQueue.h"
#include "TscMsgQueue.h"
#include "ManaKernel.h"
#include "GbtDb.h"
#include "FlashMac.h"
#include "PowerBoard.h"
#include "Detector.h"
#include "LampBoard.h"
#include "GbtTimer.h"
#include "Gps.h"
#include "Can.h"
#include "PscMode.h"
#include "MainBoardLed.h"
#include "ComFunc.h"
#include "GaCountDown.h"
#include "Configure.h"
#include "SerialCtrl.h"
#include "MacControl.h"

/**************************************************************
Function:        CGbtMsgQueue::CGbtMsgQueue
Description:     GBT CGbtMsgQueue类构造函数，初始化类			
Input:          无           
Output:         无
Return:         无
***************************************************************/
CGbtMsgQueue::CGbtMsgQueue()
{

#ifdef NTCIP
	int iLen = 0;
	timeval tTmp;
	ACE_Message_Block *mb = NULL;
	SThreadMsg sMsg; 
	ACE_Time_Value nowait(GetCurTime());
#endif

	iPort     = 0;
	m_pMsgQue = ACE_Message_Queue_Factory<ACE_MT_SYNCH>::create_static_message_queue();
	
	//STscConfig* pTscCfg = CManaKernel::CreateInstance()->m_pTscConfig;

	for ( Byte i=0; i<MAX_CLIENT_NUM; i++ )
	{
		m_sGbtDealData[i].bIsDeal = false;
	}
	
	//iPort |= pTscCfg->sSpecFun[FUN_PORT_LOW].ucValue;
	//iPort |= pTscCfg->sSpecFun[FUN_PORT_HIGH].ucValue << 8;
	//信号机端口配置，改到配置文件中.这个存在一个bug，必需先调用showconfig 再调用读取某个值。否则就会报错
	Configure::CreateInstance()->ShowConfig();
	Configure::CreateInstance()->GetInteger("COMMUNICATION","port",iPort);
	//Configure::ShowConfig();
	if ( iPort > MAX_GBT_PORT || iPort < MIN_GBT_PORT )
	{
		iPort = DEFAULT_GBT_PORT; //UDP数据通信端口
	}
	m_addrLocal.set_port_number(iPort);

#ifdef GBT_TCP
	if ( -1 == m_acceptor.open(m_addrLocal,1) )   //绑定端口
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d bind port fail",__FILE__,__LINE__));
	}
#else
	if ( -1 == m_sockLocal.open(m_addrLocal) )
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d bind port fail",__FILE__,__LINE__));
	}
#endif
	ACE_DEBUG((LM_DEBUG,"%s:%d Init GbtMsgQuete object ok !\n",__FILE__,__LINE__));
}


/**************************************************************
Function:       CGbtMsgQueue::~CGbtMsgQueue
Description:    CGbtMsgQueue类析构函数		
Input:          无              
Output:         无
Return:         无
***************************************************************/
CGbtMsgQueue::~CGbtMsgQueue()
{
	if ( m_pMsgQue != NULL )
	{
		delete m_pMsgQue;
		m_pMsgQue = NULL;
	}	
	ACE_DEBUG((LM_DEBUG,"%s:%d Destruct GbtMsgQuete object ok !\n",__FILE__,__LINE__));
}

/**************************************************************
Function:       CGbtMsgQueue::CreateInstance
Description:    创建CGbtMsgQueue类静态对象		
Input:          无              
Output:         无
Return:         CGbtMsgQueue静态对象指针
***************************************************************/
CGbtMsgQueue* CGbtMsgQueue::CreateInstance()
{
	static CGbtMsgQueue cGbt;
	return &cGbt;
}


/**************************************************************
Function:       CGbtMsgQueue::SendGbtMsg
Description:    向gbt消息队列发送gbt消息		
Input:          pMsg  消息内容结构体指针   
				iLength  消息长度         
Output:         无
Return:         0-发送失败  1-发送成功
***************************************************************/
int CGbtMsgQueue::SendGbtMsg(SThreadMsg* pMsg,int iLength)
{
	if ( pMsg==NULL || m_pMsgQue==NULL )
	{
#ifdef TSC_DEBUG
		ACE_DEBUG((LM_DEBUG,"pMsg or m_pMsgQue is NULL"));
#endif
		return 0;
	}
	
	ACE_Message_Block *mb = new ACE_Message_Block(iLength);  //构造消息块
	mb->copy((char*)pMsg, iLength); // 将数据拷贝进消息块

	ACE_Time_Value nowait(GetCurTime()+ACE_Time_Value(1));
	m_pMsgQue->enqueue_tail(mb, &nowait);  //向 ACE_Message_Queue中添加新数据块
	//ACE_DEBUG((LM_DEBUG,"%s:%d pMsg Send ok !\n",__FILE__,__LINE__));
	return 1;
}


/**************************************************************
Function:       CGbtMsgQueue::DealData
Description:    处理gbt消息队列数据		
Input:          无   
Output:         无
Return:         无
***************************************************************/
void CGbtMsgQueue::DealData()
{
	int iLen = 0;
	timeval tTmp;
	ACE_Message_Block *mb = NULL;
	SThreadMsg sMsg; 
	ACE_Time_Value nowait(GetCurTime());

	tTmp.tv_sec = 0;
	tTmp.tv_usec = 10 * 1000;

	while ( m_pMsgQue != NULL )
	{
		if(m_pMsgQue->dequeue_head(mb, &nowait) != -1) //从 ACE_Message_Queue 中弹出消息块
		{   
			iLen = (int)mb->length();
			memcpy((char*)&sMsg, mb->base(), iLen);   //从消息块中读数据
			mb->release();
		}
		else
		{
			ACE_OS::sleep(ACE_Time_Value(tTmp));   //暂停10毫秒
			continue;
		}		
		//ACE_DEBUG((LM_DEBUG,"\n%s:%d  type:%d opt:%d dataLen:%d\n",__FILE__,__LINE__,sMsg.ulType,sMsg.ucMsgOpt,sMsg.uiMsgDataLen));//MOD:0515 17:05

		switch ( sMsg.ulType )   //消息处理
		{
		case GBT_MSG_FIRST_RECV:   //解析接收到的数据，第一次从上位机接收到的UDP协议数据 或者是gbttimer主动定时器发送过来的数据
			if ( 1 == CheckMsg(sMsg.ucMsgOpt,sMsg.uiMsgDataLen,(Byte*)sMsg.pDataBuf) ) //检查接收到GBT协议数据合法性,错误则发送错误应答!
			{
				FirstRecv(sMsg.ucMsgOpt,sMsg.uiMsgDataLen,(Byte*)sMsg.pDataBuf);//sMsg.ucMsgOpt 参数表示消息属于哪个接收缓冲块
			}
			break;

		case GBT_MSG_DEAL_RECVBUF:  //解析接收到的BUF
			DealRecvBuf(sMsg.ucMsgOpt);
			break;
			
		case GBT_MSG_SEND_HOST:   //回送给上位机
			SendToHost(sMsg.ucMsgOpt);
			break;
			
		case GBT_MSG_TSC_STATUS:  //获取信号机状态  //这个是信号机TSC线程发送过来的GBT消息，不是来自上位机。
			PackTscStatus(sMsg.ucMsgOpt,sMsg.pDataBuf);
			break;

		case GBT_MSG_TSC_EXSTATUS: //获取信号机扩展对象 F8
			PackTscExStatus(sMsg.ucMsgOpt,sMsg.pDataBuf);
			break;

		case GBT_MSG_OTHER_OBJECT: //gbt其他类对象
			PackOtherObject(sMsg.ucMsgOpt);
			break;

		case GBT_MSG_EXTEND:
			PackExtendObject(sMsg.ucMsgOpt);
			break;
			
		case GBT_MSG_SELF_REPORT:  //主动上报
			SelfReport(sMsg.uiMsgDataLen,(Byte*)sMsg.pDataBuf);
			break;
			
		default:
			break;
		}

		if ( sMsg.pDataBuf != NULL )
		{
			ACE_OS::free(sMsg.pDataBuf);  //删除申请的内存
			sMsg.pDataBuf = NULL;
		}
	}
}


/**************************************************************
Function:       CGbtMsgQueue::PackOtherObject
Description:    打包其他特殊类型的对象		
Input:          ucDealDataIndex   消息处理索引下标   
Output:         无
Return:         无
***************************************************************/
void CGbtMsgQueue::PackOtherObject(Byte ucDealDataIndex)
{
	Byte ucRecvOptType = ( m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[0]) & 0xf;   //ÊÕµœÖ¡µÄ²Ù×÷ÀàÐÍ
	int iRecvIndex     = m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex;          
	int iRecvBufLen    = m_sGbtDealData[ucDealDataIndex].sRecvFrame.iBufLen;
	int iSendIndex     = m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex;          
	//int iSendBufLen    = m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen;
	Byte ucIndexCnt    = 0;  //索引个数
	Byte ucErrorSts    = 0;  //错误状态
	Byte ucErrorIdx    = 0;  //错误索引
	Byte ucObjId       = 0;  //对象名(表名)
	Byte ucIdxFst      = 0;  //第一个索引(id1)
	Byte ucIdxSnd      = 0;  //第二个索引(id2)
	Byte ucSubId       = 0;  //子对象(字段下标)
	Byte ucIndex       = 0;

	if ( iRecvIndex >= iRecvBufLen )
	{
#ifdef TSC_DEBUG
		ACE_DEBUG((LM_DEBUG,"%s:%d,GBT_ERROR_OTHER\n",__FILE__,__LINE__));
#endif
		ucErrorSts = GBT_ERROR_OTHER;
		GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
		return;
	}

	/************对象标识*************/
	ucObjId = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
	m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucObjId;  
	iRecvIndex++;
	iSendIndex++;

	/***********索引个数与子对象*******/
	if ( iRecvIndex >= iRecvBufLen )
	{
#ifdef TSC_DEBUG
		//ACE_DEBUG((LM_DEBUG,"%s:%d,GBT_ERROR_OTHER\n",__FILE__,__LINE__));
#endif
		ucErrorSts = GBT_ERROR_OTHER;
		GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
		return;
	}
	m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] =
					m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex]; 			//索引个数与子对象
	ucIndexCnt = (m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex]>>6) & 0x3;   //索引个数
	ucSubId    = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex] & 0x3F;       //子对象，字段下标
	iRecvIndex++;
	iSendIndex++;

	/***********索引*************/
	if ( ucIndexCnt > 0 )  
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
#ifdef TSC_DEBUG
			//ACE_DEBUG((LM_DEBUG,"%s:%d,GBT_ERROR_OTHER\n",__FILE__,__LINE__));
#endif
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		ucIdxFst = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucIdxFst;
		iRecvIndex++;
		iSendIndex++;
		ucIndexCnt--;
	}
	if ( ucIndexCnt > 0 ) 
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
#ifdef TSC_DEBUG
			//ACE_DEBUG((LM_DEBUG,"%s:%d,GBT_ERROR_OTHER\n",__FILE__,__LINE__));
#endif
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		ucIdxSnd = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucIdxSnd;
		iRecvIndex++;
		iSendIndex++;
		ucIndexCnt--;
	}
	if ( ucIndexCnt > 0 )  
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
#ifdef TSC_DEBUG
			//ACE_DEBUG((LM_DEBUG,"%s:%d,GBT_ERROR_OTHER\n",__FILE__,__LINE__));
#endif
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		iRecvIndex++;
		iSendIndex++;
		ucIndexCnt--;
	}

	/************值域**************/
	//m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = iSendIndex;
	switch ( ucObjId )
	{
	case OBJECT_UTC_TIME:
		if ( GBT_SEEK_REQ == ucRecvOptType ) 
		{
			ACE_Time_Value tvCurTime = GetCurTime();
			unsigned int iTotalSec   = (unsigned int)tvCurTime.sec();    //utc time
			iTotalSec = iTotalSec +8*3600 ;

			//ACE_DEBUG((LM_DEBUG,"******************iTotalSec = %d return:\n",iTotalSec+8 * 60 * 60,iTotalSec));
			/*采取网络字节序，高字节在低位*/
			while ( ucIndex < 4 )
			{
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = ( iTotalSec>>(8*(3-ucIndex)) & 0xFF );
				ucIndex++;
			}
			//ACE_DEBUG((LM_DEBUG,"%s:%d Reple UTC seconds: %d,ucDealDataIndex=%d!\n",__FILE__,__LINE__,iTotalSec,ucDealDataIndex));
		}
		else if ( (GBT_SET_REQ == ucRecvOptType) || (GBT_SET_REQ_NOACK == ucRecvOptType) ) 
		{
			int iIndex = 0;
			SThreadMsg sTscMsg;
			Ulong iTotalSec = 0;
			ACE_Time_Value tvCurTime;
			ACE_Date_Time  tvDateTime;

			while ( iIndex < 4 )
			{
				iTotalSec |=  
					((m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iIndex+iRecvIndex]&0xff)<<(8*(3-iIndex)));
				iIndex++;
			}

			tvCurTime.sec(iTotalSec);
			tvDateTime.update(tvCurTime);

			//CDate cCorrDate((Uint)tvDateTime.year(),(Byte)tvDateTime.month(),(Byte)tvDateTime.day());
			//if ( !cCorrDate.IsRightDate() ) MOD:201309281025
			if (!IsRightDate((Uint)tvDateTime.year(),(Byte)tvDateTime.month(),(Byte)tvDateTime.day()))
			{
				//ACE_DEBUG((LM_DEBUG,"%s:%d %d-%d-%d\n",
				//__FILE__,__LINE__,tvDateTime.year(),tvDateTime.month(),tvDateTime.day()));
				GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
				return;
			}

			sTscMsg.ulType       = TSC_MSG_CORRECT_TIME;
			sTscMsg.ucMsgOpt     = OBJECT_UTC_TIME;
			sTscMsg.uiMsgDataLen = 4;
			sTscMsg.pDataBuf     = ACE_OS::malloc(4);
			ACE_OS::memcpy(sTscMsg.pDataBuf,m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf+iRecvIndex,4);
			iRecvIndex += 4;
			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

			if ( CManaKernel::CreateInstance()->m_pTscConfig->sSpecFun[FUN_GPS].ucValue != 0 )
			{
				CGps::CreateInstance()->ForceAdjust();
			}
		}
		break;
	case OBJECT_LOCAL_TIME:
		if ( GBT_SEEK_REQ == ucRecvOptType )  
		{
			ACE_Time_Value tvCurTime = GetCurTime();
			unsigned int iTotalSec = (unsigned int)tvCurTime.sec() + 8 * 60 * 60; 
			
			/*采取网络字节序，高字节在低位*/
			while ( ucIndex < 4 )
			{
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = ( iTotalSec>>(8*(3-ucIndex)) & 0xFF );
				ucIndex++;
			}
		}
		else if ( (GBT_SET_REQ == ucRecvOptType) || (GBT_SET_REQ_NOACK == ucRecvOptType) ) 
		{
			int iIndex = 0;
			SThreadMsg sTscMsg;
			Ulong iTotalSec = 0;
			ACE_Time_Value tvCurTime;
			ACE_Date_Time  tvDateTime;

			while ( iIndex < 4 )
			{
				iTotalSec |=  
					((m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iIndex+iRecvIndex]&0xff)<<(8*(3-iIndex)));
				iIndex++;
			}
			iTotalSec = iTotalSec + 8 * 60 * 60;

			tvCurTime.sec(iTotalSec);
			tvDateTime.update(tvCurTime);

			//CDate cCorrDate((Uint)tvDateTime.year(),(Byte)tvDateTime.month(),(Byte)tvDateTime.day());
			//if ( !cCorrDate.IsRightDate() )MOD:201309281017
			if (!IsRightDate((Uint)tvDateTime.year(),(Byte)tvDateTime.month(),(Byte)tvDateTime.day()))
			{
				//ACE_DEBUG((LM_DEBUG,"%s:%d error date:%d-%d-%d\n"
					//,__FILE__,__LINE__,tvDateTime.year(),tvDateTime.month(),tvDateTime.day()));
				GotoMsgError(ucDealDataIndex,5,ucErrorIdx);
				return;
			}

			sTscMsg.ulType       = TSC_MSG_CORRECT_TIME;
			sTscMsg.ucMsgOpt     = OBJECT_LOCAL_TIME;
			sTscMsg.uiMsgDataLen = 4;
			sTscMsg.pDataBuf     = ACE_OS::malloc(4);
			ACE_OS::memcpy(sTscMsg.pDataBuf,m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf+iRecvIndex,4);
			iRecvIndex += 4;
			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));
			if ( CManaKernel::CreateInstance()->m_pTscConfig->sSpecFun[FUN_GPS].ucValue != 0 )
			{
				CGps::CreateInstance()->ForceAdjust();
			}
		}
		break;
	default:
		GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
		return;
		break;
	}

	m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex = iRecvIndex;
	m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex = iSendIndex;
	m_sGbtDealData[ucDealDataIndex].iObjNum--;
	if ( iRecvIndex == iRecvBufLen ) 		//该对象处理完毕
	{
		if ( 0 == m_sGbtDealData[ucDealDataIndex].iObjNum )   //所有对象都处理完毕
		{
			m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = iSendIndex;
			//ACE_DEBUG((LM_DEBUG,"%s:%d Send datetime to host,ucDealDataIndex=%d ,sendlen = %d!\n",__FILE__,__LINE__,ucDealDataIndex,iSendIndex));
			GotoSendToHost(ucDealDataIndex);
			return;
		}
		else
		{
			ucErrorSts = 5;
			ucErrorIdx = 0;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
	}
	else
	{
		GotoDealRecvbuf(ucDealDataIndex);
		return;
	}
}


/**************************************************************
Function:       CGbtMsgQueue::GotoMsgError
Description:    向上位机发送处理错误信息		
Input:          ucDealDataIndex   消息处理索引下标
				ucErrorSts  错误状态值   
				ucErrorIdx  错误索引
Output:         无
Return:         无
***************************************************************/
void CGbtMsgQueue::GotoMsgError(Byte ucDealDataIndex,Byte ucErrorSts,Byte ucErrorIdx)
{
	m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[0] = 
		(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[0] & 0xf0) | GBT_ERR_ACK;
	m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[1] = ucErrorSts;
	m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[2] = ucErrorIdx;
	m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = 3;
	GotoSendToHost(ucDealDataIndex);
}


/**************************************************************
Function:       CGbtMsgQueue::GotoSendToHost
Description:    帧处理完毕或者出现错误 执行返回给上位机信息		
Input:          ucDealDataIndex   消息处理索引下标
Output:         无
Return:         无
***************************************************************/
void CGbtMsgQueue::GotoSendToHost(Byte ucDealDataIndex)
{
	SThreadMsg sGbtMsg;
	sGbtMsg.ulType       = GBT_MSG_SEND_HOST;  
	sGbtMsg.ucMsgOpt     = ucDealDataIndex;
	sGbtMsg.uiMsgDataLen = 0;
	sGbtMsg.pDataBuf     = NULL;	
	SendGbtMsg(&sGbtMsg,sizeof(sGbtMsg));
}


/**************************************************************
Function:       CGbtMsgQueue::GotoDealRecvbuf
Description:    帧多个对象未全部完成 继续处理帧		
Input:          ucDealDataIndex   消息处理索引下标
Output:         无
Return:         无
***************************************************************/
void CGbtMsgQueue::GotoDealRecvbuf(Byte ucDealDataIndex)
{
	SThreadMsg sGbtMsg;
	sGbtMsg.ulType       = GBT_MSG_DEAL_RECVBUF;  
	sGbtMsg.ucMsgOpt     = ucDealDataIndex;
	sGbtMsg.uiMsgDataLen = 0;
	sGbtMsg.pDataBuf     = NULL;	
	SendGbtMsg(&sGbtMsg,sizeof(sGbtMsg));
}


/**************************************************************
Function:       CGbtMsgQueue::PackExtendObject
Description:    打包扩展协议的对象		
Input:          ucDealDataIndex   消息处理索引下标
Output:         无
Return:         无
***************************************************************/
void CGbtMsgQueue::PackExtendObject(Byte ucDealDataIndex)
{
	Byte ucRecvOptType = ( m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[0]) & 0xf;   //收到帧的操作类型
	int iRecvIndex     = m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex;          
	int iRecvBufLen    = m_sGbtDealData[ucDealDataIndex].sRecvFrame.iBufLen;
	int iSendIndex     = m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex;          
	//int iSendBufLen    = m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen;
	Byte ucIndexCnt    = 0;  //索引个数
	Byte ucErrorSts    = 0;  //错误状态
	Byte ucErrorIdx    = 0;  //错误索引
	Byte ucObjId       = 0;  //对象名(表名)
	Byte ucIdxFst      = 0;  //第一个索引(id1)
	Byte ucIdxSnd      = 0;  //第二个索引(id2)
	Byte ucSubId       = 0;  //子对象(字段下标)

	if ( iRecvIndex >= iRecvBufLen )
	{
		ucErrorSts = GBT_ERROR_OTHER;
		GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
		return;
	}
	
	ucObjId = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
	m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucObjId;  
	iRecvIndex++;
	iSendIndex++;
	
	if ( iRecvIndex >= iRecvBufLen )
	{
		ucErrorSts = GBT_ERROR_OTHER;
		GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
		return;
	}
	m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] =
		m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex]; 
	ucIndexCnt = (m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex]>>6) & 0x3;  
	ucSubId    = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex] & 0x3F;     
	iRecvIndex++;
	iSendIndex++;
	
	if ( ucIndexCnt > 0 ) 
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		ucIdxFst = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucIdxFst;
		iRecvIndex++;
		iSendIndex++;
		ucIndexCnt--;
	}
	if ( ucIndexCnt > 0 ) 
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		ucIdxSnd = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucIdxSnd;
		iRecvIndex++;
		iSendIndex++;
		ucIndexCnt--;
	}
	if ( ucIndexCnt > 0 ) 
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		iRecvIndex++;
		iSendIndex++;
		ucIndexCnt--;
	}
		//ACE_DEBUG((LM_DEBUG,"%s:%d Extend object ucObjecid = %d\n",__FILE__,__LINE__,ucObjId));

	//m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = iSendIndex;
	switch ( ucObjId )
	{
	case OBJECT_EXT_TSC_STATUS:   //信号机扩展状态
		if ( GBT_SEEK_REQ == ucRecvOptType )  
		{
			SThreadMsg sTscMsg;
			sTscMsg.ulType       = TSC_MSG_EXSTATUS_READ;
			sTscMsg.ucMsgOpt     = ucDealDataIndex;
			sTscMsg.uiMsgDataLen = 0;
			sTscMsg.pDataBuf     = NULL;

			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(SThreadMsg));
			
			return;
		}
		else
		{
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		break;
	case OBJECT_IP:
		if ( GBT_SEEK_REQ == ucRecvOptType )  
		{
			
			if ( ucSubId != 0 )
			{
				GetCmuAndCtrl( m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf,iSendIndex , ucSubId);
			}
			else
			{
				GetCmuAndCtrl( m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf,iSendIndex);
			}
		}
		else if ( (GBT_SET_REQ == ucRecvOptType) || (GBT_SET_REQ_NOACK == ucRecvOptType) ) 
		{
			if ( ucSubId != 0 )
			{
				//ACE_DEBUG((LM_DEBUG,"%s:%d Extend object ucSubID = %d !\n",__FILE__,__LINE__,ucSubId));
				SetCmuAndCtrl(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf,iRecvIndex,ucSubId);
			}
			else
			{
				SetCmuAndCtrl(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf,iRecvIndex);
			}
			
			SThreadMsg sTscMsg;
			sTscMsg.ulType       = TSC_MSG_UPDATE_PARA;  
			sTscMsg.ucMsgOpt     = 0;
			sTscMsg.uiMsgDataLen = 0;
			sTscMsg.pDataBuf     = NULL;
			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));
		}
		break;
	case OBJECT_WATCH_PARA:    //监控类型参数 温度 电压 门
		if ( GBT_SEEK_REQ == ucRecvOptType )  
		{
			GetWatchPara(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf,&iSendIndex);     
		}
		else
		{
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		break;
	case OBJECT_MODULE_STATUS:  
		if (  GBT_SEEK_REQ == ucRecvOptType )
		{
			GetModuleStatus(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf,&iSendIndex); 
		}
		else
		{
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		break;
	case OBJECT_YWFLASH_CFG:    //黄闪器扩展对象
		if ( GBT_SEEK_REQ == ucRecvOptType )  //查询
		{

			GetFlashCfg(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf,&iSendIndex);  

		}
		else if((GBT_SET_REQ == ucRecvOptType) || (GBT_SET_REQ_NOACK == ucRecvOptType)) //设置
		{
			SetFlashCtrl(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf,iRecvIndex);

		}
		break ;
	case OBJECT_POWERBOARD_CFG : //电源板配置扩展对象ADD:20140402
		if ( GBT_SEEK_REQ == ucRecvOptType )  //查询
		{
			
			Byte ucQueryType =( m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf)[iRecvIndex++] ;
			GetPowerCfg(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf,&iSendIndex,ucQueryType);  

		}
		else if((GBT_SET_REQ == ucRecvOptType) || (GBT_SET_REQ_NOACK == ucRecvOptType)) //设置
		{
			SetPowerCfg(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf,iRecvIndex);

		}
		break ;

	
	case OBJECT_DET_EXTCFG :    //检测器扩展配置对象
		if ( GBT_SEEK_REQ == ucRecvOptType )  //查询
		{
			Byte ucQueryType =( m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf)[iRecvIndex++] ;
			Byte ucBdIndx= (m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf)[iRecvIndex++] ;
			ACE_DEBUG((LM_DEBUG,"%s:%d,ucBdindex= %d ,ucQueryType = %02x\n",__FILE__,__LINE__,ucBdIndx+1,ucQueryType));
			GetDetCfg(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf,ucBdIndx,ucQueryType,&iSendIndex);  

		}
		else if((GBT_SET_REQ == ucRecvOptType) || (GBT_SET_REQ_NOACK == ucRecvOptType)) //设置
		{
			
			SetDetCfg(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf,iRecvIndex) ;
		}
		break ;

	case OBJECT_LAMPBOARD_CFG :    //灯控板灯泡检测和红绿冲突检测配置
		if ( GBT_SEEK_REQ == ucRecvOptType )  
		{
			;//以后再根据实际补充

		}
		else if((GBT_SET_REQ == ucRecvOptType) || (GBT_SET_REQ_NOACK == ucRecvOptType)) //设置
		{		 
			
			SetLampBdtCfg(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf,iRecvIndex); 
		}
		break ;
	case OBJECT_PSCBTN_NUM :    //模拟输入行人按钮值
		if ( GBT_SEEK_REQ == ucRecvOptType )  
		{
			;//以后再根据实际补充

		}
		else if((GBT_SET_REQ == ucRecvOptType) || (GBT_SET_REQ_NOACK == ucRecvOptType)) //设置
		{		 
			SetPscNum(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf,iRecvIndex); 
		}
		break ;
	case OBJECT_TMPPATTERN_CFG :    //临时12方向随机组合，放行60秒默认
		//ACE_DEBUG((LM_DEBUG,"%s:%d,ObjectId == OBJECT_TMPPATTERN_CFG\n",__FILE__,__LINE__));
		if ( GBT_SEEK_REQ == ucRecvOptType )  
		{
			;//以后再根据需要补充

		}
		else if((GBT_SET_REQ == ucRecvOptType) || (GBT_SET_REQ_NOACK == ucRecvOptType)) //设置
		{		 
			if(CManaKernel::CreateInstance()->bTmpPattern==true)
				return ;
			SetTmpPattern(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf,iRecvIndex); 
		}
		break ;

	case OBJECT_COMMAND_SIGNAL:
 		if ( GBT_SEEK_REQ == ucRecvOptType )  
		{
			;
		}
		else if((GBT_SET_REQ == ucRecvOptType) || (GBT_SET_REQ_NOACK == ucRecvOptType)) //设置
		{		 
			
			SetCommandSignal(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf,iRecvIndex); 
		}		
		break ;
	case OBJECT_SYSFUNC_CFG :    //系统其他功能配置
		if ( GBT_SEEK_REQ == ucRecvOptType )  
		{
			;//以后再根据需要补充

		}
		else if((GBT_SET_REQ == ucRecvOptType) || (GBT_SET_REQ_NOACK == ucRecvOptType)) //设置
		{		 
			
			SetSysFunc(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf,iRecvIndex); 
		}
		break ;	
	case OBJECT_GSM_CFG:
	   if((GBT_SET_REQ == ucRecvOptType) || (GBT_SET_REQ_NOACK == ucRecvOptType)) //设置
		{	 
			
			SetSmsFunc(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf,iRecvIndex,iRecvBufLen); 
		}
		break;
	default:
		ACE_DEBUG((LM_DEBUG,"%s:%d,ObjectId error objectId:%d\n",__FILE__,__LINE__,ucObjId));
		GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
		return;
	}	
	m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex = iRecvIndex;
	m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex = iSendIndex;
	m_sGbtDealData[ucDealDataIndex].iObjNum--;
	
	ACE_DEBUG((LM_DEBUG,"%s:%d iRecvIndex = %d iRecvBufLen = %d\n",__FILE__,__LINE__,iRecvIndex,iRecvBufLen));
	if ( iRecvIndex == iRecvBufLen )  
	{
		if ( 0 == m_sGbtDealData[ucDealDataIndex].iObjNum ) 
		{
			m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = iSendIndex;
			GotoSendToHost(ucDealDataIndex);
			return;
		}
		else
		{
			ucErrorSts = 5;
			ucErrorIdx = 0;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
	}
	else
	{
		GotoDealRecvbuf(ucDealDataIndex);
		return;
	}
}


/**************************************************************
Function:       CGbtMsgQueue::GetCmuAndCtrl
Description:    通信及控制方式参数扩展对象(0xf6)全部子对象查询		
Input:          iSendIndex   发送枕地址索引
Output:         pBuf  查询结果缓存指针
Return:         无
***************************************************************/
void CGbtMsgQueue::GetCmuAndCtrl(Byte* pBuf,int& iSendIndex)
{
	Byte ucTmp       = 0;
	Byte pHwEther[6] = {0};
	Byte pIp[4]      = {0};
	Byte pMask[4]    = {0};
	Byte pGateway[4] = {0};
	STscConfig* pTscCfg = CManaKernel::CreateInstance()->m_pTscConfig;

	GetNetPara(pHwEther , pIp , pMask , pGateway);

	//设备动作取值
	pBuf[iSendIndex++] = 0;
	pBuf[iSendIndex++] = 0;

	ucTmp = 0;
	if ( pTscCfg->sSpecFun[FUN_DOOR].ucValue > 0 )
	{
		ucTmp |= 1<<5;
	}
	if ( pTscCfg->sSpecFun[FUN_VOLTAGE].ucValue > 0 )
	{
		ucTmp |= 1<<6;
	}
	if ( pTscCfg->sSpecFun[FUN_TEMPERATURE].ucValue > 0 )
	{
		ucTmp |= 1<<7;
	}
	pBuf[iSendIndex++] = ucTmp;

	ucTmp = 0;
	if ( pTscCfg->sSpecFun[FUN_MSG_ALARM].ucValue > 0 )
	{
		ucTmp |= 1<<5;
	}
	if ( pTscCfg->sSpecFun[FUN_GPS].ucValue > 0 )
	{
		ucTmp |= 1<<6;
	}
	if ( pTscCfg->sSpecFun[FUN_COUNT_DOWN].ucValue > 0 )
	{
		ucTmp |= 1<<7;
	}
	pBuf[iSendIndex++] = ucTmp;
	
	ucTmp = pTscCfg->sSpecFun[FUN_PRINT_FLAG].ucValue;
	pBuf[iSendIndex++] = ucTmp;

	ucTmp = 0;
	ucTmp = pTscCfg->sSpecFun[FUN_PRINT_FLAGII].ucValue;
	if ( pTscCfg->sSpecFun[FUN_SERIOUS_FLASH].ucValue > 0 )
	{
		ucTmp |= 1<<7;
	}
	pBuf[iSendIndex++] = ucTmp;

		//工作方式
	ucTmp = pTscCfg->sSpecFun[FUN_CROSS_TYPE].ucValue;
	pBuf[iSendIndex++] = ucTmp;
	ucTmp = pTscCfg->sSpecFun[FUN_STAND_STAGEID].ucValue;
	pBuf[iSendIndex++] = ucTmp;
	ucTmp = pTscCfg->sSpecFun[FUN_CORSS1_STAGEID].ucValue;
	pBuf[iSendIndex++] = ucTmp;
	ucTmp = pTscCfg->sSpecFun[FUN_CROSS2_STAGEID].ucValue;
	pBuf[iSendIndex++] = ucTmp;

	//通信接口
	ucTmp = pTscCfg->sSpecFun[FUN_COMMU_PARA].ucValue;
	pBuf[iSendIndex++] = ucTmp;

	//端口号
	ucTmp = pTscCfg->sSpecFun[FUN_PORT_HIGH].ucValue;
	pBuf[iSendIndex++] = ucTmp;
	ucTmp = pTscCfg->sSpecFun[FUN_PORT_LOW].ucValue;
	pBuf[iSendIndex++] = ucTmp;

	//ip
	pBuf[iSendIndex]   = pIp[0];
	pBuf[iSendIndex+1] = pIp[1];
	pBuf[iSendIndex+2] = pIp[2];
	pBuf[iSendIndex+3] = pIp[3];
	iSendIndex += 16;

	//net mask
	pBuf[iSendIndex]   = pMask[0];
	pBuf[iSendIndex+1] = pMask[1];
	pBuf[iSendIndex+2] = pMask[2];
	pBuf[iSendIndex+3] = pMask[3];
	iSendIndex += 16;

	//gate way
	pBuf[iSendIndex]   = pGateway[0];
	pBuf[iSendIndex+1] = pGateway[1];
	pBuf[iSendIndex+2] = pGateway[2];
	pBuf[iSendIndex+3] = pGateway[3];
	iSendIndex += 16;
	
	/*
	pBuf[iSendIndex++] = pHwEther[0];
	pBuf[iSendIndex++] = pHwEther[1];
	pBuf[iSendIndex++] = pHwEther[2];
	pBuf[iSendIndex++] = pHwEther[3];
	pBuf[iSendIndex++] = pHwEther[4];
	pBuf[iSendIndex++] = pHwEther[5];
	*/
}


/**************************************************************
Function:       CGbtMsgQueue::GetCmuAndCtrl
Description:    通信及控制方式参数扩展对象(0xf6)，子对象查询		
Input:          iSendIndex   发送枕地址索引
				ucSubId      子对象id
Output:         pBuf  查询结果缓存指针
Return:         无
***************************************************************/
void CGbtMsgQueue::GetCmuAndCtrl(Byte* pBuf,int& iSendIndex , Byte ucSubId)
{
	Byte ucTmp       = 0;
	Byte pHwEther[6] = {0};
	Byte pIp[4]      = {0};
	Byte pMask[4]    = {0};
	Byte pGateway[4] = {0};
	STscConfig* pTscCfg = CManaKernel::CreateInstance()->m_pTscConfig;

	switch ( ucSubId )
	{
		case 1:   //设备动作取值
			pBuf[iSendIndex++] = 0;
			pBuf[iSendIndex++] = 0;
			break;
		case 2: //?设备启用字
			ucTmp = 0;
			if ( pTscCfg->sSpecFun[FUN_DOOR].ucValue > 0 )
			{
				ucTmp |= 1<<5;
			}
			if ( pTscCfg->sSpecFun[FUN_VOLTAGE].ucValue > 0 )
			{
				ucTmp |= 1<<6;
			}
			if ( pTscCfg->sSpecFun[FUN_TEMPERATURE].ucValue > 0 )
			{
				ucTmp |= 1<<7;
			}
			pBuf[iSendIndex++] = ucTmp;

			ucTmp = 0;
			if ( pTscCfg->sSpecFun[FUN_MSG_ALARM].ucValue > 0 )
			{
				ucTmp |= 1<<5;
			}
			if ( pTscCfg->sSpecFun[FUN_GPS].ucValue > 0 )
			{
				ucTmp |= 1<<6;
			}
			if ( pTscCfg->sSpecFun[FUN_COUNT_DOWN].ucValue > 0 )
			{
				ucTmp |= 1<<7;
			}
			pBuf[iSendIndex++] = ucTmp;

			ucTmp = pTscCfg->sSpecFun[FUN_PRINT_FLAG].ucValue;
			pBuf[iSendIndex++] = ucTmp;

			ucTmp = 0;
			ucTmp = pTscCfg->sSpecFun[FUN_PRINT_FLAGII].ucValue;
			if ( pTscCfg->sSpecFun[FUN_SERIOUS_FLASH].ucValue > 0 )
			{
				ucTmp |= 1<<7;
			}
			pBuf[iSendIndex++] = ucTmp;
			break;
		case 3: //工作方式
			ucTmp = pTscCfg->sSpecFun[FUN_CROSS_TYPE].ucValue;
			pBuf[iSendIndex++] = ucTmp;
			ucTmp = pTscCfg->sSpecFun[FUN_STAND_STAGEID].ucValue;
			pBuf[iSendIndex++] = ucTmp;
			ucTmp = pTscCfg->sSpecFun[FUN_CORSS1_STAGEID].ucValue;
			pBuf[iSendIndex++] = ucTmp;
			ucTmp = pTscCfg->sSpecFun[FUN_CROSS2_STAGEID].ucValue;
			pBuf[iSendIndex++] = ucTmp;
			break;
		case 4: //通信接口
			ucTmp = pTscCfg->sSpecFun[FUN_COMMU_PARA].ucValue;
			pBuf[iSendIndex++] = ucTmp;
			break;
		case 5:   //端口号
			ucTmp = pTscCfg->sSpecFun[FUN_PORT_HIGH].ucValue;
			pBuf[iSendIndex++] = ucTmp;
			ucTmp = pTscCfg->sSpecFun[FUN_PORT_LOW].ucValue;
			pBuf[iSendIndex++] = ucTmp;
			break;
		case 6:  //ip
			GetNetPara(pHwEther , pIp , pMask , pGateway);
			pBuf[iSendIndex]   = pIp[0];
			pBuf[iSendIndex+1] = pIp[1];
			pBuf[iSendIndex+2] = pIp[2];
			pBuf[iSendIndex+3] = pIp[3];
			iSendIndex += 16;
			break;
		case 7: //net mask
			GetNetPara(pHwEther , pIp , pMask , pGateway);
			pBuf[iSendIndex]   = pMask[0];
			pBuf[iSendIndex+1] = pMask[1];
			pBuf[iSendIndex+2] = pMask[2];
			pBuf[iSendIndex+3] = pMask[3];
			iSendIndex += 16;
			break;
		case 8: //gate way
			GetNetPara(pHwEther , pIp , pMask , pGateway);
			pBuf[iSendIndex]   = pGateway[0];
			pBuf[iSendIndex+1] = pGateway[1];
			pBuf[iSendIndex+2] = pGateway[2];
			pBuf[iSendIndex+3] = pGateway[3];
			iSendIndex += 16;
			break;
		case 9:  //倒计时类型
			ucTmp = pTscCfg->sSpecFun[FUN_COUNT_DOWN].ucValue;
			pBuf[iSendIndex++] = ucTmp;
			ucTmp = pTscCfg->sSpecFun[FUN_CNTTYPE].ucValue;
			pBuf[iSendIndex++] = ucTmp;
			break;
		/*case 10:  //灯泡检测
			ucTmp = pTscCfg->sSpecFun[FUN_LIGHTCHECK].ucValue;
			pBuf[iSendIndex++] = ucTmp;
			break;*/
		default:
			break;

	}
}


/**************************************************************
Function:       CGbtMsgQueue::SetCmuAndCtrl
Description:    通信及控制方式参数扩展对象(0xf6)，全部子对象值设置		
Input:          pBuf   设置值缓存地址指针
				iRecvIndex     当前设置值地址
Output:        
Return:         无
***************************************************************/
void CGbtMsgQueue::SetCmuAndCtrl(Byte* pBuf,int& iRecvIndex)
{
	Byte ucTmp             = 0;
	Byte cIp[4]            = {0};
	Byte cMask[4]          = {0};
	Byte cGateWay[4]       = {0};
	
	STscConfig* pTscCfg = CManaKernel::CreateInstance()->m_pTscConfig;

	/*************设备动作取值*********/
	ucTmp       = pBuf[iRecvIndex++];
	ucTmp       = pBuf[iRecvIndex++];
	//bit0  重启系统
	//bit1  进入自检
	//bit2  进入升级
	//bit3  重启程序
	//bit4  清除严重事件
	//bit5  清除事件
	//bit6  删除统计数据
	//bit7  删除日志
	//bit8  设置通信参数
	//bit15 对协议扩展

	/***********设备启用字************/
	/*        bit0   bit1     bit2     bit3    bit4    bit5    bit6    bit7
	* Byte0                                    门开关   电压     温度计
	* Byte1                                    短信     gps     倒计时
	* Byte2  检测器  发电源板   收电源板  发灯控板 收灯控板 发黄闪器 收黄闪器 步伐信息
	* Byte3  倒计时  StartTime costTime                                 严重错误黄闪 
	*/
	ucTmp = pBuf[iRecvIndex++];
	pTscCfg->sSpecFun[FUN_DOOR].ucValue        = (ucTmp>>5) & 1;
	pTscCfg->sSpecFun[FUN_VOLTAGE].ucValue     = (ucTmp>>6) & 1;
	pTscCfg->sSpecFun[FUN_TEMPERATURE].ucValue = (ucTmp>>7) & 1;
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_DOOR+1        , (ucTmp>>5) & 1 );
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_VOLTAGE+1     , (ucTmp>>6) & 1 );
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_TEMPERATURE+1 , (ucTmp>>7) & 1 );

	ucTmp = pBuf[iRecvIndex++];
	pTscCfg->sSpecFun[FUN_MSG_ALARM].ucValue  = (ucTmp>>5) & 1;
	pTscCfg->sSpecFun[FUN_GPS].ucValue        = (ucTmp>>6) & 1;
	pTscCfg->sSpecFun[FUN_COUNT_DOWN].ucValue = (ucTmp>>7) & 1;
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_MSG_ALARM+1  , (ucTmp>>5) & 1 );
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_GPS+1        , (ucTmp>>6) & 1 );
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_COUNT_DOWN+1 , (ucTmp>>7) & 1 );

	ucTmp = pBuf[iRecvIndex++];
	pTscCfg->sSpecFun[FUN_PRINT_FLAG].ucValue  = ucTmp;
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_PRINT_FLAG+1 , ucTmp);

	ucTmp = pBuf[iRecvIndex++];
	pTscCfg->sSpecFun[FUN_PRINT_FLAGII].ucValue  = ucTmp & 0x7F;
	pTscCfg->sSpecFun[FUN_SERIOUS_FLASH].ucValue = (ucTmp>>7) & 1; 
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_PRINT_FLAGII+1  , ucTmp & 0x7F );
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_SERIOUS_FLASH+1 , (ucTmp>>7) & 1 );
	
	/***********工作方式***********/
	ucTmp = pBuf[iRecvIndex++];
	//pTscCfg->sSpecFun[FUN_CROSS_TYPE].ucValue  = ucTmp;
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_CROSS_TYPE+1    , ucTmp);
	ucTmp = pBuf[iRecvIndex++];
	//pTscCfg->sSpecFun[FUN_STAND_STAGEID].ucValue  = ucTmp;
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_STAND_STAGEID+1  , ucTmp);
	ucTmp = pBuf[iRecvIndex++];
	//pTscCfg->sSpecFun[FUN_CORSS1_STAGEID].ucValue  = ucTmp;
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_CORSS1_STAGEID+1 , ucTmp);
	ucTmp = pBuf[iRecvIndex++];
	//pTscCfg->sSpecFun[FUN_CROSS2_STAGEID].ucValue  = ucTmp;
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_CROSS2_STAGEID+1 , ucTmp);

	/**********通信接口***********/
	ucTmp       = pBuf[iRecvIndex++];
	pTscCfg->sSpecFun[FUN_COMMU_PARA].ucValue  = ucTmp;
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_COMMU_PARA+1 , ucTmp);
	//bit0 通信使用null设备
	//bit1 1-TCP  0-UDP
	//bit2 1-IPV6 0-IPV4

	//端口号 两字节 取值2048-16768
	ucTmp       = pBuf[iRecvIndex++];
	pTscCfg->sSpecFun[FUN_PORT_HIGH].ucValue  = ucTmp;
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_PORT_HIGH+1 , ucTmp);
	ucTmp       = pBuf[iRecvIndex++];
	pTscCfg->sSpecFun[FUN_PORT_LOW].ucValue  = ucTmp;
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_PORT_LOW+1 , ucTmp);

	CManaKernel::CreateInstance()->SetUpdateBit();

	//ip
	cIp[0] = *(pBuf+iRecvIndex);
	cIp[1] = *(pBuf+iRecvIndex + 1);
	cIp[2] = *(pBuf+iRecvIndex + 2);
	cIp[3] = *(pBuf+iRecvIndex + 3);
	iRecvIndex += 16;

	//网络掩码
	cMask[0] = *(pBuf+iRecvIndex);
	cMask[1] = *(pBuf+iRecvIndex + 1);
	cMask[2] = *(pBuf+iRecvIndex + 2);
	cMask[3] = *(pBuf+iRecvIndex + 3);
	iRecvIndex += 16;

	//网关IP地址
	cGateWay[0] = *(pBuf+iRecvIndex);
	cGateWay[1] = *(pBuf+iRecvIndex + 1);
	cGateWay[2] = *(pBuf+iRecvIndex + 2);
	cGateWay[3] = *(pBuf+iRecvIndex + 3);
	iRecvIndex += 16;

	//物理地址 不让修改
	/*
	cHwEther[0] = *(pBuf+iRecvIndex);
	cHwEther[1] = *(pBuf+iRecvIndex + 1);
	cHwEther[2] = *(pBuf+iRecvIndex + 2);
	cHwEther[3] = *(pBuf+iRecvIndex + 3);
	cHwEther[4] = *(pBuf+iRecvIndex + 4);
	cHwEther[5] = *(pBuf+iRecvIndex + 5);
	iRecvIndex += 6;
	*/

	ReworkNetPara(cIp,cMask,cGateWay);

}


/**************************************************************
Function:       CGbtMsgQueue::SetCmuAndCtrl
Description:    通信及控制方式参数扩展对象(0xf6)，子对象值设置		
Input:          pBuf   设置值缓存地址指针
				iRecvIndex     当前设置值地址
				ucSubId        子对象ID
Output:        
Return:         无
***************************************************************/
void CGbtMsgQueue::SetCmuAndCtrl(Byte* pBuf,int& iRecvIndex , Byte ucSubId)
{
	Byte ucTmp0       = 0;
	Ushort ucTmp     = 0;
	Byte cIp[4]      = {0};
	Byte cMask[4]    = {0};
	Byte cGateWay[4] = {0};
	STscConfig* pTscCfg = CManaKernel::CreateInstance()->m_pTscConfig;
			//bit0  重启系统
			//bit1  进入自检
			//bit2  进入升级
			//bit3  重启程序
			//bit4  清除严重事件
			//bit5  清除事件
			//bit6  删除统计数据
			//bit7  删除日志
			//bit8  设置通信参数
			//bit15 对协议扩展
	switch ( ucSubId )
	{
		case 1:  //设备动作取值			
			ucTmp0 = (pBuf[iRecvIndex++]<<8);
			ucTmp0 |=  pBuf[iRecvIndex++];
			//ACE_DEBUG((LM_DEBUG,"%s:%d Action = %d !\n",__FILE__,__LINE__,ucTmp0));
			for(Byte ucActionIndex = 0 ;ucActionIndex<sizeof(Ushort);ucActionIndex++)
			{				
				//bit0重启系统	bit1进入自检	 bit2 进入升级 bit3重启程序 bit4清除严重事件
				//bit5清除事件  bit6删除统计数据 bit7删除日志 bit8设置通信参数 bit15对协议扩展
				if((ucTmp0>>ucActionIndex) &0x1)								
			
				{
					switch (ucActionIndex)
					{
						case 0:
							ACE_DEBUG((LM_DEBUG,"%s:%d Get reboot commond,system will reboot now !\n",__FILE__,__LINE__));
							system("reboot");
							return ;
							break ;	

					}

				}

			}		
			break;
		case 2:  //设备启用字//设备启用字,可以启用特殊功能FUN表中的设备 81 f6 02 aa bb cc bb cc ，aa 表示启用设备类型数, bb表示设备索引，cc表示启用值.
			ucTmp = pBuf[iRecvIndex++];
			pTscCfg->sSpecFun[FUN_DOOR].ucValue         = (ucTmp>>5) & 1;
			pTscCfg->sSpecFun[FUN_VOLTAGE].ucValue      = (ucTmp>>6) & 1;
			pTscCfg->sSpecFun[FUN_TEMPERATURE].ucValue  = (ucTmp>>7) & 1;
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_DOOR+1       , (ucTmp>>5) & 1 );
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_VOLTAGE+1    , (ucTmp>>6) & 1 );
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_TEMPERATURE+1, (ucTmp>>7) & 1 );

			ucTmp = pBuf[iRecvIndex++];
			pTscCfg->sSpecFun[FUN_MSG_ALARM].ucValue  = (ucTmp>>5) & 1;
			pTscCfg->sSpecFun[FUN_GPS].ucValue        = (ucTmp>>6) & 1;
			pTscCfg->sSpecFun[FUN_COUNT_DOWN].ucValue = (ucTmp>>7) & 1;
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_MSG_ALARM+1  , (ucTmp>>5) & 1 );
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_GPS+1        , (ucTmp>>6) & 1 );
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_COUNT_DOWN+1 , (ucTmp>>7) & 1 );

			ucTmp = pBuf[iRecvIndex++];
			pTscCfg->sSpecFun[FUN_PRINT_FLAG].ucValue = ucTmp;
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_PRINT_FLAG+1 , ucTmp);

			ucTmp = pBuf[iRecvIndex++];
			pTscCfg->sSpecFun[FUN_PRINT_FLAGII].ucValue = ucTmp & 0x7F;
			pTscCfg->sSpecFun[FUN_SERIOUS_FLASH].ucValue = (ucTmp>>7)&1;
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_PRINT_FLAGII+1  , ucTmp & 0x7F );
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_SERIOUS_FLASH+1 , (ucTmp>>7)&1 );
			break;
		case 3: //工作方式
			ucTmp = pBuf[iRecvIndex++];
			//pTscCfg->sSpecFun[FUN_CROSS_TYPE].ucValue = ucTmp ;
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_CROSS_TYPE+1     , ucTmp);
			ucTmp = pBuf[iRecvIndex++];
			//pTscCfg->sSpecFun[FUN_STAND_STAGEID].ucValue = ucTmp;
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_STAND_STAGEID+1  , ucTmp);
			ucTmp = pBuf[iRecvIndex++];
			//pTscCfg->sSpecFun[FUN_CORSS1_STAGEID].ucValue = ucTmp;
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_CORSS1_STAGEID+1 , ucTmp);
			ucTmp = pBuf[iRecvIndex++];
			//pTscCfg->sSpecFun[FUN_CROSS2_STAGEID].ucValue = ucTmp;
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_CROSS2_STAGEID+1 , ucTmp);
			CManaKernel::CreateInstance()->SetUpdateBit();
			break;
		case 4:  //通信接口
			ucTmp       = pBuf[iRecvIndex++];
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_COMMU_PARA+1 , ucTmp);
			break;
		case 5:  //端口号
			ucTmp       = pBuf[iRecvIndex++];
			pTscCfg->sSpecFun[FUN_PORT_HIGH].ucValue = ucTmp;
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_PORT_HIGH+1 , ucTmp);
			ucTmp       = pBuf[iRecvIndex++];
			pTscCfg->sSpecFun[FUN_PORT_LOW].ucValue = ucTmp;
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_PORT_LOW+1 , ucTmp);
			break;
		case 6:  //ip
			cIp[0] = *(pBuf+iRecvIndex);
			cIp[1] = *(pBuf+iRecvIndex + 1);
			cIp[2] = *(pBuf+iRecvIndex + 2);
			cIp[3] = *(pBuf+iRecvIndex + 3);
			iRecvIndex += 16;
			ReworkNetPara(cIp,NULL,NULL);
			break;
		case 7: //掩码
			cMask[0] = *(pBuf+iRecvIndex);
			cMask[1] = *(pBuf+iRecvIndex + 1);
			cMask[2] = *(pBuf+iRecvIndex + 2);
			cMask[3] = *(pBuf+iRecvIndex + 3);
			iRecvIndex += 16;
			ReworkNetPara(NULL,cMask,NULL);
			break;
		case 8:  //网关
			cGateWay[0] = *(pBuf+iRecvIndex);
			cGateWay[1] = *(pBuf+iRecvIndex + 1);
			cGateWay[2] = *(pBuf+iRecvIndex + 2);
			cGateWay[3] = *(pBuf+iRecvIndex + 3);
			iRecvIndex += 16;
			ReworkNetPara(NULL,NULL,cGateWay);
			break;
		case 9: //倒计时类型
			ucTmp = pBuf[iRecvIndex++];
			pTscCfg->sSpecFun[FUN_COUNT_DOWN].ucValue = ucTmp;
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_COUNT_DOWN+1 , ucTmp);
			ucTmp = pBuf[iRecvIndex++];
			pTscCfg->sSpecFun[FUN_CNTTYPE].ucValue = ucTmp;
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_CNTTYPE+1 , ucTmp);	
			break;
		/*case 10:
			ucTmp = pBuf[iRecvIndex++];
			pTscCfg->sSpecFun[FUN_LIGHTCHECK].ucValue = ucTmp;
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_LIGHTCHECK+1 , ucTmp);
			break;*/
		default:
			break;
	}
}


/**************************************************************
Function:       CGbtMsgQueue::GetWatchPara
Description:    获取监控参数扩展对象值		
Input:          iSendIndex  当前发送帧字节地址
Output:         pBuf  发送帧地址
Return:         无
***************************************************************/
void CGbtMsgQueue::GetWatchPara(Byte* pBuf,int *iSendIndex)
{
	CFlashMac* pFlashMac = CFlashMac::CreateInstance();

	bool bForDoor        = pFlashMac->m_bGetForDoor;   //前门打开
	bool bPowerType      = pFlashMac->m_bPowerType;    //供电类型 true:交流电 false:太阳能
	bool bAlarmStatus    = false;//CPowerBoard::CreateInstance()->m_bGetAlarmStatus;   //报警器状态
	Byte ucDoorValue     = 0;
	int  iTemperature    = pFlashMac->m_iTemperature;  //温度
	int  iVoltage        = pFlashMac->m_iVoltage;       //电压

	//温度值
	if ( iTemperature > -65 && iTemperature < 120 )
	{
		iTemperature += 65;
	}
	else if ( iTemperature < -65 )
	{
		iTemperature = 0;
	}
	else if ( iTemperature > 120 )
	{
		iTemperature = 185;
	}

	pBuf[*iSendIndex] = (iTemperature >> 8) & 0xff; //高字节在低位
	*iSendIndex += 1;

	pBuf[*iSendIndex] = iTemperature & 0xff;
	*iSendIndex += 1;

	//门开关值  校时设备.....
	if ( bForDoor && bAlarmStatus )   //门打开且当前处于报警状态
	{
		ucDoorValue = 0;
	}
	else if ( bForDoor && !bAlarmStatus ) 
	{
		ucDoorValue = 0x03;
	}
	else
	{
		ucDoorValue = 0x02;
	}
	pBuf[*iSendIndex] = ucDoorValue;
	*iSendIndex += 1;
	//电压值
	pBuf[*iSendIndex] = (iVoltage >> 8) & 0xff; //高字节在低位
	*iSendIndex += 1;

	pBuf[*iSendIndex] = iVoltage & 0xff;
	*iSendIndex += 1;


	//供电类型
	if ( bPowerType )
	{
		pBuf[*iSendIndex] = 0;
		*iSendIndex += 1;
	}
	else
	{
		pBuf[*iSendIndex] = 2;
		*iSendIndex += 1;
	}
}


/**************************************************************
Function:       CGbtMsgQueue::SetSmsFunc
Description:    设置GSM接收AT命令
Input:          pBuf   接收AT设置命令缓存指针
		       iRecvIndex     当前设置取值地址
Output:         无
Return:         无
***************************************************************/

void CGbtMsgQueue::SetSmsFunc(Byte* pBuf,int& iRecvIndex ,int iRecvBufLen)
{
	char tmpstr[400] = {0};	
	/*
	CSerialCtrl* pGsmSerial = CSerialCtrl::CreateInstance() ;	
	int fd = pGsmSerial->GetSerialFd(1);
	if(pBuf[iRecvIndex] == 0x1)
	{		
		char sendNum[20]={0};
		//char sendsms[300]={0};
		ACE_OS::memcpy(sendNum,pBuf+4,11);
		ACE_OS::memcpy(tmpstr,pBuf+15,ACE_OS::strlen((char*)pBuf+15));
		//ACE_OS::printf("%s:%d sendnum=%s sendtext= %s \n",__FILE__,__LINE__,sendNum,sendsms);
		CGsm::CreateInstance()->SendSms(sendNum,tmpstr);
	}
	else if(pBuf[iRecvIndex] == 0x2)
	{
		char smca[20]={0};
		ACE_OS::memcpy(smca,pBuf+4,11);
		ACE_OS::sprintf(tmpstr,"AT+CSCA=\"+86%s\", 145\r\n",smca);
		ACE_OS::printf("%s:%d set smca:%s \n",__FILE__,__LINE__,tmpstr);
		pGsmSerial->serialWrite(tmpstr,fd);
	}
	
	else if(pBuf[iRecvIndex] == 0x3)
	{
		switch(pBuf[iRecvIndex+1])
		{
			case 0:
				ACE_OS::strcpy(tmpstr,"AT+IPR=4800\r\n");
			break;
			case 1:
				ACE_OS::strcpy(tmpstr,"AT+IPR=9600\r\n");
			break ;
			case 2:
				ACE_OS::strcpy(tmpstr,"AT+IPR=19200\r\n");
			break;
			case 3:
				ACE_OS::strcpy(tmpstr,"AT+IPR=38400\r\n");
			break ;
			case 4:
				ACE_OS::strcpy(tmpstr,"AT+IPR=115200\r\n");
			break ;
			default:
				ACE_OS::strcpy(tmpstr,"AT+IPR=38400\r\n");
			break ;
		}
		
		ACE_OS::printf("%s:%d set IPR:%s \n",__FILE__,__LINE__,tmpstr);
		pGsmSerial->serialWrite(tmpstr,fd);
	}
	else if(pBuf[iRecvIndex] == 0x4)
	{
		
		//CGsm::CreateInstance()->OpenTcpConnec();
	}
	else 
	{		
		ACE_OS::memcpy(tmpstr,(char *)pBuf+3,iRecvBufLen-3);
		ACE_OS::strcat(tmpstr,"\r\n");
		ACE_OS::printf("%s:%d ModifyBotrate:%s \n",__FILE__,__LINE__,tmpstr);
		pGsmSerial->serialWrite(tmpstr,fd);
	}
	iRecvIndex += iRecvBufLen-3 ;*/
}


/**************************************************************
Function:       CGbtMsgQueue::GetModuleStatus
Description:    获取模块状态对象值		
Input:          iSendIndex  当前发送帧字节地址
Output:         pBuf  发送帧地址
Return:         无
***************************************************************/
void CGbtMsgQueue::GetModuleStatus(Byte* pBuf,int *iSendIndex)
{
	Byte ucTmp = 0;
	CDetector* pDet          = CDetector::CreateInstance();
	CLampBoard* pLamp        = CLampBoard::CreateInstance();
	CFlashMac* pFlashMac     = CFlashMac::CreateInstance();
		//灯控板
	if ( pLamp->m_bRecordSts[0] != true )
	{
		ucTmp |= 1;
	}
	if (  pLamp->m_bRecordSts[1] != true )
	{
		ucTmp |= 1<<1;
	}
	if ( pLamp->m_bRecordSts[2] != true )
	{
		ucTmp |= 1<<2;
	}
	if ( pLamp->m_bRecordSts[3] != true )
	{
		ucTmp |= 1<<3;
	}

	//检测器板
	if ( pDet->m_bRecordSts[0] != true )
	{
		ucTmp |= 1<<4;
	}
	if ( pDet->m_bRecordSts[1] != true )
	{
		ucTmp |= 1<<5;
	}
	if ( pDet->m_bRecordSts[2] != true)
	{
		ucTmp |= 1<<6;
	}
	if ( pDet->m_bRecordSts[3] != true )
	{
		ucTmp |= 1<<7;
	}
	pBuf[*iSendIndex] = ucTmp;   
	*iSendIndex      += 1;
	ucTmp             = 0;

	//电源板
	if ( false )//if ( pPowerBoard->m_bRecordSts != true )
	{
		ucTmp |= 1;
	}
	
	//监控板 黄闪器板
	if ( pFlashMac->m_bRecordSts != true )
	{
		ucTmp |= 1<<1;
	}
	pBuf[*iSendIndex] = ucTmp;   
	*iSendIndex      += 1;

	pBuf[*iSendIndex] = 0;   
	*iSendIndex      += 1;

	pBuf[*iSendIndex] = 0;   
	*iSendIndex      += 1;
}

/**************************************************************
Function:       CGbtMsgQueue::SetPscNum
Description:    设置行人按钮值		
Input:          pBuf  接收帧缓存地址
Output:         iRecvIndex  接收缓存当前读取地址
Return:         无
***************************************************************/
void CGbtMsgQueue::SetPscNum(Byte* pBuf,int& iRecvIndex)
{
	CPscMode::CreateInstance()->m_ucBtnNum = pBuf[iRecvIndex++];
}

/**************************************************************
Function:       CGbtMsgQueue::SetTmpPattern
Description:    设置临时方向相位放行，默认60秒
Input:          pBuf  接收帧缓存地址
Output:         iRecvIndex  接收缓存当前读取地址
Return:         无
***************************************************************/
void CGbtMsgQueue::SetTmpPattern(Byte* pBuf,int& iRecvIndex)
{	
	CManaKernel *pManaKernel = CManaKernel::CreateInstance();
	
	
	if(pManaKernel->m_iTimePatternId == 0)
	{	
		pManaKernel->bTmpPattern = true ;
		Ushort TmpPhase = pBuf[iRecvIndex++];
		GBT_DB::StagePattern TmpStgPatterb ;
		TmpStgPatterb.usAllowPhase = TmpPhase ;
		TmpStgPatterb.ucGreenTime = 60 ;
		TmpStgPatterb.ucYellowTime = 0 ;
		TmpStgPatterb.ucRedTime = 0 ;
		TmpStgPatterb.ucOption = 0 ;
		(CDbInstance::m_cGbtTscDb).ModStagePattern(17, 1, TmpStgPatterb);		
		ACE_DEBUG((LM_DEBUG,"%s:%d usAllowPhase ==%d \n",__FILE__,__LINE__,TmpStgPatterb.usAllowPhase));
		pManaKernel->UpdateConfig();
		
		SThreadMsg sTscMsg ;
		pManaKernel->m_iTimePatternId = 251;
		sTscMsg.ulType       = TSC_MSG_PATTER_RECOVER;  //从特殊方案返回原来状态
		sTscMsg.ucMsgOpt     = 0;
		sTscMsg.uiMsgDataLen = 1;
		sTscMsg.pDataBuf     = NULL;
		CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));
		ACE_DEBUG((LM_DEBUG,"%s:%d TSC_MSG_TIMEPATTERN == 251 \n",__FILE__,__LINE__));
	 }
	 else
	 	return ;

}

/**************************************************************
Function:       CGbtMsgQueue::SetLampBdtCfg
Description:    设置灯控板灯泡检测和红绿冲突检测配置		
Input:          pBuf   接收设置缓存指针
				iRecvIndex     当前设置取值地址
Output:         无
Return:         无
***************************************************************/
void CGbtMsgQueue::SetLampBdtCfg(Byte* pBuf,int& iRecvIndex)
{
	
	Byte ucSetType = pBuf[iRecvIndex++];
	//0x03 表示类型
	if(ucSetType != 0x03)
		return ;
	CLampBoard *pLampBd = CLampBoard::CreateInstance();
	Byte ucBdIndex = pBuf[iRecvIndex++];
	pLampBd->m_ucCheckCfg[ucBdIndex] = pBuf[iRecvIndex++];
	if(pLampBd->m_ucCheckCfg[ucBdIndex] == 0xa)
		CManaKernel::CreateInstance()->m_pRunData->bIsChkLght = true ;
	else if(pLampBd->m_ucCheckCfg[ucBdIndex] == 0x5)
		CManaKernel::CreateInstance()->m_pRunData->bIsChkLght = false ;
	pLampBd->SendSingleCfg(ucBdIndex);
}

/**************************************************************
Function:       CGbtMsgQueue::SetCommandSignal
Description:   接收上位机控制信号机信号指令包括下一相位方向等.
Input:          pBuf   接收设置缓存指针
		     iRecvIndex     当前设置取值地址
Output:         无
Return:         无
***************************************************************/
void CGbtMsgQueue::SetCommandSignal(Byte* pBuf,int& iRecvIndex)
{
	CManaKernel *pManakernel = CManaKernel::CreateInstance();
	Byte Tmp = pBuf[iRecvIndex++];
	switch(Tmp)
	{
		case 0x01 :
		if(pBuf[iRecvIndex++] ==0x0)
		{
		
			if ( (pManakernel->m_pRunData->uiCtrl!= CTRL_PANEL && pManakernel->m_pRunData->uiCtrl != CTRL_MANUAL)|| (pManakernel->m_pRunData->uiWorkStatus!= STANDARD) )
			{
				return ;
			}	
			if( pManakernel->m_iTimePatternId == 250) //防止上位机方向切回下一相位的时候没有过渡
			{
				pManakernel->bNextDirec = false ;
				pManakernel->m_iTimePatternId = 0;
				pManakernel->bTmpPattern = false ;
			}
			if(pManakernel->m_bNextPhase == true)
				return ;
			else
			{
				pManakernel->m_bNextPhase = true ;
			}
			SThreadMsg sTscMsgSts;
			sTscMsgSts.ulType       = TSC_MSG_NEXT_STAGE;  //按阶段前进
			sTscMsgSts.ucMsgOpt     = 0;
			sTscMsgSts.uiMsgDataLen = 1;
			sTscMsgSts.pDataBuf     = NULL ;				
			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
							
		}			
		break ;
		case 0x02 :
		if ( (pManakernel->m_pRunData->uiCtrl!= CTRL_PANEL && pManakernel->m_pRunData->uiCtrl != CTRL_MANUAL)|| (pManakernel->m_pRunData->uiWorkStatus!= STANDARD) )
		{
				return ;
		}
		if(pBuf[iRecvIndex++] ==0x0)
		{						
			static int directype = 0 ;
			if(pManakernel->m_iTimePatternId == 0)
			{	
				pManakernel->bTmpPattern = true ;
				pManakernel->m_iTimePatternId = 250;//运行四方向切换				
			}
			SThreadMsg sTscMsg ;
			sTscMsg.ulType       = TSC_MSG_PATTER_RECOVER;  
			sTscMsg.ucMsgOpt     = (directype++)%4;
			sTscMsg.uiMsgDataLen = 0;			
			sTscMsg.pDataBuf     = NULL; 			
			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));	
		}
		break ;
		case 0x03 :
		{
			Byte Direc =pBuf[iRecvIndex++] ;
			if ( (pManakernel->m_pRunData->uiCtrl!= CTRL_PANEL && pManakernel->m_pRunData->uiCtrl != CTRL_MANUAL)|| (pManakernel->m_pRunData->uiWorkStatus!= STANDARD) )
			{
				return ;
			}			
			if(Direc<1 || Direc >4)   //方向0-北方 1-东方 2-南方 3-西方
				return ;
			if(pManakernel->m_iTimePatternId == 0)
			{	
				pManakernel->bTmpPattern = true ;
				pManakernel->m_iTimePatternId = 250;//运行四方向切换				
			}	
			SThreadMsg sTscMsg;
			sTscMsg.ulType       = TSC_MSG_PATTER_RECOVER;  
			sTscMsg.ucMsgOpt     = (Direc-1);
			sTscMsg.uiMsgDataLen = 0;			
			sTscMsg.pDataBuf     = NULL; 			
			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));
		}
		break ;
		default:
			return ;
	}
}


/**************************************************************
Function:       CGbtMsgQueue::SetSysFunc
Description:    设置其他系统功能配置	
Input:          pBuf   接收设置缓存指针
				iRecvIndex     当前设置取值地址
Output:         无
Return:         无
***************************************************************/
void CGbtMsgQueue::SetSysFunc(Byte* pBuf,int& iRecvIndex)
{
	CManaKernel *pManakernel = CManaKernel::CreateInstance();
	Byte Tmp = pBuf[iRecvIndex++];	
	//char SysEypSerial[32]={0};	
	switch(Tmp)
	{
		case 0x01 :			
			pManakernel->m_pRunData->uiUtcsHeartBeat = 0; //接收到心跳，累积置0
			if(pManakernel->bUTS == false)
			{
				pManakernel->bUTS = true ;
			}
			if(pManakernel->m_pRunData->uiCtrl != CTRL_UTCS && pManakernel->m_pRunData->uiCtrl != CTRL_PANEL)
			{
				pManakernel->m_iTimePatternId = 0;    //ADD:201311111530
				pManakernel->bTmpPattern = false ;     //ADD:201311111530
				if(pManakernel->bDegrade == true)
					pManakernel->bDegrade = false ;
				//SendTscCommand(OBJECT_SWITCH_CONTROL,12);
				pManakernel->SwitchCtrl(CTRL_UTCS);
				if(pManakernel->m_pRunData->uiWorkStatus != STANDARD) //从降级黄闪 熄灯 全红返回
					pManakernel->SwitchStatus(STANDARD);
				CMainBoardLed::CreateInstance()->DoModeLed(false,true);  //MOD指示灯正常
			}
		
			break ;
		case 0x02 :			
			(CDbInstance::m_cGbtTscDb).SetEypSerial();
			 //(CDbInstance::m_cGbtTscDb).GetEypSerial(SysEypSerial);
			 //ACE_DEBUG((LM_DEBUG,"%s:%d Get SysEypSerial = %s \n",__FILE__,__LINE__,SysEypSerial));
			break ;
		case 0x03 :	
			{ 
		   	 	Byte uEvtTypeId = 0 ;
				Uint uStartTime = 0 ;
				Uint uEndTime = 0 ;
				uEvtTypeId = pBuf[iRecvIndex++] ;
				ACE_DEBUG((LM_DEBUG,"%s:%d uEvtTypeId = %d \n",__FILE__,__LINE__,uEvtTypeId));
				for(Byte uNum = 0 ;uNum <4 ;uNum++)
				{
					uStartTime |= (pBuf[iRecvIndex++])<<(8*uNum);
				}
				for(Byte uNum = 0 ;uNum <4 ;uNum++)
				{
					uEndTime |= (pBuf[iRecvIndex++])<<(8*uNum);
				}
				//ACE_DEBUG((LM_DEBUG,"%s:%d StartTime = %0X \n",__FILE__,__LINE__,uStartTime));
				//ACE_DEBUG((LM_DEBUG,"%s:%d EndTime = %0X \n",__FILE__,__LINE__,uEndTime));
				 (CDbInstance::m_cGbtTscDb).DelEventLog(uEvtTypeId , uStartTime , uEndTime) ;
				
			break ;
			}		
		case 0x04 :
			{
			Byte updatetype = pBuf[iRecvIndex++] ;
			if(updatetype == 0x01)
			{
				ACE_OS::system("mv -f Gb.aiton Gb.aiton.bak");
			}
			else if(updatetype == 0x2)
			{
				ACE_OS::system("chmod 777 Gb*") ;
				ACE_OS::system("reboot") ;
			}
			else if(updatetype == 0x03)
			{
				ACE_OS::system("rm -f Gb.aiton") ;
				ACE_OS::system("mv -f Gb.aiton.bak Gb.aiton");
				ACE_OS::system("reboot") ;
			}
			
			break ;
			}
		case 0x05:
			{
			Byte updatetype = pBuf[iRecvIndex++] ;
			if(updatetype == 0x01)
			{
				ACE_OS::system("mv -f GbAitonTsc.db GbAitonTsc.db.bak");
			}
			else if(updatetype == 0x2)
			{
				ACE_OS::system("chmod 777 GbAitonTsc*") ;
				ACE_OS::system("reboot") ;
			}
			else if(updatetype == 0x03)
			{
				ACE_OS::system("rm -f GbAitonTsc.db") ;
				ACE_OS::system("mv -f GbAitonTsc.db.bak GbAitonTsc.db");
				ACE_OS::system("reboot") ;
			}
			
			break ;
			}
		case 0x06:
			{
				Byte Direc =pBuf[iRecvIndex++] ;
				if(Direc<0 || Direc >3)   //方向0-北方 1-东方 2-南方 3-西方
					return ;
				if(pManakernel->m_iTimePatternId == 0)
				{	
					pManakernel->bTmpPattern = true ;
					pManakernel->m_iTimePatternId = 250;//运行四方向切换				
				}	
				SThreadMsg sTscMsg;
				sTscMsg.ulType       = TSC_MSG_PATTER_RECOVER;  
				sTscMsg.ucMsgOpt     = Direc;
				sTscMsg.uiMsgDataLen = 0;			
				sTscMsg.pDataBuf     = NULL; 			
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));	
				break;
			}
		
		default:
			break ;
	}
	return ;

}



/**************************************************************
Function:       CGbtMsgQueue::GetFlashCfg
Description:    获取黄闪器配置信息		
Input:         	iSendIndex     发送帧当前写地址
Output:         pBuf   发送帧地址指针
Return:         无
***************************************************************/
void CGbtMsgQueue::GetFlashCfg(Byte* pBuf,int *iSendIndex)
{
	CFlashMac* pFlashMac = CFlashMac::CreateInstance();
	pFlashMac->FlashCfgGet();
	ACE_OS::sleep(ACE_Time_Value(0, 30000)); 
	
	pBuf[*iSendIndex] = pFlashMac->m_ucGetFlashRate & 0x0f;
	pBuf[*iSendIndex]|=  (pFlashMac->m_ucGetDutyCycle<<4) &0xf0 ;
	*iSendIndex += 1;

	pBuf[*iSendIndex] = (pFlashMac->m_ucGetSyType)&0xff ;
	*iSendIndex += 1;
	pBuf[*iSendIndex] = (pFlashMac->m_ucFlashStatus)&0xff;
	*iSendIndex += 1;

}


/**************************************************************
Function:       CGbtMsgQueue::SetFlashCtrl
Description:    黄闪器控制与配置信息设置	
Input:         	pBuf     接收帧地址指针
				iRecvIndex 接收帧当前读取地址
Output:        	无
Return:         无
***************************************************************/
void  CGbtMsgQueue::SetFlashCtrl(Byte* pBuf,int& iRecvIndex)
{
	Byte Tmp = pBuf[iRecvIndex++];
	CFlashMac* pFlashMac = CFlashMac::CreateInstance();
	pFlashMac->FlashForceEnd();
	
	switch(Tmp)
	{
		case 0x05 :
			pFlashMac->FlashForceEnd();
			break ;
		case 0x04 :			
			pFlashMac->FlashForceStart(pBuf[iRecvIndex++]);
			break ;
		case 0x03 :			
			pFlashMac->m_ucSetFlashRate = pBuf[iRecvIndex] & 0x0f;
			pFlashMac->m_ucSetDutyCycle = (pBuf[iRecvIndex++]>>4) & 0x0f;
			
			pFlashMac->m_ucSetSyType    = pBuf[iRecvIndex++] ;
				
			pFlashMac->FlashCfgSet();
			break ;
		default:
			break ;
	}
	return ;
}

/**************************************************************
	Function:		CGbtMsgQueue::GetPowerCfg
	Description:  电源板配置信息设置	
	Input:		pBuf	 接收帧地址指针
				iSendIndex     发送帧当前写地址
Output:         pBuf   发送帧地址指针
	Return: 		无
***************************************************************/
void  CGbtMsgQueue::GetPowerCfg(Byte* pBuf,int *iSendIndex ,Byte ucQueryType)
{
	CPowerBoard *pPowerBoard = CPowerBoard::CreateInstance();	
	switch(ucQueryType)
		{
		case 0x02 :    //请求电源板发送电压数据
			pPowerBoard->CheckVoltage();
			ACE_OS::sleep(ACE_Time_Value(0, 30000));
			pBuf[*iSendIndex] = pPowerBoard->m_iStongVoltage ;  //强电电压
			*iSendIndex += 1 ;		
			pBuf[*iSendIndex] = pPowerBoard->m_iWeakVoltage  ;  //弱电电?	
			*iSendIndex += 1 ;		
			pBuf[*iSendIndex] = pPowerBoard->m_iBusVoltage ;  //总线电压
			*iSendIndex += 1 ;
			break ;
		case 0x03 :	   //请求电源模块发送配置数据	
		{
			Byte VolPlan = 0 ;
			pPowerBoard->GetPowerBoardCfg();
			ACE_OS::sleep(ACE_Time_Value(0, 30000));
		//	printf("get powerd 0x3\n");
		
			pBuf[*iSendIndex] = pPowerBoard->m_iGetWarnHighVol;
			*iSendIndex += 1 ;
			pBuf[*iSendIndex] = //pPowerBoard->m_iGetWarnLowVol ;
			*iSendIndex += 1 ;
			VolPlan |= pPowerBoard->m_ucGetStongHighVolPlan ;
			VolPlan |= pPowerBoard->m_ucGetStongLowVolPlan <<2 ;
			VolPlan |=  pPowerBoard->m_ucGetWeakHighVolPlan << 4 ;
			VolPlan |= pPowerBoard->m_ucGetWeakLowVolPlan << 6 ;
			pBuf[*iSendIndex] = VolPlan ;
			*iSendIndex += 1 ;
			pBuf[*iSendIndex] = pPowerBoard->m_ucSetWatchCfg ;	
			*iSendIndex += 1 ;
			break ;
		//	printf("get powerd 0x3 -2\n");
		}
			
		 default:
		 	break ;
		}

}

/**************************************************************
	Function:		CGbtMsgQueue::SetPowerCfg
	Description:  电源板配置信息设置	
	Input:		pBuf	 接收帧地址指针
				iRecvIndex 接收帧当前读取地址
	Output: 		无
	Return: 		无
***************************************************************/
void  CGbtMsgQueue::SetPowerCfg(Byte* pBuf,int& iRecvIndex)	
{
	Byte Tmp = pBuf[iRecvIndex++];
	CPowerBoard *pPowerBoard = CPowerBoard::CreateInstance();
	switch(Tmp)
	{
		
		case 0x04 :	   //下发电源模块配置数据
		{
			Byte tmp1 = pBuf[iRecvIndex++];
			Byte tmp2 = pBuf[iRecvIndex++];
			Byte tmp3 = pBuf[iRecvIndex++];
			Byte tmp4 = pBuf[iRecvIndex++];
			pPowerBoard->SetPowerCfgData(tmp1,tmp2,tmp3,tmp4);
			pPowerBoard->SetPowerBoardCfg();
		}
			break ;
		case 0x05 :	   //心跳命令
			pPowerBoard->HeartBeat();			
			break ;
		default:
			break ;
	}
	return ;
}

/**************************************************************
Function:       CGbtMsgQueue::GetDetCfg
Description:    获取检测器信息	
Input:         	pBuf     待发送帧地址指针
				iSendIndex 发送帧当前写入地址
				ucBdIndex   检测器板索引
				ucQueryType 需查询的检测器项目
Output:        	无
Return:         无
***************************************************************/
void CGbtMsgQueue::GetDetCfg(Byte* pBuf,Byte ucBdIndex,Byte ucQueryType,int *iSendIndex)
{
	CDetector* pDetctor = CDetector::CreateInstance();
	Byte Tmp = ucQueryType;
	Byte ucBoardIndex = ucBdIndex ;
	ACE_Time_Value tv = ACE_Time_Value(0,150000);
	switch(Tmp)
	{
		
		case 0x03 :
			pDetctor->GetDecVars(ucBoardIndex,0x03);
			pBuf[(*iSendIndex)++] = 0x03 ;
			pBuf[(*iSendIndex)++] = ucBoardIndex ;
			ACE_OS::sleep(tv);
			for(int ucIndex = 0 ;ucIndex < 4 ;ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_ucRoadSpeed[ucBoardIndex][ucIndex];
			break ;
		case 0x04 :
			pDetctor->GetDecVars(ucBoardIndex,0x04);
			pBuf[(*iSendIndex)++] = 0x04 ;
			pBuf[(*iSendIndex)++] = ucBoardIndex ;
			ACE_OS::sleep(tv);
			for(int ucIndex = 4 ;ucIndex < 8 ;ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_ucRoadSpeed[ucBoardIndex][ucIndex];
			break ;
		case 0x05 :
			pDetctor->GetDecVars(ucBoardIndex,0x05);
			pBuf[(*iSendIndex)++] = 0x05 ;
			pBuf[(*iSendIndex)++] = ucBoardIndex ;
			ACE_OS::sleep(tv);
			for(int ucIndex = 0 ; ucIndex <8 ; ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_ucGetDetDelicacy[ucBoardIndex][ucIndex];
			break ;
		case 0x06 :
			pDetctor->GetDecVars(ucBoardIndex,0x06);
			pBuf[(*iSendIndex)++] = 0x06 ;
			pBuf[(*iSendIndex)++] = ucBoardIndex ;
			ACE_OS::sleep(tv);
			for(int ucIndex = 8 ; ucIndex <16 ; ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_ucGetDetDelicacy[ucBoardIndex][ucIndex];
			break ;
		case 0x07 :
			pDetctor->GetDecVars(ucBoardIndex,0x07);
			pBuf[(*iSendIndex)++] = 0x07 ;
			pBuf[(*iSendIndex)++] = ucBoardIndex ;
			ACE_OS::sleep(tv);
			for(int ucIndex = 0 ;ucIndex < 4 ;ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_sGetLookLink[ucBoardIndex][ucIndex];
			break ;
		case 0x08 :
			pDetctor->GetDecVars(ucBoardIndex,0x08);
			pBuf[(*iSendIndex)++] = 0x08 ;
			pBuf[(*iSendIndex)++] = ucBoardIndex ;
			ACE_OS::sleep(tv);
			for(int ucIndex = 4 ;ucIndex < 8 ;ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_sGetLookLink[ucBoardIndex][ucIndex];
			break ;
		case 0x09 :
			pDetctor->GetDecVars(ucBoardIndex,0x09);
			pBuf[(*iSendIndex)++] = 0x09 ;
			pBuf[(*iSendIndex)++] = ucBoardIndex ;
			ACE_OS::sleep(tv);
			for(int ucIndex = 0 ;ucIndex < 4 ;ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_ucGetRoadDis[ucBoardIndex][ucIndex];
			
			break ;
		case 0x0a :
			pDetctor->GetDecVars(ucBoardIndex,0x0a);
			pBuf[(*iSendIndex)++] = 0x0a ;
			pBuf[(*iSendIndex)++] = ucBoardIndex ;
			ACE_OS::sleep(tv);
			for(int ucIndex = 4 ;ucIndex < 8 ;ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_ucGetRoadDis[ucBoardIndex][ucIndex];
			break ;
		case 0x11 :
			pDetctor->GetVehSts(ucBoardIndex,DET_HEAD_STS);
			pBuf[(*iSendIndex)++] = 0x11 ;
			pBuf[(*iSendIndex)++] = ucBoardIndex ;
			ACE_OS::sleep(tv);
			for(Byte ucIndex = 0 ;ucIndex < MAX_DETECTOR_PER_BOARD;ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_ucDetError[ucBoardIndex*MAX_DETECTOR_PER_BOARD+ucIndex];
			
			break ;			
		case 0x12 :
			pDetctor->GetDecVars(ucBoardIndex,0x12);
			pBuf[(*iSendIndex)++] = 0x12 ;
			pBuf[(*iSendIndex)++] = ucBoardIndex ;
			ACE_OS::sleep(tv);
			for(int ucIndex = 0 ;ucIndex < 7 ;ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_ucGetSensibility[ucBoardIndex][ucIndex];
			break ;
		case 0x13 :
			pDetctor->GetDecVars(ucBoardIndex,0x13);
			pBuf[(*iSendIndex)++] = 0x13 ;
			pBuf[(*iSendIndex)++] = ucBoardIndex ;
			ACE_OS::sleep(tv);
			for(int ucIndex = 7 ;ucIndex < 14 ;ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_ucGetSensibility[ucBoardIndex][ucIndex];
			
			break ;
		case 0x14 :
			pDetctor->GetDecVars(ucBoardIndex,0x14);
			pBuf[(*iSendIndex)++] = 0x14 ;
			pBuf[(*iSendIndex)++] = ucBoardIndex ;
			ACE_OS::sleep(tv);
			for(int ucIndex = 14 ;ucIndex < 16 ;ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_ucGetSensibility[ucBoardIndex][ucIndex];
			break ;
		case 0x18 :
			pDetctor->GetDecVars(ucBoardIndex,0x18);
			pBuf[(*iSendIndex)++] = 0x18 ;
			pBuf[(*iSendIndex)++] = ucBoardIndex ;
			ACE_OS::sleep(tv);
			for(int ucIndex = 0 ;ucIndex < 16 ;ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_ucGetFrency[ucBoardIndex][ucIndex];
			break ;
		case 0x1a :
			pDetctor->GetDecVars(ucBoardIndex,0x1a);
			pBuf[(*iSendIndex)++] = 0x1a ;
			pBuf[(*iSendIndex)++] = ucBoardIndex ;
			ACE_OS::sleep(tv);			
			pBuf[(*iSendIndex)++] = pDetctor->m_iChkType;
			break ;
		default :
			return ;
	}
}


/**************************************************************
Function:       CGbtMsgQueue::SetDetCfg
Description:    设置检测器配置信息	
Input:         	pBuf     接收帧地址指针
				iRecvIndex  接收帧当前读取地址
Output:        	无
Return:         无
***************************************************************/
void CGbtMsgQueue::SetDetCfg(Byte* pBuf,int& iRecvIndex)
{
	Byte ucSetType = pBuf[iRecvIndex++];
	Byte ucBdIndex = pBuf[iRecvIndex++];
	CDetector* pDetctor = CDetector::CreateInstance();
	
	switch(ucSetType)
	{
		case 0x0b :
			for(int ucIndex = 0 ;ucIndex <4 ; ucIndex++)
			{
				pDetctor->m_ucSetDetDelicacy[ucBdIndex][2*ucIndex] = pBuf[iRecvIndex] &0x0f;
				pDetctor->m_ucSetDetDelicacy[ucBdIndex][2*ucIndex+1] = (pBuf[iRecvIndex]>>4) &0x0f;
				iRecvIndex++;
			}	
			pDetctor->SendDelicacy(ucBdIndex, 0x0b);
			break ;
		case 0x0c :
			for(int ucIndex = 4 ;ucIndex <8 ; ucIndex++)
			{
				pDetctor->m_ucSetDetDelicacy[ucBdIndex][2*ucIndex] = pBuf[iRecvIndex] &0x0f;
				pDetctor->m_ucSetDetDelicacy[ucBdIndex][2*ucIndex+1] = (pBuf[iRecvIndex]>>4) &0x0f;
				iRecvIndex++;
			}	
			pDetctor->SendDelicacy(ucBdIndex, 0x0c);
			break ;
		case 0x0d :
			for(int ucIndex = 0 ;ucIndex <4 ; ucIndex++)
			{
				pDetctor->m_sSetLookLink[ucBdIndex][ucIndex] = pBuf[iRecvIndex++];		
				
			}	
			pDetctor->SendDecIsLink(ucBdIndex,0);				
			pDetctor->SendDetLink(ucBdIndex,DET_HEAD_COIL0104_SET);
					
			break ;
		case 0x0e :
			for(int ucIndex = 4 ;ucIndex <8 ; ucIndex++)
			{
				pDetctor->m_sSetLookLink[ucBdIndex][ucIndex] = pBuf[iRecvIndex++];
				
			}	
			pDetctor->SendDecIsLink(ucBdIndex,0);				
			pDetctor->SendDetLink(ucBdIndex,DET_HEAD_COIL0508_SET);
			break ;
		case 0x0f :
			for(int ucIndex = 0 ;ucIndex <4 ; ucIndex++)
			{
				pDetctor->m_ucSetRoadDis[ucBdIndex][ucIndex] = pBuf[iRecvIndex++];		
				
			}	
			pDetctor->SendRoadDistance(ucBdIndex,DET_HEAD_DISTAN0104_SET);
					
			break ;
		case 0x10 :
			for(int ucIndex = 4 ;ucIndex <8 ; ucIndex++)
			{
				pDetctor->m_ucSetRoadDis[ucBdIndex][ucIndex] = pBuf[iRecvIndex++];
				
			}	
			pDetctor->SendRoadDistance(ucBdIndex,DET_HEAD_DISTAN0508_SET);
			break ;
		case 0x19 :
			for(int ucIndex = 0 ;ucIndex < 4 ; ucIndex++)
			{
				pDetctor->m_ucSetFrency[ucBdIndex][4*ucIndex] = pBuf[iRecvIndex] & 0x03;
				pDetctor->m_ucSetFrency[ucBdIndex][4*ucIndex+1] = (pBuf[iRecvIndex]>>2) & 0x03;
				pDetctor->m_ucSetFrency[ucBdIndex][4*ucIndex+2] = (pBuf[iRecvIndex] >>4)& 0x03;
				pDetctor->m_ucSetFrency[ucBdIndex][4*ucIndex+3] = (pBuf[iRecvIndex]>>6) & 0x03;
				iRecvIndex++;
			}	
			pDetctor->SendDecFrency(ucBdIndex);
			break ;
		case 0x1a :
			pDetctor->m_iChkType = pBuf[iRecvIndex++] ;
			pDetctor->SendDecWorkType(ucBdIndex);
			//ACE_DEBUG((LM_DEBUG," pDetctor->m_iChkType =  %d !\n",pDetctor->m_iChkType));
			break ;
		case 0x15 :			
					
			for(int ucIndex = 0 ;ucIndex < 7 ;ucIndex++)
			   pDetctor->m_ucSetSensibility[ucBdIndex][ucIndex]=pBuf[iRecvIndex++] ;
							
			pDetctor->SendDecSenData(ucBdIndex,DET_HEAD_SENDATA0107_SET);
			break ;
		case 0x16 :
			
			for(int ucIndex = 7 ;ucIndex < 14 ;ucIndex++)
				pDetctor->m_ucSetSensibility[ucBdIndex][ucIndex]=pBuf[iRecvIndex++];
			pDetctor->SendDecSenData(ucBdIndex,DET_HEAD_SENDATA0814_SET);
			break ;
		case 0x17 :
			pDetctor->GetDecVars(ucBdIndex,0x14);
			for(int ucIndex = 14 ;ucIndex < 16 ;ucIndex++)
				pDetctor->m_ucSetSensibility[ucBdIndex][ucIndex]=pBuf[iRecvIndex++];
			pDetctor->SendDecSenData(ucBdIndex,DET_HEAD_SENDATA1516_SET);
			break ;
					
		default :
			return ;


	}

	printf("\n%s:%d ucSetType= %d ucBdIndex = %d \n",__FILE__,__LINE__,ucSetType,ucBdIndex);

}


/**************************************************************
Function:       CGbtMsgQueue::PackTscStatus
Description:    信号机状态信息打包	
Input:         	pValue  信息机状态结构体指针
Output:        	无
Return:         无
***************************************************************/
void CGbtMsgQueue::PackTscStatus(Byte ucDealDataIndex,void* pValue)
{
	//Byte ucRecvOptType = ( m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[0]) & 0xf;    //收到帧的操作类型
	int iRecvIndex     = m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex;          
	int iRecvBufLen    = m_sGbtDealData[ucDealDataIndex].sRecvFrame.iBufLen;
	int iSendIndex     = m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex;          
	//int iSendBufLen    = m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen;
	Byte ucIndexCnt    = 0;  //索引个数
	Byte ucErrorSts    = 0;  //错误状态
	Byte ucErrorIdx    = 0;  //错误索引
	Byte ucObjId       = 0;  //对象名(表名)
	Byte ucIdxFst      = 0;  //第一个索引(id1)
	Byte ucIdxSnd      = 0;  //第二个索引(id2)
	Byte ucSubId       = 0;  //子对象(字段下标)
	Byte ucIndex       = 0;
	Byte ucRecordCnt   = 0;
	STscStatus* pTscStatus = (STscStatus*)pValue;

	if ( iRecvIndex >= iRecvBufLen )
	{
		ucErrorSts = GBT_ERROR_OTHER;
		GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
		return;
	}

	/************对象标识**************/
	ucObjId = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
	m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucObjId;   //对象标识
	iRecvIndex++;
	iSendIndex++;

	/***********索引个数与子对象******/
	if ( iRecvIndex >= iRecvBufLen )
	{
		ucErrorSts = GBT_ERROR_OTHER;
		GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
		return;
	}
	m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] =
		m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex]; //索引个数与子对象
	ucIndexCnt = (m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex]>>6) & 0x3;  //索引个数
	ucSubId    = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex] & 0x3F;      //子对象，字段下标
	iRecvIndex++;
	iSendIndex++;

	/***********索引*************/
	if ( ucIndexCnt > 0 )  /*索引1*/
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		ucIdxFst = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucIdxFst;
		iRecvIndex++;
		iSendIndex++;
		ucIndexCnt--;
	}
	if ( ucIndexCnt > 0 ) /*索引2*/
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		ucIdxSnd = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucIdxSnd;
		iRecvIndex++;
		iSendIndex++;
		ucIndexCnt--;
	}
	if ( ucIndexCnt > 0 )  /*索引3忽略*/
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		iRecvIndex++;
		iSendIndex++;
		ucIndexCnt--;
	}

	/*************值域************/
	switch ( ucObjId )
	{
	case OBJECT_CURTSC_CTRL:          /*当前信号机的控制状态*/
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = ToObjectCurTscCtrl(pTscStatus->uiCtrl);
		break;
	/*
	case OBJECT_SWITCH_CONTROL:
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
			ToObjectControlSwitch(pTscStatus->uiWorkStatus,pTscStatus->uiCtrl);
		break;
	
	case OBJECT_SWITCH_STAGE:        //阶段状态
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = pTscStatus->ucStageNo;
		break;
	*/
	case OBJECT_ACTIVESCHEDULE_NO: /*当前活动时段编号*/
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = pTscStatus->ucActiveSchNo;
		break;
	case OBJECT_TSC_WARN2:  /*信号机报警2*/
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = pTscStatus->ucTscAlarm2;
		break;
	case OBJECT_TSC_WARN1:  /*信号机报警1*/
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = pTscStatus->ucTscAlarm1;
		break;
	case OBJECT_TSC_WARN_SUMMARY:   /*信号机报警摘要*/
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = pTscStatus->ucTscAlarmSummary;
		break;
	case OBJECT_ACTIVEDETECTOR_NUM:   /*活动检测器总数*/
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = pTscStatus->ucActiveDetCnt;
		break;
	case OBJECT_SWITCH_MANUALCONTROL:  /*手动控制方案*/
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] 
			= GetManualCtrlStatus(pTscStatus->uiWorkStatus,pTscStatus->uiCtrl);
		break;
	case OBJECT_SWITCH_SYSTEMCONTROL:	/*系统控制方案*/
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] 
			= GetSysCtrlStatus(pTscStatus->uiWorkStatus,pTscStatus->uiCtrl);
		break;
	case OBJECT_SWITCH_CONTROL: 	 /*控制方式*/
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] 
			= GetCtrlStatus(pTscStatus->uiWorkStatus,pTscStatus->uiCtrl);
		break;
	case OBJECT_SWITCH_STAGE: 	 /*阶段状态*/
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = pTscStatus->ucStageNo;
		break;
	case OBJECT_GOSTEP: 	 /*步进指令*/
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = pTscStatus->ucStepNo;
		break;
	case OBJECT_CURPATTERN_SCHTIMES:   /*当前方案各阶段时长*/
		ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex,
			                  pTscStatus->ucCurStageLen,16);
	    iSendIndex += 16;
		break;
	case OBJECT_CURPATTERN_GREENTIMES: /*当前方案各关键相位绿灯时长*/
		ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex,
			pTscStatus->ucCurKeyGreen,16);
		iSendIndex += 16;
		break;
	case OBJECT_DETECTORSTS_TABLE: 	 /*检测器状态*/
		if ( (0==ucIdxFst) && (0==ucIdxSnd) && (0==ucSubId) )  //整张表
		{
			ucIndex     = 0;
			ucRecordCnt = 8;
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = ucRecordCnt;
			while ( ucIndex < ucRecordCnt )
			{
				/*
				ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex
					            ,(pTscStatus->sDetSts)+ucIndex,sizeof(SDetectorStsPara));
				iSendIndex += sizeof(SDetectorStsPara);
				*/
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] 
												= ((pTscStatus->sDetSts)+ucIndex)->ucId;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] 
												= ((pTscStatus->sDetSts)+ucIndex)->ucStatus;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] 
												= ((pTscStatus->sDetSts)+ucIndex)->ucAlarm;
											
				//ACE_DEBUG((LM_DEBUG,"ID:%d\n",((pTscStatus->sDetSts)+ucIndex)->ucId));

				ucIndex++;
			}
		}
		else if ( (ucIdxFst>0) && (ucIdxFst<9) )
		{
			switch ( ucSubId )
			{
			case 0:
				ucIndex = ucIdxFst-1;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 1;
				/*
				ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex,
					(pTscStatus->sDetSts)+ucIdxFst-1,sizeof(SDetectorStsPara));
				iSendIndex += sizeof(SDetectorStsPara);
				*/
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] 
												= ((pTscStatus->sDetSts)+ucIndex)->ucId;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] 
												= ((pTscStatus->sDetSts)+ucIndex)->ucStatus;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] 
												= ((pTscStatus->sDetSts)+ucIndex)->ucAlarm;
				break;
			case 1:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetSts)+ucIdxFst-1)->ucId;
				break;
			case 2:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetSts)+ucIdxFst-1)->ucStatus;
				break;
			case 3:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetSts)+ucIdxFst-1)->ucAlarm;
				break;
			default:
				GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
				return;
			}
		}
		else
		{
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		break;
	case OBJECT_DETECTORDATA_TABLE: 	/*交通检测数据表*/
		if ( (0==ucIdxFst) && (0==ucIdxSnd) && (0==ucSubId) )  	//整张表
		{
			ucIndex = 0;
			ucRecordCnt = 48;
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = ucRecordCnt;

			while ( ucIndex < ucRecordCnt )
			{
				/*
				ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex
								,(pTscStatus->sDetData)+ucIndex,sizeof(SDetectorDataPara));
				iSendIndex += sizeof(SDetectorDataPara);
				*/
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucId;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucVolume;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucLongVolume;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucSmallVolume;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucOccupancy;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucVelocity;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucVehLen;

				ucIndex++;
			}
		}
		else if ( (ucIdxFst>0) && (ucIdxFst<49) )
		{
			switch ( ucSubId )
			{
			case 0:
				ucIndex = ucIdxFst - 1;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 1;
				/*
				ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex
					,(pTscStatus->sDetData)+ucIdxFst-1,sizeof(SDetectorDataPara));
				iSendIndex += sizeof(SDetectorDataPara);
				*/
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucId;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucVolume;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucLongVolume;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucSmallVolume;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucOccupancy;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucVelocity;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucVehLen;
				break;
			case 1:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetData)+ucIdxFst-1)->ucId;
				break;
			case 2:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetData)+ucIdxFst-1)->ucVolume;
				break;
			case 3:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetData)+ucIdxFst-1)->ucLongVolume;
				break;
			case 4:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetData)+ucIdxFst-1)->ucSmallVolume;
				break;
			case 5:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetData)+ucIdxFst-1)->ucOccupancy;
				break;
			case 6:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetData)+ucIdxFst-1)->ucVelocity;
				break;
			case 7:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetData)+ucIdxFst-1)->ucVehLen;
				break;
			default:
				GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
				return;
			}
		}
		else
		{
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		break;
	case OBJECT_DETECTORWARN_TABLE: /*车辆检测器告警*/
		if ( (0==ucIdxFst) && (0==ucIdxSnd) && (0==ucSubId) )   //整张表
		{
			ucIndex = 0;
			ucRecordCnt = 48;
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = ucRecordCnt;
			
			while ( ucIndex < ucRecordCnt )
			{
				/*
				ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex
					,(pTscStatus->sDetAlarm)+ucIndex,sizeof(SDetectorAlarm));
				iSendIndex += sizeof(SDetectorAlarm);
				*/
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sDetAlarm)+ucIndex)->ucId;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sDetAlarm)+ucIndex)->ucDetAlarm;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sDetAlarm)+ucIndex)->ucCoilAlarm;
				ucIndex++;
			}
		}
		else if ( (ucIdxFst>0) && (ucIdxFst<49) )
		{
			switch ( ucSubId )
			{
			case 0:
				ucIndex = ucIdxFst - 1;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 1;
				/*
				ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex
					,(pTscStatus->sDetAlarm)+ucIdxFst-1,sizeof(SDetectorAlarm));
				iSendIndex += sizeof(SDetectorAlarm);
				*/
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sDetAlarm)+ucIndex)->ucId;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sDetAlarm)+ucIndex)->ucDetAlarm;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sDetAlarm)+ucIndex)->ucCoilAlarm;

				break;
			case 1:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetAlarm)+ucIdxFst-1)->ucId;
				break;
			case 2:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetAlarm)+ucIdxFst-1)->ucDetAlarm;
				break;
			case 3:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetAlarm)+ucIdxFst-1)->ucCoilAlarm;
				break;
			default:
				GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
				return;
			}
		}
		else
		{
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		break;
		break;
	case OBJECT_PHASESTATUS_TABLE:    /*相位状态*/
		if ( (0==ucIdxFst) && (0==ucIdxSnd) && (0==ucSubId) )  //整张表
		{
			ucIndex = 0;
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 2;
			while ( ucIndex < 2 )
			{
				/*
				ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex
					,(pTscStatus->sPhaseSts)+ucIndex,sizeof(SPhaseStsPara));
				iSendIndex += sizeof(SPhaseStsPara);
				*/
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sPhaseSts)+ucIndex)->ucId;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sPhaseSts)+ucIndex)->ucRed;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sPhaseSts)+ucIndex)->ucYellow;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sPhaseSts)+ucIndex)->ucGreen;

				ucIndex++;
			}
		}
		else if ( (1 == ucIdxFst) || (2 == ucIdxFst) )
		{
			switch ( ucSubId )
			{
				case 0:
					ucIndex = ucIdxFst - 1;
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 1;
					/*
					ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex
						,(pTscStatus->sPhaseSts)+ucIdxFst-1,sizeof(SPhaseStsPara));
					iSendIndex += sizeof(SPhaseStsPara);
					*/
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sPhaseSts)+ucIndex)->ucId;
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sPhaseSts)+ucIndex)->ucRed;
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sPhaseSts)+ucIndex)->ucYellow;
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sPhaseSts)+ucIndex)->ucGreen;
					break;
				case 1:
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
						((pTscStatus->sPhaseSts)+ucIdxFst-1)->ucId;
					break;
				case 2:
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
						((pTscStatus->sPhaseSts)+ucIdxFst-1)->ucRed;
					break;
				case 3:
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
						((pTscStatus->sPhaseSts)+ucIdxFst-1)->ucYellow;
					break;
				case 4:
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
						((pTscStatus->sPhaseSts)+ucIdxFst-1)->ucGreen;
					break;
				default:
					GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
					return;
			}
		}
		else
		{
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		break;
	case OBJECT_OVERLAPPHASE_STATUS: /*跟随相位状态*/
		if ( (0==ucIdxFst) && (0==ucIdxSnd) && (0==ucSubId) )  //整张表
		{
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 1;
			
			/*
			ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex
				,&(pTscStatus->sOverlapPhaseSts),sizeof(SOverlapPhaseStsPara));
			iSendIndex += sizeof(SOverlapPhaseStsPara);
			*/

			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= (pTscStatus->sOverlapPhaseSts).ucId;
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= (pTscStatus->sOverlapPhaseSts).ucRed;
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= (pTscStatus->sOverlapPhaseSts).ucYellow;
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= (pTscStatus->sOverlapPhaseSts).ucGreen;
		}
		else if ( 1 == ucIdxFst )
		{
			switch ( ucSubId )
			{
			case 0:
				/*
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 1;
				ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex
					,&(pTscStatus->sOverlapPhaseSts),sizeof(SOverlapPhaseStsPara));
				iSendIndex += sizeof(SOverlapPhaseStsPara);
				*/
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= (pTscStatus->sOverlapPhaseSts).ucId;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= (pTscStatus->sOverlapPhaseSts).ucRed;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= (pTscStatus->sOverlapPhaseSts).ucYellow;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= (pTscStatus->sOverlapPhaseSts).ucGreen;
				break;
			case 1:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] =
					(pTscStatus->sOverlapPhaseSts).ucId;
				break;
			case 2:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] =
					(pTscStatus->sOverlapPhaseSts).ucRed;
				break;
			case 3:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					(pTscStatus->sOverlapPhaseSts).ucYellow;
				break;
			case 4:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					(pTscStatus->sOverlapPhaseSts).ucGreen;
				break;
			default:
				GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
				return;
			}
		}
		else
		{
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}

		break;
	case OBJECT_CHANNELSTATUS_TABLE:       /*通道状态*/
		if ( (0==ucIdxFst) && (0==ucIdxSnd) && (0==ucSubId) )  //整张表
		{
			ucIndex = 0;
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 2;

			while ( ucIndex < 2 )
			{
				/*
				ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex
					,pTscStatus->sChannelSts,sizeof(SChannelStsPara));
				iSendIndex += sizeof(SChannelStsPara);
				*/
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sChannelSts)+ucIndex)->ucId;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sChannelSts)+ucIndex)->ucRed;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sChannelSts)+ucIndex)->ucYellow;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sChannelSts)+ucIndex)->ucGreen;
				ucIndex++;
			}
		}
		else if ( 1 == ucIdxFst || 2 == ucIdxFst )
		{
			switch ( ucSubId )
			{
				case 0:
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 1;
					/*
					ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex
						,(pTscStatus->sChannelSts)+ucIdxFst-1,sizeof(SChannelStsPara));
					iSendIndex += sizeof(SChannelStsPara);
					*/
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sChannelSts)+ucIndex)->ucId;
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sChannelSts)+ucIndex)->ucRed;
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sChannelSts)+ucIndex)->ucYellow;
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sChannelSts)+ucIndex)->ucGreen;
				break;
				case 1:
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
						((pTscStatus->sChannelSts)+ucIdxFst-1)->ucId;
					break;
				case 2:
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
						((pTscStatus->sChannelSts)+ucIdxFst-1)->ucRed;
					break;
				case 3:
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] =
						((pTscStatus->sChannelSts)+ucIdxFst-1)->ucYellow;
					break;
				case 4:
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] =
						((pTscStatus->sChannelSts)+ucIdxFst-1)->ucGreen;
					break;
				default:
					GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
					return;
			}
		}
		else
		{
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		break;
	case OBJECT_CURTSC_FLASHCTRL:  /*当前闪光控制引起的原因*/
		if ( pTscStatus->uiWorkStatus != FLASH )   //非黄闪
		{
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 2;
		}
		else if ( CTRL_MANUAL == pTscStatus->uiCtrl )  //手动黄闪
		{
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 4;
		}
		else if ( true == pTscStatus->bStartFlash ) //启动时黄闪
		{
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 7;
		}
		else
		{
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 3;
		}
		break;
	default:
#ifdef TSC_DEBUG
		ACE_DEBUG((LM_DEBUG,"%s:%d,objectId error objectid:%d\n",__FILE__,__LINE__,ucObjId));
#endif
		GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
		return;
	}

	m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex = iRecvIndex;
	m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex = iSendIndex;
	m_sGbtDealData[ucDealDataIndex].iObjNum--;
	if ( iRecvIndex == iRecvBufLen )   //处理完毕
	{
		if ( 0 == m_sGbtDealData[ucDealDataIndex].iObjNum ) //对象数也刚好处理完毕
		{
			m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = iSendIndex;
			GotoSendToHost(ucDealDataIndex);
			return;
		}
		else
		{
#ifdef TSC_DEBUG
			ACE_DEBUG((LM_DEBUG,"%s:%d,ObjNum error ObjNum:%d\n",
				               __FILE__,__LINE__,m_sGbtDealData[ucDealDataIndex].iObjNum));
#endif
			ucErrorSts = 5;
			ucErrorIdx = 0;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
	}
	else
	{
		GotoDealRecvbuf(ucDealDataIndex);
		return;
	}
}


/**************************************************************
Function:       CGbtMsgQueue::PackTscExStatus
Description:    信号机扩展状态信息打包	
Input:         	ucDealDataIndex  待处理信息下标
				pValue  信息机扩展状态结构体指针
Output:        	无
Return:         无
***************************************************************/
void CGbtMsgQueue::PackTscExStatus(Byte ucDealDataIndex,void* pValue)
{
	//Byte ucRecvOptType = ( m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[0]) & 0xf;   //收到帧的操作类型
	int iRecvIndex     = m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex;          
	int iRecvBufLen    = m_sGbtDealData[ucDealDataIndex].sRecvFrame.iBufLen;
	int iSendIndex     = m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex;          
	//int iSendBufLen    = m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen;
	Byte ucIndexCnt    = 0;  //索引个数
	Byte ucErrorSts    = 0;  //错误状态
	Byte ucErrorIdx    = 0;  //错误索引
	Byte ucObjId       = 0;  //对象名(表名)
	Byte ucIdxFst      = 0;  //第一个索引(id1)
	Byte ucIdxSnd      = 0;  //第二个索引(id2)
	Byte ucSubId       = 0; //子对象(字段下标)

	if ( iRecvIndex >= iRecvBufLen )
	{
		ucErrorSts = GBT_ERROR_OTHER;
		GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
		return;
	}

	/*************对象标识*********/
	ucObjId = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
	m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucObjId;   //对象标识
	iRecvIndex++;
	iSendIndex++;

	/***********索引个数与子对象********/
	if ( iRecvIndex >= iRecvBufLen )
	{
		ucErrorSts = GBT_ERROR_OTHER;
		GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
		return;
	}
	m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] =
		m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex]; //索引个数与子对象
	ucIndexCnt = (m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex]>>6) & 0x3;  //索引个数
	ucSubId    = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex] & 0x3F;      //子对象，字段下标
	iRecvIndex++;
	iSendIndex++;

	/***********索引*************/
	if ( ucIndexCnt > 0 )  /*索引1*/
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		ucIdxFst = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucIdxFst;
		iRecvIndex++;
		iSendIndex++;
		ucIndexCnt--;
	}
	if ( ucIndexCnt > 0 ) /*索引2*/
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		ucIdxSnd = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucIdxSnd;
		iRecvIndex++;
		iSendIndex++;
		ucIndexCnt--;
	}
	if ( ucIndexCnt > 0 )  /*索引3忽略*/
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		iRecvIndex++;
		iSendIndex++;
		ucIndexCnt--;
	}

	/**********值域************/
	switch ( ucObjId )
	{
	case OBJECT_EXT_TSC_STATUS:  /*当前信号机的控制状态*/
		ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex,(Byte*)pValue,25);
		iSendIndex += 25;
		break;
	default:
		GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
		return;
	}

	m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex = iRecvIndex;
	m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex = iSendIndex;
	m_sGbtDealData[ucDealDataIndex].iObjNum--;
	if ( iRecvIndex == iRecvBufLen )  //处理完毕
	{
		if ( 0 == m_sGbtDealData[ucDealDataIndex].iObjNum )  //对象数也刚好处理完毕
		{
			m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = iSendIndex;
			GotoSendToHost(ucDealDataIndex);
			return;
		}
		else
		{
			ucErrorSts = 5;
			ucErrorIdx = 0;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
	}
	else
	{
		GotoDealRecvbuf(ucDealDataIndex);
		return;
	}
}


/**************************************************************
Function:       CGbtMsgQueue::GetCtrlStatus
Description:    获取信号机当前控制状态
Input:         	uiWorkStatus  工作状态
				uiCtrl  	  控制方式
Output:        	无
Return:         控制方式数字值
***************************************************************/
int CGbtMsgQueue::GetCtrlStatus(unsigned int uiWorkStatus,unsigned int uiCtrl)
{
	if ( SIGNALOFF == uiWorkStatus )
	{
		return 1;
	}
	else if ( FLASH == uiWorkStatus )
	{
		return 2;
	}
	else if ( ALLRED == uiWorkStatus )
	{
		return 3;
	}
	else
	{
		switch ( uiCtrl )
		{
		case CTRL_VEHACTUATED:
		case CTRL_MAIN_PRIORITY:
		case CTRL_SECOND_PRIORITY:
			return 6;
		case CTRL_ACTIVATE:
			return 8;
		case CTRL_HOST:
			return 11;
		case CTRL_UTCS:
			return 12;
		case CTRL_WIRELESS:
		case CTRL_LINE:
			return 13;
		default:
			return 5;
		}
	}
}


/**************************************************************
Function:       CGbtMsgQueue::GetSysCtrlStatus
Description:    获取信号机系统控制状态
Input:         	uiWorkStatus  工作状态
				uiCtrl  	  控制方式
Output:        	无
Return:         系统控制状态数字值
***************************************************************/
int CGbtMsgQueue::GetSysCtrlStatus(unsigned int uiWorkStatus,unsigned int uiCtrl)
{
	if ( CTRL_MANUAL != uiCtrl )
	{
		return m_iSystemCtrlNo;
	}
	else 
	{
		switch ( uiWorkStatus )
		{
		case ALLRED:
			return 253;
		case FLASH:
			return 254;
		case SIGNALOFF:
			return 255;
		default:
			return m_iSystemCtrlNo;
		}
	}
}


/**************************************************************
Function:       CGbtMsgQueue::GetManualCtrlStatus
Description:    获取信号机手动控制状态
Input:         	uiWorkStatus  工作状态
				uiCtrl  	  控制方式
Output:        	无
Return:         手动控制状态数字值
***************************************************************/
int CGbtMsgQueue::GetManualCtrlStatus(unsigned int uiWorkStatus,unsigned int uiCtrl)
{
	if ( CTRL_MANUAL != uiCtrl )
	{
		return 0;
	}
	else 
	{
		switch ( uiWorkStatus )
		{
		case ALLRED:
			return 253;
		case FLASH:
			return 254;
		case SIGNALOFF:
			return 255;
		default:
			return m_iManualCtrlNo;
		}
	}
}


/**************************************************************
Function:       CGbtMsgQueue::DealRecvBuf
Description:    对接收到udp数据进行处理
Input:         	ucDealDataIndex  接收信息处理队列下标
Output:        	无
Return:         无
***************************************************************/
void CGbtMsgQueue::DealRecvBuf(Byte ucDealDataIndex) 
{
	Byte ucRecvOptType = ( m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[0] ) & 0xf;  //收到帧的操作类型
	Byte ucSendOptType = 0;                                                              //发送帧的操作类型
	int iRecvIndex     = m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex;          
	int iRecvBufLen    = m_sGbtDealData[ucDealDataIndex].sRecvFrame.iBufLen;
	int iSendIndex     = m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex;          
	int iSendBufLen    = m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen;
	Byte ucIndexCnt    = 0;    //索引个数
	Byte ucErrorSts    = 0;    //错误状态
	Byte ucErrorIdx    = 0;    //错误索引
	Byte ucObjId       = 0;    //对象名(表名)
	Byte ucIdxFst      = 255;  //第一个索引(id1)
	Byte ucIdxSnd      = 255;  //第二个索引(id2)
	Byte ucSubId       = 0;    //子对象(字段下标)
	int  iFunRet       = -1;   //函数返回值

	/**********首次处理该帧************/
	if ( 0 == iRecvIndex ) 
	{
		ucSendOptType = GetSendOperateType(ucRecvOptType);   
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[0] = 
			(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[0] & 0xf0) | ucSendOptType; //头字节
		m_sGbtDealData[ucDealDataIndex].iObjNum = 
			 ( (m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[0]>>4) & 7 ) + 1;  //对象个数

		iRecvIndex = 1;
		iSendIndex = 1;
	}
	
	while ( 1 )
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
			ucErrorSts = GBT_ERROR_OTHER;
#ifdef TSC_DEBUG
			ACE_DEBUG((LM_DEBUG,"%s:%d,GBT_ERROR_OTHER\n",__FILE__,__LINE__));
#endif
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		
		/************对象标识*************/
		ucObjId = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucObjId;   //对象标识

		if ( ( IsSendTscCommand(ucObjId) && (GBT_SEEK_REQ == ucRecvOptType) ) 
			|| IsGetTscStatusObject(ucObjId) ) //获取信号机状态对象
		{
			if ( GBT_SEEK_REQ != ucRecvOptType )
			{
				ucErrorSts = GBT_ERROR_OTHER;
#ifdef TSC_DEBUG
				ACE_DEBUG((LM_DEBUG,"%s:%d,GetTscStatusObject && !GBT_SEEK_REQ\n",__FILE__,__LINE__));
#endif
				GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
				return;
			}
			m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex  = iRecvIndex; 
			m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex  = iSendIndex;          
			m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = iSendBufLen;

			SThreadMsg sTscMsg;
			sTscMsg.ulType       = TSC_MSG_STATUS_READ;
			sTscMsg.ucMsgOpt     = ucDealDataIndex;
			sTscMsg.uiMsgDataLen = 0;
			sTscMsg.pDataBuf     = NULL;

			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(SThreadMsg));
			return;
		}
		else if ( ucObjId == OBJECT_SET_REPORTSELF ) //主动上报
		{
#ifdef GBT_TCP
			CGbtTimer::CreateInstance()->TailorReport(ucDealDataIndex 
				                                 , iRecvBufLen 
												 , m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf);
#else
			CGbtTimer::CreateInstance()->TailorReport(m_sGbtDealData[ucDealDataIndex].AddrClient
				                                   ,iRecvBufLen
												   ,m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf);
#endif
			CleanDealData(ucDealDataIndex);
			return;
		}
		else if(ucObjId == OBJECT_SENDCLIENT_CNTDOWN)
		{
			CGaCountDown::CreateInstance()->SetClinetCntDown(m_sGbtDealData[ucDealDataIndex].AddrClient
				                                  ,iRecvBufLen ,m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf);
			CleanDealData(ucDealDataIndex);
			return ;
		}
		else if ( IsOtherObject(ucObjId) )  	 //其他类型的对象
		{
			m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex  = iRecvIndex; 
			m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex  = iSendIndex;          
			m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = iSendBufLen;

			SThreadMsg sTscMsg;
			sTscMsg.ulType       = GBT_MSG_OTHER_OBJECT;
			sTscMsg.ucMsgOpt     = ucDealDataIndex;
			sTscMsg.uiMsgDataLen = 0;
			sTscMsg.pDataBuf     = NULL;

			SendGbtMsg(&sTscMsg,sizeof(SThreadMsg));
			return;
		}
		else if ( IsExtendObject(ucObjId) )  //扩展类型对象
		{
			m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex  = iRecvIndex; 
			m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex  = iSendIndex;          
			m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = iSendBufLen;

			SThreadMsg sTscMsg;
			sTscMsg.ulType       = GBT_MSG_EXTEND;
			sTscMsg.ucMsgOpt     = ucDealDataIndex;
			sTscMsg.uiMsgDataLen = 0;
			sTscMsg.pDataBuf     = NULL;

			SendGbtMsg(&sTscMsg,sizeof(SThreadMsg));
			return;
		}
		
		iRecvIndex++;
		iSendIndex++;

		/***********索引个数与子对象*******/
		if ( iRecvIndex >= iRecvBufLen )
		{
			ucErrorSts = GBT_ERROR_OTHER;
#ifdef TSC_DEBUG
			ACE_DEBUG((LM_DEBUG,"%s:%d,GBT_ERROR_OTHER\n",__FILE__,__LINE__));
#endif
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] =
			               m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex]; //索引个数与子对象
		ucIndexCnt = (m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex]>>6) & 0x3;  //索引个数
		ucSubId    = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex] & 0x3F;       //子对象，字段下标
		iRecvIndex++;
		iSendIndex++;

		/***********索引**************/
		if ( ucIndexCnt > 0 )  /*索引1*/
		{
			if ( iRecvIndex >= iRecvBufLen )
			{
				ucErrorSts = GBT_ERROR_OTHER;
#ifdef TSC_DEBUG
				ACE_DEBUG((LM_DEBUG,"%s:%d,GBT_ERROR_OTHER\n",__FILE__,__LINE__));
#endif
				GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
				return;
			}
			ucIdxFst = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucIdxFst;
			iRecvIndex++;
			iSendIndex++;
			ucIndexCnt--;
		}
		if ( ucIndexCnt > 0 ) /*索引2*/
		{
			if ( iRecvIndex >= iRecvBufLen )
			{
				ucErrorSts = GBT_ERROR_OTHER;
#ifdef TSC_DEBUG
				ACE_DEBUG((LM_DEBUG,"%s:%d,GBT_ERROR_OTHER\n",__FILE__,__LINE__));
#endif
				GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
				return;
			}
			ucIdxSnd = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucIdxSnd;
			iRecvIndex++;
			iSendIndex++;
			ucIndexCnt--;
		}
		if ( ucIndexCnt > 0 )  /*索引3忽略*/
		{
			if ( iRecvIndex >= iRecvBufLen )
			{
				ucErrorSts = GBT_ERROR_OTHER;
#ifdef TSC_DEBUG
				ACE_DEBUG((LM_DEBUG,"%s:%d,GBT_ERROR_OTHER\n",__FILE__,__LINE__));
#endif
				GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
				return;
			}
			iRecvIndex++;
			iSendIndex++;
			ucIndexCnt--;
		}
		
		/************值域**************/
		if ( IsSendTscCommand(ucObjId) )  //发送信号机控制对象
		{
			 if ( ! ( GBT_SEEK_REQ != ucRecvOptType ) )
			 {
				 ucErrorSts = GBT_ERROR_OTHER;
#ifdef TSC_DEBUG
				 ACE_DEBUG((LM_DEBUG,"%s:%d,IsSendTscCommand && !SET\n",__FILE__,__LINE__));
#endif
				 GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
				 return;
			 }
			//向信号机进程发送控制命令
			if ( !SendTscCommand(ucObjId,m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex]) )   //发送失败
			{
				ucErrorSts = GBT_ERROR_OTHER;
#ifdef TSC_DEBUG
				ACE_DEBUG((LM_DEBUG,"%s:%d,SendTscCommand error\n",__FILE__,__LINE__));
#endif
				GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
				return;
			}
			iRecvIndex++;
		}
		else //数据库操作对象
		{
			if ( GBT_SEEK_REQ == ucRecvOptType ) //查询
			{
				iFunRet = GBT_DB::ExchangeData(1,
									    ucObjId,      //表名
										ucIdxFst,     //第一个索引(id1)
										ucIdxSnd,     //第二个索引(id2)
										ucSubId,      //子对象
					                    m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex,      //值
					                    MAX_BUF_LEN-iSendIndex,    //发送帧的剩余缓存长度
										ucErrorSts,  //错误状态
										ucErrorIdx); //错误索引
				ACE_DEBUG((LM_DEBUG,"%s:%d,ucObjId:%02X  ucIdxFst:%d ucIdxSnd:%d ucSubId:%d	sizeleft:%d	 \n",__FILE__,__LINE__,ucObjId,ucIdxFst,ucIdxSnd,ucSubId,MAX_BUF_LEN-iSendIndex));
				if ( iFunRet < 0 )
				{
#ifdef TSC_DEBUG
					ACE_DEBUG((LM_DEBUG,"%s:%d,Database operate error iFunRet:%d\n",__FILE__,__LINE__,iFunRet));
#endif
					GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
					return;
				}
				else
				{
					iSendIndex += iFunRet;
				}
			}
			else if ( (GBT_SET_REQ == ucRecvOptType) || (GBT_SET_REQ_NOACK == ucRecvOptType) ) //设置
			{
				iFunRet = GBT_DB::ExchangeData(0,
									ucObjId,      //表名
									ucIdxFst,     //第一个索引(id1)
									ucIdxSnd,     //第二个索引(id2)
									ucSubId,      //子对象
									m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf+iRecvIndex,      //值
									m_sGbtDealData[ucDealDataIndex].sRecvFrame.iBufLen-iRecvIndex,     //数据长度
									ucErrorSts,  //错误状态
									ucErrorIdx); //错误索引
				if ( iFunRet < 0 )
				{
#ifdef TSC_DEBUG
					ACE_DEBUG((LM_DEBUG,"%s:%d,Database operate error iFunRet:%d\n",__FILE__,__LINE__,iFunRet));
#endif
					GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
					return;
				}
				else
				{
					SThreadMsg sTscMsg;
					sTscMsg.ulType       = TSC_MSG_UPDATE_PARA;  
					sTscMsg.ucMsgOpt     = 0;
					sTscMsg.uiMsgDataLen = 0;
					sTscMsg.pDataBuf     = NULL;
					CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));
					
					iRecvIndex += iFunRet;
				}
			}
		}
		
		m_sGbtDealData[ucDealDataIndex].iObjNum--;
		if ( iRecvIndex == iRecvBufLen )  //收到的字节数已处理完毕
		{
			if ( 0 == m_sGbtDealData[ucDealDataIndex].iObjNum )  //对象数也刚好处理完毕
			{
				m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = iSendIndex;
				GotoSendToHost(ucDealDataIndex);
				return;
			}
			else
			{
#ifdef TSC_DEBUG
				ACE_DEBUG((LM_DEBUG,"%s:%d,ObjNum error num:%d\n"
					                ,__FILE__,__LINE__,m_sGbtDealData[ucDealDataIndex].iObjNum));
#endif
				ucErrorSts = 5;
				ucErrorIdx = 0;
				GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
				return;
			}
			
		}		
		/*
		ucIndexCnt    = 0;   //索引个数
		ucErrorSts    = 0;   //错误状态
		ucErrorIdx    = 0;   //错误索引
		ucObjId       = 0;   //对象名(表名)
		ucIdxFst      = 255;  //第一个索引(id1)
		ucIdxSnd      = 255;  //第二个索引(id2)
		ucSubId       = 0;    //子对象(字段下标)
		*/
	}
}


/**************************************************************
Function:       CGbtMsgQueue::GetSendOperateType
Description:    获取对udp数据返回的操作类型
Input:         	ucRecvOptType  接收的udp操作类型
Output:        	无
Return:         要返回udp数据类型
***************************************************************/
Byte CGbtMsgQueue::GetSendOperateType(Byte ucRecvOptType)
{
	if ( GBT_SEEK_REQ == ucRecvOptType )
	{
		return GBT_SEEK_ACK;
	}
	else if ( GBT_SET_REQ == ucRecvOptType )
	{
		return GBT_SET_ACK;
	}
	else 
	{
		return GBT_OTHER_ACK;
	}
}


/**************************************************************
Function:       CGbtMsgQueue::SelfReport
Description:    自动上报功能
Input:         	uiDataLen  数据长度
				pDataBuf   数据缓存指针
Output:        	无
Return:         无
***************************************************************/
void CGbtMsgQueue::SelfReport(unsigned int uiDataLen,Byte* pDataBuf)
{
	Byte ucIndex = 0;
	Byte sendData[MAX_GBT_MSG_LEN];
	//事件类型 pDataBuf[0]
	//根据事件类型编号，获取信息.......
	//事件日志表 pDataBuf[1]
	//根据事件日志编号，获取信息.......
	//构造主动上报的信息，模拟........
	sendData[ucIndex++] = 0xA3;
	
	sendData[ucIndex++] = OBJECT_EVENTTYPE_TABLE;
	sendData[ucIndex++] = 0x43;
	sendData[ucIndex++] = 0x01;
	sendData[ucIndex++] = 0x01;
	ACE_OS::memcpy(sendData+ucIndex,"CTRL_MANUAL",ACE_OS::strlen("CTRL_MANUAL"));
	ucIndex = ucIndex + (Byte)ACE_OS::strlen("CTRL_MANUAL");
	
	sendData[ucIndex++] = OBJECT_EVENTLOG_TEBLE;
	sendData[ucIndex++] = 0x43;
	sendData[ucIndex++] = 0x01;
	sendData[ucIndex++] = 0x01;
	sendData[ucIndex++] = 0x0f;
	sendData[ucIndex++] = 0xff;
	sendData[ucIndex++] = 0xff;
	sendData[ucIndex++] = 0xff;
	
}


/**************************************************************
Function:       CGbtMsgQueue::SendTscCommand
Description:    发送Tsc的控制命令,以及构造回送上位机的信息
Input:         	ucObjType:对象标示  
				ucValue： 发送值
Output:        	无
Return:         true：发送成功    false：发送失败
***************************************************************/
bool CGbtMsgQueue::SendTscCommand(Byte ucObjType,Byte ucValue)
{
	//bool bRight = true;
	CManaKernel *pManaKernel = CManaKernel::CreateInstance();
	Uint uiTscCtrl    = CManaKernel::CreateInstance()->m_pRunData->uiCtrl;
	Uint uiWorkStatus = CManaKernel::CreateInstance()->m_pRunData->uiWorkStatus;

	switch ( ucObjType )
	{
		case OBJECT_CURTSC_CTRL:  						/*进入手控状态ucValue:4;ucValue:2联网；*/
			if ( (ucValue < 1) || (ucValue > 6) )		//ucValue:3主机；ucValue:5多时段；ucValue:6线控
			{											///
				return false;
			}
			SThreadMsg sTscMsg;
			sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
			sTscMsg.ucMsgOpt     = 0;
			sTscMsg.uiMsgDataLen = 1;
			sTscMsg.pDataBuf     = ACE_OS::malloc(1);
			*((Byte*)sTscMsg.pDataBuf) = GbtCtrl2TscCtrl(ucValue);  
			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));
			break;
		case OBJECT_SWITCH_MANUALCONTROL:   
			if ( 0 == ucValue )		//标准运行，有感应，多时段，联网等
			{
				SThreadMsg sTscMsg;
				sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsg.ucMsgOpt     = 0;
				sTscMsg.uiMsgDataLen = 1;
				sTscMsg.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsg.pDataBuf) = CTRL_LAST_CTRL; 
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = STANDARD; 
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( (ucValue>0) && (ucValue<33) ) //进行手控中的某个特定时间方案
			{
				SThreadMsg sTscMsg;
				sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsg.ucMsgOpt     = 0;
				sTscMsg.uiMsgDataLen = 1;
				sTscMsg.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsg.pDataBuf) = CTRL_PANEL;   
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

				m_iManualCtrlNo = ucValue;
				SThreadMsg sTscMsgTm;
				sTscMsgTm.ulType       = TSC_MSG_TIMEPATTERN;  
				sTscMsgTm.ucMsgOpt     = 0;
				sTscMsgTm.uiMsgDataLen = 1;
				sTscMsgTm.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgTm.pDataBuf) = ucValue; 
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgTm,sizeof(sTscMsgTm));
			}
			else if ( 253 == ucValue )	//进行全红控制
			{
				SThreadMsg sTscMsg;
				sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsg.ucMsgOpt     = 0;
				sTscMsg.uiMsgDataLen = 1;
				sTscMsg.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsg.pDataBuf) = CTRL_PANEL;   
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = ALLRED;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( 254 == ucValue )	//进行黄闪控制
			{
				SThreadMsg sTscMsg;
				sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsg.ucMsgOpt     = 0;
				sTscMsg.uiMsgDataLen = 1;
				sTscMsg.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsg.pDataBuf) = CTRL_PANEL;   
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = FLASH; 
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( 255 == ucValue )	//进行关灯控制
			{
				SThreadMsg sTscMsg;
				sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsg.ucMsgOpt     = 0;
				sTscMsg.uiMsgDataLen = 1;
				sTscMsg.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsg.pDataBuf) = CTRL_PANEL;   
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = SIGNALOFF;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else
			{
				return false;
			}
			break;

		case OBJECT_SWITCH_SYSTEMCONTROL:
			if ( CTRL_PANEL == uiTscCtrl )
			{
				return false;
			}
			else if ( 0 == ucValue )
			{
				if( pManaKernel->m_iTimePatternId == 250)
				{
					pManaKernel->bNextDirec = false ;
					pManaKernel->m_iTimePatternId = 0;
					pManaKernel->bTmpPattern = false ;
				}
				SThreadMsg sTscMsg;
				sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsg.ucMsgOpt     = 0;
				sTscMsg.uiMsgDataLen = 1;
				sTscMsg.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsg.pDataBuf) = CTRL_LAST_CTRL; 
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = STANDARD;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
				CMainBoardLed::CreateInstance()->DoAutoLed(true);
			}
			else if ( (ucValue>0) && (ucValue<33) )
			{
				m_iSystemCtrlNo = ucValue;

				SThreadMsg sTscMsg;
				sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsg.ucMsgOpt     = 0;
				sTscMsg.uiMsgDataLen = 1;
				sTscMsg.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsg.pDataBuf) = CTRL_UTCS;   
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

				SThreadMsg sTscMsgTime;
				sTscMsgTime.ulType       = TSC_MSG_TIMEPATTERN;  
				sTscMsgTime.ucMsgOpt     = 0;
				sTscMsgTime.uiMsgDataLen = 1;
				sTscMsgTime.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgTime.pDataBuf) = ucValue;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgTime,sizeof(sTscMsgTime));
			}
			else if ( 253 == ucValue )
		    {
				SThreadMsg sTscMsg;
				sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsg.ucMsgOpt     = 0;
				sTscMsg.uiMsgDataLen = 1;
				sTscMsg.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsg.pDataBuf) = CTRL_MANUAL;   
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

		    	SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = ALLRED;  //全红
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( 254 == ucValue )
		    {
				SThreadMsg sTscMsg;
				sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsg.ucMsgOpt     = 0;
				sTscMsg.uiMsgDataLen = 1;
				sTscMsg.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsg.pDataBuf) = CTRL_MANUAL;   
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

		    	SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = FLASH;  //黄闪
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( 255 == ucValue )
			{
				SThreadMsg sTscMsg;
				sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsg.ucMsgOpt     = 0;
				sTscMsg.uiMsgDataLen = 1;
				sTscMsg.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsg.pDataBuf) = CTRL_MANUAL;   
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = SIGNALOFF;  //关灯
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else
			{
				return false;
			}
			break;

		case OBJECT_SWITCH_CONTROL:  /***控制方式切换***/
			if ( uiTscCtrl == CTRL_PANEL || ucValue == 0 )   //当前如果是面板控制 或ucValue=0 无意义的
			{
				return false;
			}
			
		if(ucValue>=1 && ucValue<=3)  //ADD 201310211800
		{
				SThreadMsg sTscMsg;
				sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsg.ucMsgOpt     = 0;
				sTscMsg.uiMsgDataLen = 1;
				sTscMsg.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsg.pDataBuf) = CTRL_MANUAL;   
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));
			
			if ( 1 == ucValue ) 			
			{				
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = SIGNALOFF;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( 2 == ucValue )
			{
				
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = FLASH;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( 3 == ucValue )
			{
				
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = ALLRED;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			return true ;
		}
		 if ( 5 ==  ucValue )//ADD 20131021 2100 添加多时段
			{
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = CTRL_SCHEDULE;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( 6 ==  ucValue )
			{
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = CTRL_VEHACTUATED;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( 8 == ucValue )
			{
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = CTRL_ACTIVATE;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( 11 == ucValue )
			{
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = CTRL_HOST;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( 12 == ucValue )
			{
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = CTRL_UTCS;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( 13 == ucValue )
			{
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = CTRL_LINE;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( 10 == ucValue )  
			{
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = CTRL_MANUAL;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
				CMainBoardLed::CreateInstance()->DoAutoLed(false);
			}
			else
			{
				return false;
			}
			break;
		
		case OBJECT_SWITCH_STAGE: /*相位阶段切换*/
			if ( (uiTscCtrl != CTRL_PANEL && uiTscCtrl != CTRL_MANUAL)|| (uiWorkStatus != STANDARD) )

			{
				return false;
			}
			else if(ucValue == 0)
			{
				CManaKernel *pManaKernel = CManaKernel::CreateInstance();
				if(pManaKernel->m_bNextPhase == true)
					return false;
				else
				{
					pManaKernel->m_bNextPhase = true ;
				}
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_NEXT_STAGE;  //按阶段前进
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = NULL ;
				
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
				
			}
			else if ( (ucValue>0) && (ucValue<17) )
			{
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_LOCK_STAGE;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = ucValue; 
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
				
			}			
			else if( ucValue == 250)
			{
				
			}
			else 
			{
				return false;
			}
			break;

		case OBJECT_GOSTEP: /***步进和跳步操作***/ 
			//ACE_DEBUG((LM_DEBUG,"%s:%d Send Next Step TscMsg ! uiTscCtrl%d uiWorkStatus%d\n",__FILE__,__LINE__,uiTscCtrl,uiWorkStatus));
			if ( (uiTscCtrl != CTRL_PANEL && uiTscCtrl != CTRL_MANUAL)|| (uiWorkStatus != STANDARD) )
			{
				return false;
			}
			else if ( 0 == ucValue )  //步进操作
			{
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_LOCK_STEP;  //锁定步伐
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = 0;
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
				//ACE_DEBUG((LM_DEBUG,"%s:%d Send Next Step TscMsg ! \n",__FILE__,__LINE__));
				pManaKernel->SndMsgLog(LOG_TYPE_MANUAL,6,0,0,0);
			}
			else if ( (ucValue > 0) && (ucValue < MAX_PHASE) )  //锁定步伐后进行跳步
			{
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_LOCK_PHASE;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = ucValue;
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else
			{
				return false;
			}
			break;

		default:
			break;
		}	

	return true;
}


/**************************************************************
Function:       CGbtMsgQueue::ToObjectControlSwitch
Description:    将当前的控制模式，转化为协议的控制模式代码(OBJECT_CONTROL_SWITCH)
Input:         	uiWorkSts:工作状态  
				uiCtrl： 控制方式
Output:        	无
Return:         协议控制数字
***************************************************************/
Byte CGbtMsgQueue::ToObjectControlSwitch(unsigned int uiWorkSts,unsigned int uiCtrl)
{
	if ( uiWorkSts == SIGNALOFF )
	{
		return 1;
	}
	else if ( uiWorkSts == FLASH )
	{
		return 2;
	}
	else if ( uiWorkSts == ALLRED )
	{
		return 3;
	}
	else
	{
		switch ( uiCtrl )
		{
		case CTRL_VEHACTUATED:
		case CTRL_MAIN_PRIORITY:
		case CTRL_SECOND_PRIORITY:
			return 6;
		case CTRL_ACTIVATE:
			return 8;
		case CTRL_HOST:
			return 11;
		case CTRL_UTCS:
			return 12;
		case CTRL_LINE:
			return 13;
		default:
			return 14;
		}
	}
}



/**************************************************************
Function:       CGbtMsgQueue::ToObjectControlSwitch
Description:    将当前的控制模式，转化为协议的控制模式代码(OBJECT_CUTTSC_CTRL)
Input:         	uiCtrl： 控制方式
Output:        	无
Return:         协议控制数字
***************************************************************/
Byte CGbtMsgQueue::ToObjectCurTscCtrl(unsigned int uiCtrl)
{
	switch ( uiCtrl )
	{
	case CTRL_UTCS:
		return 2;
	case CTRL_HOST:
		return 3;
	case CTRL_PANEL:
		return 4;
	case CTRL_SCHEDULE:
		return 5;
	case CTRL_LINE:
		return 6;
	default:
		return 1;
	}
}


/**************************************************************
Function:       CGbtMsgQueue::SendToHost
Description:    向客户端发送消息
Input:         	ucDealDataIndex： 待处理信息下标
Output:        	无
Return:         无
***************************************************************/
void CGbtMsgQueue::SendToHost(Byte ucDealDataIndex)
{
	int iSendToClient = 0;
	Byte ucOperateType = (m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[0]) & 0xf;  //操作类型

	if ( GBT_SET_REQ_NOACK == ucOperateType )  //设置无应答
	{
		//ACE_DEBUG((LM_DEBUG,"%s:%d ringht frame GBT_SET_REQ_NOACK\n",__FILE__,__LINE__));
	}
	else
	{
		if ( m_sGbtDealData[ucDealDataIndex].bReportSelf ) //主动上报
		{
			/*
			if ( OBJECT_DETECTORSTS_TABLE == m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[1] )  //检测器状态
			{
				
				static Byte ucDetStatus[28] = {0};
				bool bSend = false;
				for (Byte ucIndex=0; ucIndex<28; ucIndex++ )
				{
					if ( ucDetStatus[ucIndex] != m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[ucIndex] )
					{
						bSend = true;
					}
				}
				
				if ( !bSend )  //检测器与上次的状态一致 不必发送直到状态改变
				{
					CleanDealData(ucDealDataIndex);
					return;
				}
				else
				{
					ACE_OS::memcpy(ucDetStatus , m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf , 28);
				}
			}*/

			Byte ucTmp =  (m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[0]) & 0xf0;
			ucTmp      = ucTmp | GBT_SELF_REPORT; 
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[0] = ucTmp;
			m_sGbtDealData[ucDealDataIndex].bReportSelf = false;
		}
#ifdef GBT_TCP
		iSendToClient 
			= m_sGbtDealData[ucDealDataIndex].SockStreamClient.send(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf,
			                            m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen);
#else
		iSendToClient = m_sockLocal.send(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf,
			                             m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen,
								         m_sGbtDealData[ucDealDataIndex].AddrClient);
#endif
		#if 0
		if ( iSendToClient != m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen )
		{
			ACE_DEBUG((LM_DEBUG,"CGbtMsgQueue.cpp:%d send to client error %d-%d\n"
				,__LINE__,iSendToClient,m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen));
		}
		else
		{
			ACE_DEBUG((LM_DEBUG,"CGbtMsgQueue.cpp:%d send to client right %d\n",__LINE__,iSendToClient));
		}
		#endif
		
	}

	CleanDealData(ucDealDataIndex);
}


/**************************************************************
Function:       CGbtMsgQueue::CleanDealData
Description:    清理待处理数据
Input:         	ucDealDataIndex： 待处理信息下标
Output:        	无
Return:         无
***************************************************************/
void CGbtMsgQueue::CleanDealData(Byte ucDealDataIndex)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_sMutex);
	
	m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex  = 0;
	m_sGbtDealData[ucDealDataIndex].sRecvFrame.iBufLen = 0;
	m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex  = 0;
	m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = 0;
 
#ifdef GBT_TCP
 	//tcp 发送完数据后连接还存在
#else
	m_sGbtDealData[ucDealDataIndex].bIsDeal = false;   //udp 不必保存连接 下次会再有ip信息过来
#endif
}


/**************************************************************
Function:       CGbtMsgQueue::FirstRecv
Description:    根据消息队列收到的数据块解析构造SGbtDealData的sRecvFrame
				解析完pBuf的内存将被释放
Input:         	ucDealDataIndex： 待处理信息下标
				iBufLen           接收到的udp数据长度
				pBuf              接收数据缓存指针
Output:        	无
Return:         无
***************************************************************/
void CGbtMsgQueue::FirstRecv(Byte ucDealDataIndex,Uint iBufLen,Byte* pBuf)
{
	SThreadMsg sGbtMsg;
	m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex = 0;
	m_sGbtDealData[ucDealDataIndex].sRecvFrame.iBufLen = iBufLen;
	ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf,pBuf,iBufLen);

	sGbtMsg.ulType       = GBT_MSG_DEAL_RECVBUF;  
	sGbtMsg.ucMsgOpt     = ucDealDataIndex;
	sGbtMsg.uiMsgDataLen = iBufLen;  //原始收到数据的大小
	sGbtMsg.pDataBuf     = NULL;	
	SendGbtMsg(&sGbtMsg,sizeof(sGbtMsg));
}


/**************************************************************
Function:       CGbtMsgQueue::CheckMsg
Description:    检测接收到的数据合法性
Input:         	ucDealDataIndex： 待处理信息下标
				iBufLen           接收到的udp数据长度
				pBuf              接收数据缓存指针
Output:        	无
Return:         无			0:非法消息 1：合法消息
***************************************************************/
int CGbtMsgQueue::CheckMsg(Byte ucDealDataIndex,Uint iBufLen,Byte* pBuf)
{
	if ( pBuf == NULL )
	{
		return -1;
	}

	if ( (int)iBufLen < MIN_GBT_MSG_LEN ) //协议的消息长度短
	{
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[0] = 0x86;
							//(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[0] & 0xf0 ) | GBT_ERR_ACK; 
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[1] = GBT_ERROR_SHORT;
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[2] = 0;
	}
	else if ( (pBuf[0]&0xf)<GBT_SEEK_REQ || (pBuf[0]&0xf)>GBT_SET_REQ_NOACK || ((pBuf[0]>>7)&1)!=1 ) //协议其它问题
	{
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[0] = 0x86;
							//(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[0] & 0xf0 ) | GBT_ERR_ACK;
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[1] = GBT_ERROR_OTHER;
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[2] = 0;
	}
	else
	{
		return 1; //协议正确
	}

	m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = 3;
	SThreadMsg sGbtMsg;
	sGbtMsg.ulType       = GBT_MSG_SEND_HOST;
	sGbtMsg.ucMsgOpt     = ucDealDataIndex;
	sGbtMsg.uiMsgDataLen = 0;
	sGbtMsg.pDataBuf     = NULL;	
	SendGbtMsg(&sGbtMsg,sizeof(sGbtMsg));

	return 0;
}

#ifdef GBT_TCP

/**************************************************************
Function:       CGbtMsgQueue::GetDealDataIndex
Description:    获取待处理数据的下标 TCP
Input:         	bReportSelf - 是否为主动上报
     		    sockStreamRemote  - 客户端地址
Output:        	无
Return:         0-7 200：没有可用的缓存数组
***************************************************************/
Byte CGbtMsgQueue::GetDealDataIndex(bool bReportSelf , ACE_SOCK_Stream& sockStreamRemote)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_sMutex);

	int iDealIndex = -1;
	ACE_Addr addrNew;
	ACE_Addr addrOld;
	
	sockStreamRemote.get_remote_addr(addrNew);
	for (Byte i=0; i<MAX_CLIENT_NUM; i++)
	{
		if ( m_sGbtDealData[i].bIsDeal == false )
		{
			if ( -1 == iDealIndex )
			{
				iDealIndex = i;
			}	
		}
		else
		{
			m_sGbtDealData[i].SockStreamClient.get_remote_addr(addrOld);
			if ( addrNew == addrOld )
			{
				m_sGbtDealData[i].bReportSelf      = bReportSelf;
				m_sGbtDealData[i].SockStreamClient = sockStreamRemote;
				return i;
			}
		}
	}

	if ( iDealIndex >=0 && iDealIndex<MAX_CLIENT_NUM )
	{
		m_sGbtDealData[iDealIndex].bIsDeal          = true;
		m_sGbtDealData[iDealIndex].bReportSelf      = bReportSelf;
		m_sGbtDealData[iDealIndex].SockStreamClient = sockStreamRemote;
		return iDealIndex;
	}

	return 200;
}

SGbtDealData* CGbtMsgQueue::GetGbtDealDataPoint()
{
	return m_sGbtDealData;
}

#else

/**************************************************************
Function:       CGbtMsgQueue::GetDealDataIndex
Description:    获取待处理数据的下标 UDP
Input:         	bReportSelf - 是否为主动上报
     		    addrRemote  - 客户端地址
Output:        	无
Return:         0-7 200：没有可用的缓存数组
***************************************************************/
Byte CGbtMsgQueue::GetDealDataIndex(bool bReportSelf , ACE_INET_Addr& addrRemote)
{
	
	ACE_Guard<ACE_Thread_Mutex> guard(m_sMutex);

	for (Byte i=0; i<MAX_CLIENT_NUM; i++)
	{
		if ( m_sGbtDealData[i].bIsDeal == false )
		{
			m_sGbtDealData[i].bIsDeal     = true;
			m_sGbtDealData[i].bReportSelf = bReportSelf;
			m_sGbtDealData[i].AddrClient.set(ACE_INET_Addr(addrRemote));
			//m_sGbtDealData[i].AddrClient.set_port_number(61394);  // ADD: 2013 07 30 in order to test udp send and recv.
			return i;
		}
	}

	return 200;
}
#endif


/**************************************************************
Function:       CGbtMsgQueue::RunGbtRecv
Description:    udp数据接收线程函数,待接收上位机的UDP 数据，然后转送处理
Input:         	arg  默认NULL，线程函数参数
Output:        	无
Return:         0
***************************************************************/
void* CGbtMsgQueue::RunGbtRecv(void* arg)
{
	ACE_DEBUG((LM_DEBUG,"%s:%d Begin Gbt date recv thread!\n",__FILE__,__LINE__));
	Byte ucDealDataIndex = 0;
	int iRecvCount = 0;
	Byte pBuf[MAX_BUF_LEN] = {0};
	SThreadMsg sMsg;
	CGbtMsgQueue* pGbtMsgQueue = CGbtMsgQueue::CreateInstance();
#ifdef GBT_TCP
	ACE_SOCK_Stream sockStreamRemote;
	SGbtDealData* pGbtDealData = pGbtMsgQueue->GetGbtDealDataPoint();
	ACE_Time_Value timeout(0,10*1000);
#else
	ACE_INET_Addr addrRemote;
#endif
	
	while ( true )
	{
		#ifdef GBT_TCP
		if ( pGbtMsgQueue->m_acceptor.accept(sockStreamRemote,NULL,&timeout) != -1 ) //新的客户端连接
		{
			ucDealDataIndex = pGbtMsgQueue->GetDealDataIndex(false , sockStreamRemote);

			ACE_DEBUG((LM_DEBUG,"%s:%d **********%d \n",__FILE__,__LINE__,ucDealDataIndex));
		}

		for (Byte i=0; i<MAX_CLIENT_NUM; i++)
		{
			if ( pGbtDealData[i].bIsDeal == true )
			{
				if ( ( iRecvCount = pGbtDealData[i].SockStreamClient.recv(pBuf,MAX_BUF_LEN))  != -1 )		
				{
					//#if 1
					ACE_DEBUG((LM_DEBUG,"\n%s:%d ",__FILE__,__LINE__));
					for (int iIndex=0; iIndex<iRecvCount; iIndex++ )
					{
						ACE_DEBUG((LM_DEBUG," %x ",pBuf[iIndex]));
					}
					ACE_DEBUG((LM_DEBUG,"\n"));
					//#endif

					sMsg.ulType       = GBT_MSG_FIRST_RECV;
					sMsg.ucMsgOpt     = i;
					sMsg.uiMsgDataLen = iRecvCount;
					sMsg.pDataBuf     = ACE_OS::malloc(iRecvCount);
					ACE_OS::memcpy(sMsg.pDataBuf,pBuf,iRecvCount);

					pGbtMsgQueue->SendGbtMsg(&sMsg,sizeof(sMsg));

					ACE_OS::memset(&sMsg,0,sizeof(sMsg));
					ACE_OS::memset(pBuf,0,MAX_BUF_LEN);
				}
			}
		}
		
#else
		if ( ( iRecvCount = (pGbtMsgQueue->m_sockLocal).recv(pBuf, MAX_BUF_LEN,addrRemote))  != -1 )		
		{

			#if 1
			ACE_DEBUG((LM_DEBUG,"%s:%d Get %d Byte udp data:  ",__FILE__,__LINE__,iRecvCount));
			for (int iIndex=0; iIndex<iRecvCount; iIndex++ )
			{
				ACE_DEBUG((LM_DEBUG,"%x ",pBuf[iIndex]));
			}
			ACE_DEBUG((LM_DEBUG,"\n"));
			#endif

			ucDealDataIndex = pGbtMsgQueue->GetDealDataIndex(false , addrRemote);
			
			if ( ucDealDataIndex < MAX_CLIENT_NUM )
			{
				sMsg.ulType       = GBT_MSG_FIRST_RECV;
				sMsg.ucMsgOpt     = ucDealDataIndex;
				sMsg.uiMsgDataLen = iRecvCount;
				sMsg.pDataBuf     = ACE_OS::malloc(iRecvCount);
				ACE_OS::memcpy(sMsg.pDataBuf,pBuf,iRecvCount);

				pGbtMsgQueue->SendGbtMsg(&sMsg,sizeof(sMsg));
				ACE_DEBUG((LM_DEBUG,"%s :%d recv udp data from %s  %d !\n",__FILE__,__LINE__,addrRemote.get_host_addr(),addrRemote.get_port_number())); //ADD: 2013 0613 1008
			}
			else
			{	///超过4个客户大小，无法再接收新数据
				ACE_OS::memset(pBuf,0,MAX_GBT_MSG_LEN);
				pBuf[0] = 0x86;                 //错误应答GBT协议错误应答
				pBuf[1] = GBT_ERROR_OTHER + 1; //不属于上述类型的其他错误
				pBuf[2] = 0;                    //不是因为数据字段的值引起的
				(pGbtMsgQueue->m_sockLocal).send(pBuf , 3 , addrRemote);
				//CGbtMsgQueue::CreateInstance()->PrintIpList();
			}
			ACE_OS::memset(&sMsg,0,sizeof(sMsg));
			ACE_OS::memset(pBuf,0,MAX_BUF_LEN);
		}
#endif
	}

	return 0;
}


/**************************************************************
Function:       CGbtMsgQueue::PrintIpList
Description:    打印客户端ip列表
Input:         	arg  默认NULL，线程函数参数
Output:        	无
Return:         0
***************************************************************/
void CGbtMsgQueue::PrintIpList()
{	
	for ( Byte ucIndex=0; ucIndex<MAX_CLIENT_NUM; ucIndex++ )
	{
		//ACE_DEBUG((LM_DEBUG,"%d %s %x\n"
			//, m_sGbtDealData[ucIndex].bIsDeal
			//, m_AddrArr[ucIndex].get_host_addr()
			//, m_AddrArr[ucIndex].get_ip_address()));

		//CleanDealData(ucIndex);
	}
	
}


/**************************************************************
Function:       CGbtMsgQueue::IsGetTscStatusObject
Description:    判断是否为获取信号机状态对象
Input:         	ucObjectFlag  对象标志
Output:        	无
Return:         true:是  false:否
***************************************************************/
bool CGbtMsgQueue::IsGetTscStatusObject(Byte ucObjectFlag)
{
	switch ( ucObjectFlag )
	{
		case OBJECT_CHANNELSTATUS_TABLE:     //通道输出状态表   灯控端口参数
		case OBJECT_OVERLAPPHASE_STATUS:     //跟谁相位状态表   跟随相位参数
		case OBJECT_PHASESTATUS_TABLE:		 //相位输出状态表   相位参数
		case OBJECT_CURTSC_FLASHCTRL:        //当前闪光控制模式 单元参数
		case OBJECT_ACTIVESCHEDULE_NO:       //活动时段表编号   公共事件参数
		//case OBJECT_SWITCH_STAGE:
		case OBJECT_TSC_WARN2:				//信号机报警2       单元参数
		case OBJECT_TSC_WARN1:				//信号机报警1       单元参数
		case OBJECT_TSC_WARN_SUMMARY:       //信号机报警摘要    单元参数
		case OBJECT_ACTIVEDETECTOR_NUM:     //活动检测器总数    检测器参数
		case OBJECT_DETECTORSTS_TABLE:      //检测器状态表      检测器参数
		case OBJECT_DETECTORDATA_TABLE:     //交通检测数据表    检测器参数
		case OBJECT_DETECTORWARN_TABLE:     //车辆检测器警告参数表 检测器参数
		case OBJECT_CURPATTERN_SCHTIMES:    //当前方案个阶段市场  控制参数
		case OBJECT_CURPATTERN_GREENTIMES:  //当前方案各关键相位绿灯市场 控制参数
#ifdef TSC_DEBUG
		ACE_DEBUG((LM_DEBUG,"\n IsGetTscStatusObject\n"));
#endif
		return true;
	default:
		return false;
	}
	return false;
}


/**************************************************************
Function:       CGbtMsgQueue::IsSendTscCommand
Description:    判断是否为发送信号机控制命令
Input:         	ucObjectFlag  对象标志
Output:        	无
Return:         true:是  false:否
***************************************************************/
bool CGbtMsgQueue::IsSendTscCommand(Byte ucObjectFlag)
{
	switch ( ucObjectFlag )
	{
		case OBJECT_CURTSC_CTRL:     //当前信号机控制状态   单元参数
		case OBJECT_SWITCH_MANUALCONTROL: //手动控制方案   控制参数
		case OBJECT_SWITCH_SYSTEMCONTROL: //系统控制方案  控制参数
		case OBJECT_SWITCH_CONTROL:       //控制方式     控制参数
		case OBJECT_SWITCH_STAGE:         //阶段状态     控制参数
		case OBJECT_GOSTEP:               //步进指令     控制参数 
#ifdef TSC_DEBUG
		ACE_DEBUG((LM_DEBUG,"\n IsSendTscCommand\n"));
#endif
		return true;
	default:
		return false;
	}

	return false;
}


/**************************************************************
Function:       CGbtMsgQueue::IsOtherObject
Description:    判断是否其他操作对象，非获取状态，非控制命令，
				非数据库操作函数
Input:         	ucObjectFlag  对象标志
Output:        	无
Return:         true:是  false:否
***************************************************************/
bool CGbtMsgQueue::IsOtherObject(Byte ucObjectFlag)
{
	switch ( ucObjectFlag )
	{
	case OBJECT_UTC_TIME:
	case OBJECT_LOCAL_TIME:
#ifdef TSC_DEBUG
		ACE_DEBUG((LM_DEBUG,"\n IsOtherObject\n"));
#endif
		return true;
	default:
		return false;
	}

	return false;
}


/**************************************************************
Function:       CGbtMsgQueue::IsOtherObject
Description:    判断是否为扩展对象
Input:         	ucObjectFlag  对象标志
Output:        	无
Return:         true:是  false:否
***************************************************************/
bool CGbtMsgQueue::IsExtendObject(Byte ucObjectFlag)
{
	switch ( ucObjectFlag )
	{
		case OBJECT_IP:                 //信号机IP 扩展对象
		//case OBJECT_MULTICAST_INFO:	//
		case OBJECT_WATCH_PARA:         //监控类型参数 扩展对象
		case OBJECT_MODULE_STATUS:      //模块状态    扩展对象
		case OBJECT_EXT_TSC_STATUS:     //状态类型表参数 扩展对象
		case OBJECT_YWFLASH_CFG:        //黄闪器扩展对象
		case OBJECT_DET_EXTCFG :  		//检测器扩展配置
		case OBJECT_LAMPBOARD_CFG :     //灯泡检测配置
		case OBJECT_PSCBTN_NUM :         //模拟8位行人按钮输入
		case OBJECT_TMPPATTERN_CFG :     //临时12方向随机组合，放行60秒默认
		case OBJECT_SYSFUNC_CFG :        //系统其他功能设置
		case OBJECT_POWERBOARD_CFG :       //电源板配置
		case OBJECT_GSM_CFG :            //配置GSM功能
		case OBJECT_COMMAND_SIGNAL:      //上位机命令控制下一阶段相位和方向
			return true;
		default:
			return false;
	}
	return false;
}


/**************************************************************
Function:       CGbtMsgQueue::GbtCtrl
Description:    协议控制状态转为信号机的控制状态
Input:         	ucctrl  协议控制值
Output:        	无
Return:         控制状态值
***************************************************************/
int CGbtMsgQueue::GbtCtrl2TscCtrl(Byte ucctrl)
{
	switch ( ucctrl )
	{
	case 2:
		return CTRL_UTCS;
	case 3:
		return CTRL_HOST;
	case 4:
		return CTRL_PANEL;
	case 5:
		return CTRL_SCHEDULE;
	case 6:
		return CTRL_LINE;
	default:
		return -1;
	}
}
/**************************************************************
Function:       CGbtMsgQueue::UpdateNetPara
Description:    永久修改网络配置信息，这里不对其它参数进行修改比较子网掩码，网关等
				这里主要是针对m3352核心板
Input:         	cIp      - ip地址
Output:        	无
Return:         无
***************************************************************/
void CGbtMsgQueue::UpdateNetPara(Byte* pIp)
{
	char cIpCmd[64] = {0};
	Byte cIp[4]      = {0};
	if ( pIp != NULL )
	{
		ACE_OS::memcpy(cIp,pIp,4);
	}
	ACE_OS::sprintf(cIpCmd , "eeprom net set ip %d.%d.%d.%d\n",
		cIp[0] , cIp[1] , cIp[2] , cIp[3] ) ;
	system(cIpCmd);
}
/**************************************************************
Function:       CGbtMsgQueue::ReworkNetPara
Description:    修改网络配置信息
Input:         	cIp      - ip地址
				cMask           - 掩码
				cGateWay        - 网关
Output:        	无
Return:         无
***************************************************************/
void CGbtMsgQueue::ReworkNetPara(Byte* pIp , Byte* pMask , Byte* pGateWay)
{
	Byte cHwEther[6] = {0};
	Byte cIp[4]      = {0};
	Byte cMask[4]    = {0};
	Byte cGateWay[4] = {0};
	char cIpAndMaskCmd[64] = {0};
	char cGatewayCmd[64]   = {0};

	GetNetPara(cHwEther , cIp , cMask , cGateWay);
	
	if ( pIp != NULL )
	{
		ACE_OS::memcpy(cIp,pIp,4);
	}
	if ( pMask != NULL )
	{
		ACE_OS::memcpy(cMask,pMask,4);
	}
	if ( pGateWay != NULL )
	{
		ACE_OS::memcpy(cGateWay,pGateWay,4);
	}


	ACE_OS::sprintf(cIpAndMaskCmd , "/sbin/ifconfig eth0 %d.%d.%d.%d netmask %d.%d.%d.%d up\n",
		cIp[0]   , cIp[1]   , cIp[2]   , cIp[3] , 
		cMask[0] , cMask[1] , cMask[2] , cMask[3] ) ;
	ACE_OS::sprintf(cGatewayCmd , "/sbin/route add default gw %d.%d.%d.%d\n",
		cGateWay[0] , cGateWay[1] , cGateWay[2] , cGateWay[3] ) ;

	system("/sbin/ifconfig eth0 down\n");	
	system(cIpAndMaskCmd);
	system(cGatewayCmd);	
	
	RecordNetPara( cIpAndMaskCmd , cGatewayCmd );	
}


/**************************************************************
Function:       CGbtMsgQueue::RecordNetPara
Description:    永久修改网络配置参数
Input:        
				pIpAndMaskCmd - ip地址和网关命令
				pGatewayCmd   - 网关命令
Output:        	无
Return:         无
***************************************************************/
void CGbtMsgQueue::RecordNetPara(/*char* pHwEtherCmd ,*/ char* pIpAndMaskCmd , char* pGatewayCmd )
{
	FILE* fpRcs    = NULL;
	FILE* fpRcsTmp = NULL;
	char  sTmp[1024] = {0};

	//if ( NULL == ( fpRcs = ACE_OS::fopen("/etc/init.d/rcS", "r") ) )
	if ( NULL == ( fpRcs = ACE_OS::fopen("rcS", "r") ) )
	{
		return;
	}

	//if ( (fpRcsTmp = ACE_OS::fopen("/etc/init.d/rcS_tmp", "w")) != NULL ) 
	if ( (fpRcsTmp = ACE_OS::fopen("rcS_tmp", "w")) != NULL ) 
	{
		while ( fgets(sTmp , 1024 , fpRcs) != NULL ) 
		{
			/*
			if ( strstr(sTmp , "hw") != NULL && strstr(sTmp , "ether") != NULL )
			{
				ACE_OS::memset(sTmp , 0 , 1024);
				ACE_OS::memcpy(sTmp , pHwEtherCmd , strlen(pHwEtherCmd) );
			}
			else*/ if ( strstr(sTmp , "eth0") != NULL && strstr(sTmp , "netmask") != NULL ) 
			{
				ACE_OS::memset(sTmp , 0 , 1024);
				ACE_OS::memcpy(sTmp , pIpAndMaskCmd , strlen(pIpAndMaskCmd) );
			}
			else if ( strstr(sTmp , "default") != NULL && strstr(sTmp , "gw") != NULL ) 
			{
				ACE_OS::memset(sTmp , 0 , 1024);
				ACE_OS::memcpy(sTmp , pGatewayCmd , strlen(pGatewayCmd) );
			}
			fputs(sTmp, fpRcsTmp);
			ACE_OS::memset(sTmp , 0 , 1024);
		}
		fclose(fpRcs);
		fclose(fpRcsTmp);
#ifdef LINUX
		TscCopyFile((char*)"/etc/init.d/rcS_tmp" , (char*)"/etc/init.d/rcS");
		remove((char*)"/etc/init.d/rcS_tmp");
#endif
	} 
	else 
	{
		fclose(fpRcs);
	}
}

/**************************************************************
Function:       CGbtMsgQueue::GetNetParaByAce
Description:    获取物理地址 ip地址 掩码 网关
Input:        	addr_array_result - ip地址的数组
				host_name - 主机名称
Output:        	无
Return:         无
***************************************************************/
void CGbtMsgQueue::GetNetParaByAce(Byte* pip,char* phost_name)
{
#ifdef WINDOWS
//在windows 系统下在调用网络的情况，必需先执行以下两句
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2,2),&wsaData) != 0) return 0;	
#endif

	//得到主机名
	char hostname[MAXHOSTNAMELEN];
    ACE_OS::hostname(hostname, sizeof (hostname));
	phost_name = hostname;
	printf("host name is %s\n", hostname);
	printf("--------------------------\n");


    ACE_INET_Addr* addr_array1; 
    size_t count = 0; 
    if (ACE::get_ip_interfaces(count, addr_array1) != 0)
	{
        return ; 
	}

	//可以装下IPv6地址（46），IPv4为INET_ADDRSTRLEN（16）
    char address[INET6_ADDRSTRLEN];
	//为了最后要删除空间，所以定义新的变量
	 ACE_INET_Addr* addr_array2 = addr_array1; 

    for(size_t i=0; i<count; i++)
    { 
		unsigned int net_order_ip = htonl(addr_array2->get_ip_address());
		struct in_addr *paddr_stru = (in_addr*)&net_order_ip;
		InterceptStr(ACE_OS::inet_ntoa(*paddr_stru),(char*)".",pip,3);
		
		printf("format 0: %s\n", ACE_OS::inet_ntoa(*paddr_stru));

		addr_array2->addr_to_string(address, sizeof (address), 1);
		printf("format 1: %s\n", address);
        addr_array2++;
		printf("--------------------------\n");
	
    }

	//记得要delete[] addr_array;  
	delete[] addr_array1;

	//getchar();
}
/**************************************************************
Function:       CGbtMsgQueue::GetNetPara
Description:    获取物理地址 ip地址 掩码 网关
Input:        	pIp      - IP地址
				pHwEther - 物理地址
				pMask   - 子网掩码
				pGateway - 网关
Output:        	无
Return:         无
***************************************************************/
void CGbtMsgQueue::GetNetPara(Byte* pHwEther , Byte* pIp , Byte* pMask , Byte* pGateway)
{
	FILE* fpRcs      = NULL;
	char* pStart     = NULL;
	//char* pEnd       = NULL;
	char  sTmp[200] = {0};

	if ( NULL == ( fpRcs = ACE_OS::fopen("/etc/eth0-setting", "r") ) )
	{
		return;
	}

	while ( fgets(sTmp , 100 , fpRcs) != NULL ) 
	{
		if ( (pStart=strstr(sTmp , "MAC=")) != NULL )
		{
			pStart = pStart + ACE_OS::strlen("MAC=");
			InterceptStr(pStart,(char*)":",pHwEther,5);
		}
		else if ( (pStart=strstr(sTmp , "IP=")) != NULL ) 
		{
			pStart = pStart + ACE_OS::strlen("IP=") ;
			InterceptStr(pStart,(char*)".",pIp,3);
		}
		else if( (pStart=strstr(sTmp , "Mask=")) != NULL)
		{
			pStart = pStart + ACE_OS::strlen("Mask=") ;
			InterceptStr(pStart,(char*)".",pMask,3);
		}
		else if (( pStart=strstr(sTmp , "Gateway=") ) != NULL) 
		{
			pStart = pStart + ACE_OS::strlen((char*)"Gateway=");
			InterceptStr(pStart,(char*)".",pGateway,3);
		}

		pStart = NULL;
		ACE_OS::memset(sTmp , 0 , 200);
	}

	fclose(fpRcs);
}


/**************************************************************
Function:       CGbtMsgQueue::InterceptStr
Description:   截取所要的值
Input:        	pBuf  - 截取的字符串 pstr  - 截取标示
				pData - 转化后的值   ucCnt - 个数
Output:        	无
Return:         无
***************************************************************/
void CGbtMsgQueue::InterceptStr(char* pBuf, char* pstr , Byte* pData , Byte ucCnt)
{
	Byte ucIndex  = 0;
	char* pStrart = pBuf;
	char* pEnd    = pBuf;
	char  sTmp[8] = {0};

	while ( ucIndex < ucCnt )
	{
		if ( (pEnd = strstr(pStrart,pstr)) != NULL )
		{
			ACE_OS::strncpy(sTmp , pStrart , pEnd - pStrart);
			pData[ucIndex] = atoi(sTmp);
			
			pStrart = ++pEnd;
			ACE_OS::memset(sTmp , 0 , 8);
		}
	
		ucIndex++;
	}

	if ( (pEnd = strstr(pStrart," ")) != NULL )
	{
		ACE_OS::strncpy(sTmp , pStrart , pEnd - pStrart);
		pData[ucIndex] = atoi(sTmp);
	}
	else
	{
		pData[ucIndex] = atoi(pStrart);
	}
}


/**************************************************************
Function:       CGbtMsgQueue::ReworkIp
Description:    修改ip，永久生效
Input:        	ucIp1 - 192
		        ucIp2 - 168     
                ucIp3 - 1
                ucIp4 - 20
Output:        	无
Return:         无
***************************************************************/
void CGbtMsgQueue::ReworkIp(Byte ucIp1,Byte ucIp2,Byte ucIp3,Byte ucIp4)
{
	FILE* fpRcs    = NULL;
	FILE* fpRcsTmp = NULL;
	char  sTmp[1024] = {0};
	
	if ( NULL == ( fpRcs = ACE_OS::fopen("/etc/init.d/rcS", "r") ) )
	{
		return;
	}
	
	if ( (fpRcsTmp = ACE_OS::fopen("/etc/init.d/rcS_tmp", "w")) != NULL ) 
	{
		while ( fgets(sTmp , 1024 , fpRcs) != NULL ) 
		{
			if ( strstr(sTmp , "eth0") != NULL && strstr(sTmp , "netmask") != NULL )
			{
				ACE_OS::memset(sTmp , 0 , 1024);
				ACE_OS::sprintf(sTmp, "/sbin/ifconfig eth0 %d.%d.%d.%d\n"
					        , ucIp1 , ucIp2 , ucIp3 , ucIp4);
			}
			fputs(sTmp, fpRcsTmp);
			ACE_OS::memset(sTmp , 0 , 1024);
		}
		fclose(fpRcs);
		fclose(fpRcsTmp);
#ifdef LINUX
		TscCopyFile((char*)"/etc/init.d/rcS_tmp" , (char*)"/etc/init.d/rcS");
		remove("/etc/init.d/rcS_tmp");
#endif
	} 
	else 
	{
		fclose(fpRcs);
	}
}



/**************************************************************
Function:       CGbtMsgQueue::TscCopyFile
Description:    文件拷贝
Input:        	fpSrc - 源文件名字
				fpDest- 目的文件名字
Output:        	无
Return:         无
***************************************************************/
void CGbtMsgQueue::TscCopyFile(char* fpSrc, char* fpDest)
{
#ifdef LINUX
	int iSrcFd, iDestFd;
	char cBuf[100] = {0};
	int iReadCnt = 0;

	if ( strcmp(fpSrc, fpDest) == 0 )
	{
		return;
	}

	iSrcFd = open(fpSrc, O_RDONLY);
	iDestFd = open(fpDest, O_CREAT | O_WRONLY | O_TRUNC, 0755);
	if ( -1 == iSrcFd || -1 == iDestFd )
	{
		return;
	}

	while ( (iReadCnt = read(iSrcFd, cBuf, sizeof(cBuf))) > 0 ) 
	{
#ifdef TSC_DEBUG
		ACE_DEBUG((LM_DEBUG,"%s:%d iReadCnt:%d cBuf:%s" , __FILE__ , __LINE__ , iReadCnt , cBuf));
#endif
		write(iDestFd, cBuf, iReadCnt);
	}
	
	close(iSrcFd);
	close(iDestFd);
#endif
}
