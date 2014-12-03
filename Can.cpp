/***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   Can.cpp
Date:       2013-1-1
Description:CAN总线接口处理类文件.包含CAN总线数据收发操作.
Version:    V1.0
History:
***************************************************************/
#include "Can.h"
#include "Detector.h"
#include "LampBoard.h"
#include "FlashMac.h"
#include "PowerBoard.h"
#include "MacControl.h"
#include <ace/Guard_T.h>
#include "ComFunc.h" //ADD:201309281040
#include "WirelessButtons.h"

#ifndef WINDOWS
#include <sys/time.h>
#include <unistd.h>
#endif

/*
*协议版本 bit0-bit3
*/
enum
{
	B2B_PROTOCOL_V00 = 0x0F , //V0.0
	B2B_PROTOCOL_V01 = 0x0E , //V0.1
	B2B_PROTOCOL_V02 = 0x0D   //V0.2

};

/**************************************************************
Function:       Can::Can
Description:    Can，用于类初始化处理				
Input:          无              
Output:         无
Return:         0
***************************************************************/
Can::Can()
{
	pMainBoardLed = CMainBoardLed::CreateInstance();
	m_CanMsgQue 	    = ACE_Message_Queue_Factory<ACE_MT_SYNCH>::create_static_message_queue();
	InitCan();
	ACE_DEBUG((LM_DEBUG,"%s:%d Init CanBus object ok !\n",__FILE__,__LINE__));
}

/**************************************************************
Function:       Can::~Can
Description:    Can	析构函数	
Input:          无              
Output:         无
Return:         0
***************************************************************/
Can::~Can()
{
	ACE_DEBUG((LM_DEBUG,"%s:%d Destruct CanBus object ok !\n",__FILE__,__LINE__));
}

/**************************************************************
Function:       Can::CreateInstance
Description:    创建	Can静态对象
Input:          无              
Output:         无
Return:         静态对象指针
***************************************************************/
Can* Can::CreateInstance()
{
	static Can can;
	return &can;
}

/**************************************************************
Function:       Can::InitCan
Description:    初始化Can socket总线
Input:          无              
Output:         无
Return:         无
***************************************************************/
void Can::InitCan()
{
#ifdef LINUX
	m_socketHandle = socket(PF_CAN,SOCK_RAW,CAN_RAW);

	strcpy((char *)(m_ifrCan.ifr_name),"can0");
	ioctl(m_socketHandle,SIOCGIFINDEX,&m_ifrCan);

	ACE_DEBUG((LM_DEBUG,"%s:%d can0 can_ifindex = %x,m_socketHandle= %d\n",__FILE__,__LINE__,m_ifrCan.ifr_ifindex,m_socketHandle));
	m_addrCan.can_family  = AF_CAN;
	m_addrCan.can_ifindex = m_ifrCan.ifr_ifindex;
	bind(m_socketHandle,(struct sockaddr*)&m_addrCan,sizeof(m_addrCan));
#endif
}

/**************************************************************
Function:       Can::GetHandle
Description:    获取Can socket总线文件句柄
Input:          无              
Output:         无
Return:         Can socket总线文件句柄
***************************************************************/
int Can::GetHandle()
{
	return m_socketHandle;
}

