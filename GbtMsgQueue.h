#ifndef _GBTMSGQUEUE_H_
#define _GBTMSGQUEUE_H_

#include "ace/Message_Queue.h"
#include "ace/Synch.h"
#include "ace/SOCK_Dgram.h"
#include "ace/INET_Addr.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/SOCK_Stream.h"
#include "ComDef.h"
#include "ComStruct.h"

/*****************GBTÐ­Òé¶ÔÏó±êÖ¾ÀàÐÍºÍÀ©³äÐ­Òé±êÖ¾ÀàÐÍÄÇ¸öÃ¶¾Ù********************/
enum 
{
	//¹«¹²ÅäÖÃÏà¹Ø¶ÔÏó
	OBJECT_DEV_PARA     = 0x81,  //¹«¹²Éè±¸Ê¶±ð²ÎÊý
	OBJECT_MAX_MODULE         ,  //¹«¹²Ä£¿é±í×î´óÐÐÊý
	OBJECT_COM_SYN_TIME       ,  //¹«¹²Í¬²½Ê±¼ä
	OBJECT_SYN_FLAG           ,  //¹«¹²Í¬²½±êÖ¾
	OBJECT_MODULE_TABLE       ,  //Ä£¿é±í

	//¹«¹²Ê±¼ä²ÎÊý
	OBJECT_UTC_TIME     = 0x86,  //¹«¹²Ê±¼ä
	OBJECT_STANDAR_TIMEZONE   ,  //±ê×¼Ê±Çø
	OBJECT_LOCAL_TIME         ,  //±¾µØÊ±¼ä
	OBJECT_MAX_TIMEGROUP      ,  //Ê±»ùµ÷¶È±í×î´óÐÐÊý
	OBJECT_MAX_SCHEDULE       ,  //Ê±¶Î±í×î´óÐÐÊý
	OBJECT_MAX_SUBSCHEDULE    ,  //Ê±¶Î±íµÄ×î´óÊ±¶ÎÊý
	OBJECT_ACTIVESCHEDULE_NO  ,  //»î¶¯Ê±¶Î±íµÄ±àºÅ
	OBJECT_TIMEGROUP_TABLE    ,  //Ê±»ùµ÷¶È±í
	OBJECT_SCHEDULE_TABLE     ,  //Ê±¶Î±í

	//ÉÏ±¨²ÎÊý
	OBJECT_MAX_EVENTTYPE    = 0x8F,  //ÊÂ¼þÀàÐÍ±í×î´óÐÐÊý
	OBJECT_MAX_EVENTLOG           ,  //ÊÂ¼þÈÕÖ¾±íµÄ×î´óÐÐÊý
	OBJECT_EVENTTYPE_TABLE        ,  //ÊÂ¼þÀàÐÍ±í²ÎÊý
	OBJECT_EVENTLOG_TEBLE         ,  //ÊÂ¼þÈÕÖ¾±í²ÎÊý

	//ÏàÎ»²ÎÊý
	OBJECT_MAX_PHASE      = 0x93  ,  //ÏàÎ»±í×î´óÐÐÊý 
	OBJECT_MAX_PHASESTS           ,  //ÏàÎ»×´Ì¬×é×î´óÊýÁ¿
	OBJECT_PHASE_TABLE            ,  //ÏàÎ»²ÎÊý±í
	OBJECT_PHASESTATUS_TABLE      ,  //ÏàÎ»Êä³ö×´Ì¬±í
	OBJECT_PHASECONFLICT_TABLE    ,  //ÏàÎ»³åÍ»±í

	//¼ì²âÆ÷²ÎÊý
	OBJECT_MAX_DETECTOR    = 0x98,  //³µÁ¾¼ì²âÆ÷×î´óÊýÁ¿
	OBJECT_MAX_DETECTORSTS       ,  //¼ì²âÆ÷×´Ì¬×éµÄ×î´óÊýÁ¿
	OBJECT_DETECTORDATA_ID       ,  //¼à²âÊý¾ÝÁ÷Ë®ºÅ
	OBJECT_DATA_CYCLE            ,  //Êý¾Ý²É¼¯ÖÜÆÚ
	OBJECT_ACTIVEDETECTOR_NUM    ,  //»î¶¯¼ì²âÆ÷×ÜÊý
	OBJECT_PULSEDATA_ID          ,  //Âö³åÊý¾ÝÁ÷Ë®ºÅ
	OBJECT_PULSEDATA_GETCYCLE    ,  //Âö³åÊý¾Ý²É¼¯ÖÜÆÚ
	OBJECT_DETECTORDEF_TABLE     ,  //³µÁ¾¼ì²âÆ÷²ÎÊý¶¨Òå±í
	OBJECT_DETECTORSTS_TABLE     ,  //¼ì²âÆ÷×´Ì¬±í
	OBJECT_DETECTORDATA_TABLE    ,  //½»Í¨¼ì²âÊý¾Ý±í
	OBJECT_DETECTORWARN_TABLE    ,  //³µÁ¾¼ì²âÆ÷¾¯¸æ²ÎÊý±í