/**************************************************************
Function:       Can::Send
Description:    发送Can数据包
Input:          sendFrame can数据包              
Output:         无
Return:         false-失败  true-成功
***************************************************************/
bool Can::Send(SCanFrame& sendFrame)
{
	ACE_Guard<ACE_Thread_Mutex>  guard(m_mutexCan);
	int ulBytes = 0;

	m_frameCan.can_id = sendFrame.ulCanId;
	ACE_OS::memcpy(m_frameCan.data,sendFrame.pCanData,sendFrame.ucCanDataLen);
	m_frameCan.can_dlc = sendFrame.ucCanDataLen;

	ulBytes	= sendto(m_socketHandle , &m_frameCan , sizeof(struct can_frame), 0	,(struct sockaddr*)&m_addrCan,sizeof(m_addrCan));

	if(ulBytes == -1)
	{
		int erronum = errno ;
		switch(erronum)
		{
			case EBADF :
				//ACE_DEBUG((LM_DEBUG,"%s:%d can socket 错误的文件描述符!\n",__FILE__,__LINE__));//MOD??:20130523 14 25
				break ;
			case EFAULT :
				//ACE_DEBUG((LM_DEBUG,"%s:%d 错误的地址!\n",__FILE__,__LINE__));//MOD??:20130523 14 25
				break ;
			case ENOTSOCK :
				//ACE_DEBUG((LM_DEBUG,"%s:%d can Socket操作在一个非socket上!\n",__FILE__,__LINE__));//MOD??:20130523 14 25
				break ;
			case EINTR :
				//ACE_DEBUG((LM_DEBUG,"%s:%d EINTR系统调用被阻止!\n",__FILE__,__LINE__));//MOD??:20130523 14 25
				break ;
			case EAGAIN :
				//ACE_DEBUG((LM_DEBUG,"%s:%d EAGAIN资源暂时不能获得!\n",__FILE__,__LINE__));//MOD??:20130523 14 25
				break ;
			case ENOBUFS :
				//ACE_DEBUG((LM_DEBUG,"%s:%d ENOBUFS没有可用的缓存空间!\n",__FILE__,__LINE__));//MOD??:20130523 14 25
				break ;
			case EINVAL :
				//ACE_DEBUG((LM_DEBUG,"%s:%d EINVAL无效的参数!\n",__FILE__,__LINE__));//MOD??:20130523 14 25
				break ;
			default :
				//ACE_DEBUG((LM_DEBUG,"%s:%d errno = %d!\n",__FILE__,__LINE__,erronum));//MOD??:20130523 14 25
				break ;
			
			return false ;

		}

	}

	if ( ulBytes == sizeof(struct can_frame) )
	{
		//ACE_DEBUG((LM_DEBUG,"%s:%d CAN 的帧与ulBytes大小不相等!\n",__FILE__,__LINE__));//MOD??:20130523 14 25
		//这里影响到的can数据的发送
		pMainBoardLed->DoCan0Led();
	}

		//ACE_DEBUG((LM_DEBUG,"%s:%d Send  %d  bytes can_frame !\n",__FILE__,__LINE__,ulBytes));//MOD??:20130523 14 25
	return true;
}

/**************************************************************
Function:       Can::Recv
Description:    接收Can数据包
Input:          recvFrame 接收到到can数据包              
Output:         无
Return:         false-失败  true-成功
***************************************************************/
bool Can::Recv(SCanFrame& recvFrame)
{
	ACE_Guard<ACE_Thread_Mutex>  guard(m_mutexCan);
	Ulong ulBytes = 0;
	Ulong ulLen   = 0;

#ifdef LINUX
	ulBytes = recvfrom(m_socketHandle  , &m_frameCan,sizeof(struct can_frame) , 0  ,(struct sockaddr *)&m_addrCan,(socklen_t*)&ulLen);
	
	//ACE_DEBUG((LM_DEBUG,"%s:%d Recv %d bytes from can_frame! \n",__FILE__,__LINE__,ulBytes));//MOD??:20130523 1422
	recvFrame.ulCanId = m_frameCan.can_id;
	ACE_OS::memcpy(recvFrame.pCanData,m_frameCan.data,m_frameCan.can_dlc);
	recvFrame.ucCanDataLen = m_frameCan.can_dlc;
#endif

	if ( ulBytes == sizeof(recvFrame) )
	{
		pMainBoardLed->DoCan0Led();
	}

	return true;
}



/**************************************************************
Function:       Can::BuildCanId
Description:    建立CanId
Input:          bit28-bit26  bit25-bit20   bit19-bit18 bit17-bit12  bit11-bit4  bit0-bit3
		 		报文类型     模块地址      帧模式      目的地址     保留        协议版本
 			    ucCanMsgType ucModuleAddr ucFrameMode ucRemodeAddr             
Output:         ulCanId  can报文id
Return:         无
***************************************************************/
void Can::BuildCanId(Ulong u1CanMsgType
		          , Ulong u1ModuleAddr
				  , Ulong u1FrameMode
				  , Ulong u1RemodeAddr
				  , Ulong* ulCanId)
{
	Ulong ulCanIdTmp = 0;

	ulCanIdTmp = ulCanIdTmp | B2B_PROTOCOL_V00;  
	ulCanIdTmp = ulCanIdTmp | (0xff << 4);   //保留部分  err	
	ulCanIdTmp = ulCanIdTmp | (u1RemodeAddr << 12);
	ulCanIdTmp = ulCanIdTmp | (u1FrameMode  << 18);
	ulCanIdTmp = ulCanIdTmp | (u1ModuleAddr << 20);
	ulCanIdTmp = ulCanIdTmp | (u1CanMsgType << 26);

	//????????????
	ulCanIdTmp = ulCanIdTmp | (1 << 31);
	*ulCanId = ulCanIdTmp;
}



/**************************************************************
Function:       Can::ExtractCanId
Description:    解析CanId
Output:          bit28-bit26  bit25-bit20   bit19-bit18 bit17-bit12  bit11-bit4  bit0-bit3
		 		报文类型     模块地址      帧模式      目的地址     保留        协议版本
 			    ucCanMsgType ucModuleAddr ucFrameMode ucRemodeAddr             
Input:          ulCanId  can报文id
Return:         无
***************************************************************/
void Can::ExtractCanId(Ulong& u1CanMsgType
		            , Ulong& u1ModuleAddr
				    , Ulong& u1FrameMode
				    , Ulong& u1RemodeAddr
					, Ulong& ulProtocolVersion
				    , Ulong ulCanId)
{
	u1CanMsgType = 0;
	u1ModuleAddr = 0;
	u1FrameMode  = 0;
	u1RemodeAddr = 0;
	ulProtocolVersion = 0;

	ulProtocolVersion = ulCanId & 0x0F;
	u1RemodeAddr      = (ulCanId >> 12) & 0x3F;
	u1FrameMode       = (ulCanId >> 18) & 0x3;
	u1ModuleAddr      = (ulCanId >> 20) & 0x3F;
	u1CanMsgType      = (ulCanId >> 26) & 0x07;
}


/**************************************************************
Function:       Can::PrintInfo
Description:    打印数据
Output:         无              
Input:          file  打印文件名
				line  打印起始行
				iSub  打印结束行
				canFrame 打印数据帧
Return:         无
***************************************************************/
void Can::PrintInfo(char* file,int line,int iSub,SCanFrame canFrame)
{
	ACE_DEBUG((LM_DEBUG,"%s:%d-%d\n",file,line,iSub));
	ACE_DEBUG((LM_DEBUG,"CanId:%08x CanDataLen:%d\nData:",canFrame.ulCanId,canFrame.ucCanDataLen));
	for ( int iPrtIndex=0; iPrtIndex<canFrame.ucCanDataLen; iPrtIndex++)
	{
		ACE_DEBUG((LM_DEBUG,"%02x ",canFrame.pCanData[iPrtIndex]));
	}
	ACE_DEBUG((LM_DEBUG,"\n"));
}


/**************************************************************
Function:       Can::PrintCurTime
Description:    打印当前时间
Output:         无              
Input:          file  打印文件名
				line  打印起始行
				sTmp  打印字符
Return:         无
***************************************************************/
void Can::PrintCurTime(char* file,int line,char* sTmp)
{
	#ifndef WINDOWS	
	struct timeval tvCurTime;
	struct timezone tzCurTime;
	
	gettimeofday (&tvCurTime , &tzCurTime );
	ACE_DEBUG((LM_DEBUG,"%s:%d %s %d %06d(us)\n",file,line,sTmp,tvCurTime.tv_sec,tvCurTime.tv_usec));
	
	#endif
}



/**************************************************************
Function:       Can::RunCanRecv
Description:    Can返回消息接收线程函数，接收数据并放入CAN消息队列
Output:         无              
Input:          arg  默认NULL
Return:         0
***************************************************************/
void * Can::RunCanRecv(void *arg)
{	
	
	SCanFrame sRecvFrameTmp;
	int iLenCanFrame = sizeof(sRecvFrameTmp) ;
	fd_set rfds;
	struct timeval 	tv;
	
	int iCanHandle = Can::CreateInstance()->GetHandle();
	Can* pCan = Can::CreateInstance();
	while(true)
{
	tv.tv_sec  = 0;
	tv.tv_usec = 20000;
	FD_ZERO(&rfds);
	FD_SET(iCanHandle, &rfds);
	
	int iRetCnt = select(iCanHandle + 1, &rfds, NULL, NULL, &tv);
	if ( -1 == iRetCnt )  //select error
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d select error\n",__FILE__,__LINE__));
		continue ;
	}
	else if ( 0 == iRetCnt )  //timeout
	{
		//ACE_DEBUG((LM_DEBUG,"%s:%d timeout\n",__FILE__,__LINE__));
		 continue ;
	}
	else
	{
		Can::CreateInstance()->Recv(sRecvFrameTmp);
		
		ACE_Message_Block *mb = new ACE_Message_Block(iLenCanFrame); //构造消息块
		mb->copy((char*)&sRecvFrameTmp, iLenCanFrame); // 将数据拷贝进消息块

		//ACE_Time_Value nowait(GetCurTime()+ACE_Time_Value(1));
		ACE_Time_Value nowait(GetCurTime()) ;
		if( -1 == (pCan->m_CanMsgQue)->enqueue_tail(mb, &nowait))			//向 CAN ACE_Message_Queue中添加新数据块
		{
			mb->release();
		}
		//pGbtMsgQueue->SendGbtMsg(&sMsg,sizeof(sMsg));
    }

 }
	return 0 ;
}