	//µ¥Ôª²ÎÊý
	OBJECT_START_FLASH  = 0xA3   ,  //Æô¶¯Ê±µÄÉÁ¹â¿ØÖÆÊ±¼ä
	OBJECT_START_ALLRED          ,  //Æô¶¯Ê±µÄÈ«ºì¿ØÖÆÊ±¼ä
	OBJECT_CURTSC_CTRL           ,  //µ±Ç°µÄÐÅºÅ»ú¿ØÖÆ×´Ì¬
	OBJECT_CURTSC_FLASHCTRL      ,  //µ±Ç°µÄÉÁ¹â¿ØÖÆÄ£Ê½
	OBJECT_TSC_WARN2             ,  //ÐÅºÅ»ú±¨¾¯2
	OBJECT_TSC_WARN1             ,  //ÐÅºÅ»ú±¨¾¯1
	OBJECT_TSC_WARN_SUMMARY      ,  //ÐÅºÅ»ú±¨¾¯ÕªÒª
	OBJECT_ALLOW_FUN             ,  //ÔÊÐíÔ¶³Ì¿ØÖÆÊµÌå¼¤»îÐÅºÅ»úµÄÄ³Ð©¹¦ÄÜ
	OBJECT_FLASH_FRE             ,  //ÉÁ¹âÆµÂÊ
	OBJECT_SHINE_STARTTIME       ,  //»Ô¶È¿ØÖÆ¿ªÆôÊ±¼ä
	OBJECT_SHINE_DOWM_TIME       ,  //»Ô¶È¿ØÖÆ¹Ø±ÕÊ±¼ä

	//µÆ¿Ø¶Ë¿Ú²ÎÊý
	OBJECT_MAX_CHANNEL     = 0xAE,  //ÐÅºÅ»úÖ§³ÖµÄ×î´óÍ¨µÀÊýÁ¿
	OBJECT_MAX_CHANNELSTS        ,  //Í¨µÀ×´Ì¬×éÊý
	OBJECT_CHANNEL_TABLE         ,  //Í¨µÀ²ÎÊý±í
	OBJECT_CHANNELSTATUS_TABLE   ,  //Í¨µÀÊä³ö×´Ì¬±í

	//¿ØÖÆ²ÎÊý
	OBJECT_TIMEPATTERN_NUM = 0xB2,  //ÅäÊ±·½°¸Êý
	OBJECT_MAX_SCHEDULETIME      ,  //×î´ó½×¶ÎÅäÊ±±íÊý
	OBJECT_MAX_SUBSCHEDULETIME   ,  //×î´ó½×¶ÎÊý
	OBJECT_SWITCH_MANUALCONTROL  ,  //ÊÖ¶¯¿ØÖÆ·½°¸
	OBJECT_SWITCH_SYSTEMCONTROL  ,  //ÏµÍ³¿ØÖÆ·½°¸
	OBJECT_SWITCH_CONTROL        ,  //¿ØÖÆ·½Ê½
	OBJECT_COMCYCLETIME          ,  //¹«¹²ÖÜÆÚÊ±³¤
	OBJECT_ADJUST_PHASEGAP       ,  //Ð­µ÷ÏàÎ»²î
	OBJECT_SWITCH_STAGE          ,  //½×¶Î×´Ì¬
	OBJECT_GOSTEP                ,  //²½½øÖ¸Áî
	OBJECT_DEMOTION_MODE         ,  //½µ¼¶Ä£Ê½
	OBJECT_DEMOTION_PATTERN      ,  //½µ¼¶»ù×¼·½°¸±í
	OBJECT_CURPATTERN_SCHTIMES   ,  //µ±Ç°·½°¸¸÷½×¶ÎÊ±³¤
	OBJECT_CURPATTERN_GREENTIMES ,  //µ±Ç°·½°¸¸÷¹Ø¼üÏàÎ»ÂÌµÆÊ±³¤
	OBJECT_TIMEPATTERN_TABLE     ,  //ÅäÊ±·½°¸±í
	OBJECT_SCHEDULETIME_TABLE    ,  //½×¶ÎÅäÊ±±í