/**************************************************************
Function:       Can::DealCanData
Description:    从CAN接收信息队列取出接收数据并处理线程函数
Output:         无              
Input:          arg  默认NULL
Return:         0
***************************************************************/
void * Can::DealCanData(void* arg)
{
	int iLen = 0;
	Ulong u1CanMsgType;
	Ulong u1ModuleAddr;
	Ulong u1FrameMode;
	Ulong u1RemodeAddr;
	Ulong ulProtocolVersion;

	timeval tTmp;
	tTmp.tv_sec = 0;
	tTmp.tv_usec = 10 * 1000;//10毫秒

	ACE_Message_Block *mb = NULL;

	SCanFrame sRecvFrameTmp;
	//int iLenCanFrame = sizeof(sRecvFrameTmp) ;
	Can* pCan = Can::CreateInstance();
	CDetector    *pDector  = CDetector::CreateInstance();
	CLampBoard   *pLampBoard = CLampBoard::CreateInstance();
	CFlashMac    *pFlash = CFlashMac::CreateInstance();
	CPowerBoard  *pPower = CPowerBoard::CreateInstance(); //ADD :2013 0712 17 54
	CMacControl  *pMacControl = CMacControl::CreateInstance(); //ADD: 2013 0815 0920
	CWirelessBtn *pWirelessBtn = CWirelessBtn::CreateInstance();  //ADD: 20141022 1123


	ACE_Time_Value nowait(GetCurTime());	
	
	while ( pCan->m_CanMsgQue != NULL )
	{		
		if((pCan->m_CanMsgQue)->dequeue_head(mb, &nowait) != -1) 
		{ 
			iLen = (int)mb->length();
			ACE_OS::memcpy((char*)&sRecvFrameTmp, mb->base(), iLen);  //MOD: add ACE_OS::
			mb->release();			
		}
		else
		{		
			ACE_OS::sleep(ACE_Time_Value(tTmp));  //暂停10毫秒
			continue;			
		}
		Can::CreateInstance()->ExtractCanId(u1CanMsgType  , u1ModuleAddr  , u1FrameMode   , u1RemodeAddr, ulProtocolVersion  , sRecvFrameTmp.ulCanId);
		switch(u1ModuleAddr)
		{
			case BOARD_ADDR_LAMP1:
				pLampBoard->RecvLampCan(BOARD_ADDR_LAMP1, sRecvFrameTmp);
				break;
			case BOARD_ADDR_LAMP2:
				pLampBoard->RecvLampCan(BOARD_ADDR_LAMP2, sRecvFrameTmp);
				break;
			case BOARD_ADDR_LAMP3:
				pLampBoard->RecvLampCan(BOARD_ADDR_LAMP3, sRecvFrameTmp);
				break;
			case BOARD_ADDR_LAMP4:
				pLampBoard->RecvLampCan(BOARD_ADDR_LAMP4, sRecvFrameTmp);
				break;
			case BOARD_ADDR_LAMP5:
				pLampBoard->RecvLampCan(BOARD_ADDR_LAMP5, sRecvFrameTmp);
				break;
			case BOARD_ADDR_POWER:
				pPower->RecvPowerCan(BOARD_ADDR_POWER,sRecvFrameTmp);
				break;
			case BOARD_ADDR_DETECTOR1:
				pDector->RecvDetCan(BOARD_ADDR_DETECTOR1, sRecvFrameTmp);// ADD: 2013 0710 1039	
				break;
			case BOARD_ADDR_DETECTOR2:
				pDector->RecvDetCan(BOARD_ADDR_DETECTOR2, sRecvFrameTmp);// ADD: 2013 0710 1039	
				break;
			case BOARD_ADDR_INTEDET1 :
				break;
			case BOARD_ADDR_INTEDET2 :
				break;
			case BOARD_ADDR_FLASH:
				pFlash->RecvFlashCan(sRecvFrameTmp) ; //ADD: 2013 0712 1543
				break;
			case BOARD_ADDR_HARD_CONTROL:
				pMacControl->RecvMacCan(sRecvFrameTmp);
				break;
			case BOARD_ADDR_LED :
				break ;
			case BOARD_ADDR_WIRELESS_BTNCTRLA:
				pWirelessBtn->RecvMacCan(sRecvFrameTmp); //ADD: 20141022 1125				
				break ;
			case BOARD_ADDR_MAIN: //ADD:20141024 Get Can date from mainboard
			{
				Byte icandatelength = sRecvFrameTmp.ucCanDataLen ;
				ACE_OS::printf("%s:%d ",__FILE__,__LINE__);				
				for(Byte idex = 0 ; idex<icandatelength;idex++)
				{
					ACE_OS::printf(" %2X ",sRecvFrameTmp.pCanData[idex]);
				}
				ACE_OS::printf("\r\n");
				break ;
			}
			default:
				ACE_DEBUG((LM_DEBUG,"\n%s:%d Recv from unknow Module address :%2X !\n",u1ModuleAddr));
				break ;
		}
	}

	return 0 ;
}

	