	//ÏÂÔØ±êÖ¾²ÎÊý
	OBJECT_DOWNLOAD_FLAG   = 0xC2,  //ÉÏÎ»»úÏÂÔØ²ÎÊýµÄ¿ªÊ¼½áÊø±êÖ¾

	//¿ØÖÆÖ÷»ú²ÎÊý
	OBJECT_CONTROLTSC_PARA = 0xC3,  //¿ØÖÆÖ÷»úÑ¡Ïî²ÎÊý
	OBJECT_TSCADDRESS            ,  //ÐÅºÅ»ú»ùµØÖ·
	OBJECT_CROSSNUM              ,  //Â·¿ÚÊýÁ¿

	//¸úËæÏàÎ»²ÎÊý
	OBJECT_MAX_OVERLAPPHASE = 0xC6,  //¸úËæÏàÎ»±í×î´óÐÐÊý
	OBJECT_MAX_OVERLAPPHASESTS    ,  //¸úËæÏàÎ»×´Ì¬±í×î´óÐÐÊý
	OBJECT_OVERLAPPHASE_TABLE     ,  //¸úËæÏàÎ»±í
	OBJECT_OVERLAPPHASE_STATUS    ,  //¸úËæÏàÎ»×´Ì¬±í

	//À©³ä¶ÔÏó±êÖ¾
    OBJECT_WATCH_PARA       = 0xf5,   //¼à¿ØÀàÐÍ²ÎÊý ÎÂ¶È µçÑ¹ ÃÅ
	OBJECT_IP               = 0xf6,   //ÐÅºÅ»úµÄip
	OBJECT_SET_REPORTSELF   = 0xf7,   //¶¨ÖÆÖ÷¶¯ÉÏ±¨
	OBJECT_EXT_TSC_STATUS   = 0xf8,   //×´Ì¬ÀàÐÍ±í²ÎÊý
	OBJECT_MODULE_STATUS    = 0xf9 ,   //Ä£¿é×´Ì¬
	OBJECT_CNTDOWN_DEV      = 0xf1,   //µ¹¼ÆÊ±Éè±¸±í    ADD:2013071 1034
	OBJECT_PHASETODIRECT    = 0xfa,   //ÏàÎ»Óë·½Ïò¶ÔÓ¦±í
	OBJECT_ADAPTPARA        = 0xfb,   //×ÔÊÊÓ¦²ÎÊýÖµ
	OBJECT_DET_EXTENTED     = 0xfc,   //¼ì²âÆ÷À©Õ¹±í
	OBJECT_ADAPT_STAGE      = 0xfd,
	OBJECT_CONNECT_DEV      = 0xf0,   //Íâ½çÉè±¸ÊýµÚÒ»¸ö×Ö½Ú±íÊ¾µ¹¼ÆÊ±Éè±¸Êý£¬È¡ÖµÎª0-32£¬0±íÊ¾Ã»ÓÐµ¹¼ÆÊ±Éè±¸¡£µÚ¶þ¸ö×Ö½Ú±íÊ¾¿É±ä±êÖ¾Éè±¸Êý£¬È¡ÖµÎª0-16£¬0±íÊ¾Ã»ÓÐ¿É±ä±êÖ¾Éè±íÊ¾Íâ½Ó¼ì²âÆ÷Êý£¬È¡ÖµÎª0-48£¬0±íÊ¾Ã»ÓÐÍâ½Ó¼ì²âÆ÷
	//OBJECT_CNTDOWN_STATS   = 0xf2,   //µ¹¼ÆÊ±×´Ì¬±í
	OBJECT_COMMAND_SIGNAL   = 0xf2 ,    //ÉÏÎ»»úÖ¸Áî¿ØÖÆÏàÎ»½×¶ÎÇÐ»»ºÍ·½Ïò
	OBJECT_CHANNEL_CHK      = 0xff,   //Í¨µÀµÆÅÝ¼ì²âÅäÖÃ±í ADD?20130801 1121
	OBJECT_YWFLASH_CFG      = 0xe1,   //»ÆÉÁÆ÷ÅäÖÃ
	OBJECT_DET_EXTCFG       = 0xe2,   //¼ì²âÆ÷À©Õ¹ÅäÖÃ
	OBJECT_LAMPBOARD_CFG    = 0xe3,   //µÆ¿Ø°åµÆÅÝ¼ì²âºÍºìÂÌ³åÍ»¼ì²âÅäÖÃ
	OBJECT_PSCBTN_NUM 	    = 0xee,    //Ä£Äâ8Î»ÐÐÈË°´Å ¥ÊäÈë ADD:2013 0829 1540
	OBJECT_TMPPATTERN_CFG   = 0xef,    //12·½ÏòÁÙÊ±×éºÏ·½°¸£¬Ä¬ÈÏ60Ãë
	OBJECT_SYSFUNC_CFG      = 0xe4,      //ÏµÍ³ÆäËû¹¦ÄÜÉèÖÃ
	OBJECT_SENDCLIENT_CNTDOWN = 0xe6 ,
	OBJECT_POWERBOARD_CFG     = 0xe7,   //µçÔ´°åÅäÖÃ
	OBJECT_GSM_CFG            = 0xe8 ,  //GSMÅäÖÃ
	OBJECT_BUTTONPHASE_CFG    = 0xe9 ,   //Ä£ÄâÎÞÏß°´¼ü°´Å¥
	OBJECT_BUSPRIORITY_CFG    = 0xea     //¹«½»ÓÅÏÈÅäÖÃ
};
/*****************GBTÐ­Òé¶ÔÏó±êÖ¾ÀàÐÍºÍÀ©³äÐ­Òé±êÖ¾ÀàÐÍÄÇ¸öÃ¶¾Ù********************/

class CGbtMsgQueue
{
public:
	static CGbtMsgQueue* CreateInstance();
	static void* RunGbtRecv(void* arg);	
	int  SendGbtMsg(SThreadMsg* pMsg,int iLen);
	void DealData();	
#ifdef GBT_TCP
	SGbtDealData* GetGbtDealDataPoint();
	Byte GetDealDataIndex(bool bReportSelf , ACE_SOCK_Stream& sockStreamRemote);
#else
	Byte GetDealDataIndex(bool bReportSelf ,ACE_INET_Addr& addrRemote);
#endif
	bool SendTscCommand(Byte ucObjType,Byte ucValue);
	void SetCmuAndCtrl(Byte* pBuf,int& iRecvIndex);
	void SetCmuAndCtrl(Byte* pBuf,int& iRecvIndex , Byte ucSubId);
	void GetCmuAndCtrl(Byte* pBuf,int& iSendIndex);
	void GetCmuAndCtrl(Byte* pBuf,int& iSendIndex , Byte ucSubId);
	void ReworkNetPara(Byte* cIp ,Byte* cMask,Byte* cGateWay);
	void GetNetPara(Byte* pIp , Byte* pMask , Byte* pGateway);
	void InterceptStr(char* pBuf, char* pstr , Byte* pData , Byte ucCnt);
	void GetNetParaByAce(Byte* pip,char* phost_name);  //add 2014 03 20   lurenhua Í¨¹ýACEÀ´È¡µÃIPµØÖ·£¬¿ÉÒÔÈ¡µÃ¶à¸ö
	
	bool GetSystemShellRst(const char* shellcmd , char * cshellrst ,Byte datasize); //ADD:201411041430
public:
	ACE_INET_Addr m_addrLocal;
#ifdef GBT_TCP
	ACE_SOCK_Acceptor m_acceptor;   //tcp
#else
	ACE_SOCK_Dgram m_sockLocal;  //udp
#endif
	int iPort   ;               //MOD:201309250830
private:
	CGbtMsgQueue();
	~CGbtMsgQueue();

	int  CheckMsg(Byte ucDealDataIndex,Uint iBufLen,Byte* pBuf);
	void FirstRecv(Byte ucDealDataIndex,Uint iBufLen,Byte* pBuf);
	Byte GetSendOperateType(Byte ucRecvOptType);
	void DealRecvBuf(Byte ucDealDataIndex);
 	void CleanDealData(Byte ucDealDataIndex);
 	void SendToHost(Byte ucDealDataIndex);
  	bool IsSingleObject(Byte ucObjectFlag);
  	bool IsGetTscStatusObject(Byte ucObjectFlag);
	bool IsSendTscCommand(Byte ucObjectFlag);
	bool IsOtherObject(Byte ucObjectFlag);
	bool IsExtendObject(Byte ucObjectFlag);
	//bool IsDoubleIndexObject(Byte ucObjectFlag);
 	void PackTscStatus(Byte ucDealDataIndex,void* pValue);
	void PackOtherObject(Byte ucDealDataIndex);
	void PackExtendObject(Byte ucDealDataIndex);
	Byte ToObjectCurTscCtrl(unsigned int uiCtrl);
 	Byte ToObjectControlSwitch(unsigned int uiWorkSts,unsigned int uiCtrl);
 	
 	void SelfReport(unsigned int uiDataLen,Byte* pDataBuf);
	int GbtCtrl2TscCtrl(Byte ucctrl);
	int GetCtrlStatus(unsigned int uiWorkStatus,unsigned int uiCtrl);
	int GetSysCtrlStatus(unsigned int uiWorkStatus,unsigned int uiCtrl);
	int GetManualCtrlStatus(unsigned int uiWorkStatus,unsigned int uiCtrl);

	void TscCopyFile(char* fpSrc, char* fpDest);
	void UpdateNetPara(Byte* pIp);  //add: 2014 03 20   lurenhua zlg m3352ºËÐÄ°åIPµØÖ·ÐÞ¸Äº¯Êý
	
	void GetWatchPara(Byte* pBuf,int *iSendIndex);	
	void GetModuleStatus(Byte* pBuf,int *iSendIndex ,Byte subId,Byte ucQueryType,Byte ucBdindex);
	void PackTscExStatus(Byte ucDealDataIndex,void* pValue);
	void GetFlashCfg(Byte* pBuf,int *iSendIndex) ;  //ADD: 2013 0808 10 30
	void SetFlashCtrl(Byte* pBuf,int& iRecvIndex); //ADD 2013 0808 1430
	void  GetPowerCfg(Byte* pBuf,int *iSendIndex ,Byte ucQueryType); //ADD:201404021520	
	void  SetPowerCfg(Byte* pBuf,int& iRecvIndex) ;//ADD:201404021520

	void GetDetCfg(Byte* pBuf,Byte ucBdIndex,Byte ucQueryType,int *iSendIndex) ; //ADD 20130821 1130
	void SetDetCfg(Byte* pBuf,int& iRecvIndex) ; //ADD 2013 0821 1624
	void SetLampBdtCfg(Byte* pBuf,int& iRecvIndex);  //ADD 2013 0822 1606
	void SetPscNum(Byte* pBuf,int& iRecvIndex);  //ADD 20130829 1600
	void SetTmpPattern(Byte* pBuf,int& iRecvIndex) ; //ADD 20131016 1700
	void SetSysFunc(Byte* pBuf,int& iRecvIndex); //ADD 20131019 1400
	void PrintIpList();

	void GotoMsgError(Byte ucDealDataIndex,Byte ucErrorSts,Byte ucErrorIdx);
	void GotoSendToHost(Byte ucDealDataIndex);
	void GotoDealRecvbuf(Byte ucDealDataIndex);
	void SetSmsFunc(Byte* pBuf,int& iRecvIndex ,int iRecvBufLen); //ADD 201406041030
	void SetCommandSignal(Byte* pBuf,int& iRecvIndex) ;  //ADD 201409231002
	void SetButtonPhase(Byte* pBuf,int& iRecvIndex);        //ADD 201410181052
    void SetBusPriorityCfg(Byte* pBuf,int& iRecvIndex);  //ADD 20150615
    void GetBusPriorityCfg(Byte* pBuf,int *iSendIndex) ;  //ADD: 2015 0615 10 30
	//Byte m_ucAddrNum;
	ACE_Message_Queue<ACE_MT_SYNCH>* m_pMsgQue;
      
	ACE_Thread_Mutex  m_sMutex;
	//ACE_INET_Addr m_AddrArr[MAX_CLIENT_NUM];
	SGbtDealData  m_sGbtDealData[MAX_CLIENT_NUM];

	int m_iManualCtrlNo;
	int m_iSystemCtrlNo;
};

#endif  //_GBTMSGQUEUE_H_

