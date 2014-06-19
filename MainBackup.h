#ifndef MAINBACKUP_H_
#define MAINBACKUP_H_

#include "ComDef.h"
#include "SerialCtrl.h"
#include "IoOperate.h"
#include "Cdkey.h"
#include "ComStruct.h"

enum
{
	MAINBACKUP_NONE 			= 	0 ,  		//无意义
	MAINBACKUP_READ_ID 		= 	1 ,  		//核心板读取设备ID串号  备用
	MAINBACKUP_WRITE_ID		=	2,			//核心板写入设备ID串号
	MAINBACKUP_READ_MANUAL	=	3,			//核心板读取手控状态
	MAINBACKUP_READ_LED		=	4,			//核心板读取指示灯状态
	MAINBACKUP_WRITE_LED		=	5,			//核心板写入指示灯状态
	MAINBACKUP_HEART			=	6,			//此命令每500ms发送一次，作为核心板心跳包。
	MAINBACKUP_LAMP			=	7			//核心板下发阶段步伐表到备份单片机，最多有64步伐。参数表示的是步伐。注意组合灯色数据和解析灯色数据！

};

enum
{
	MAINBACKUP_RECEVIE_NONE 			= 	0 ,  		//无意义
	MAINBACKUP_RECEVIE_ID 			= 	1 ,  		//核心板读取设备ID串号  备用
	MAINBACKUP_RECEVIE_WRITE_ID_TF	=	2,			//核心板写入设备ID串号  成功失败   8个字节
	MAINBACKUP_RECEVIE_READ_MANUAL	=	3,			//核心板读取手控状态
	MAINBACKUP_RECEVIE_READ_LED		=	4,			//返回指示灯的状态  1个字节
	MAINBACKUP_RECEVIE_WRITE_LED_TF	=	5,			//指示灯状态写入成功或失败 ，1个字节
	MAINBACKUP_RECEVIE_SETP			=	6,			//单片机当前步伐号
	MAINBACKUP_RECEVIE_SETP_TABLE	=	7			//单片机确认收到的阶段步伐表 13个字节

};


enum
{
	MAINBACKUP_LED_MODE_GREEN	= 0,
	MAINBACKUP_LED_MODE_YELLOW	=1,
	MAINBACKUP_LED_MODE_RED		=2
	
};

enum 
{
	MAINBACKUP_TSCP_TSC = 0,
	MAINBACKUP_TSCP_PSC = 1
};

enum 
{
	MAINBACKUP_AUTO_SELF = 0,
	MAINBACKUP_AUTO_MANUAL = 1
};

enum
{
	MAINBACKUP_MANUAL_SELF 			= 0,	//自主运行
	MAINBACKUP_MANUAL_MANUAL 		= 1,	//手动运行
	MAINBACKUP_MANUAL_NEXT_STEP		=3,		//下一步
	MAINBACKUP_MANUAL_YELLOW_FLASH	=5,		//黄闪
	MAINBACKUP_MANUAL_ALL_RED		=9,		//全红
	MAINBACKUP_MANUAL_NEXT_PHASE	=17,	//下一相位
	MAINBACKUP_MANUAL_NEXT_DIREC	=33,	//下一方向
	MAINBACKUP_MANUAL_NORTH			=33024,	//北
	MAINBACKUP_MANUAL_EAST			=33280,	//东
	MAINBACKUP_MANUAL_SOUTH			=33792,	//南
	MAINBACKUP_MANUAL_WEST			=34816	//西
	
};

enum
{
	LAMP_OFF_ALL			=0,		//关灯
	LAMP_RED			=1,		//红灯
	LAMP_YELLOW		=2,//黄灯
	LAMP_GREEN			=3,//绿灯
	LAMP_RED_FLASH	=4,//红闪
	LAMP_YELLOW_FLASH	=5,//黄闪
	LAMP_GREEN_FLASH	=6,//绿闪
	LAMP_OTHER			=7//其它，预留
};

class MainBackup 
{
public:
	MainBackup();
	virtual ~MainBackup();
	static MainBackup* CreateInstance();
	void OpenDev();
	bool SendBackup(Byte *pByte ,int iSize);
	bool RecevieBackup(Byte *pByte ,int iSize);
	void OperateBackup(Byte *pbytes, Uint pLen,Byte *rsBytes, Uint rsLen);
	void OperateManual(Ushort mbs);
	void DoManual();
	static void* Recevie(void* arg);
	void HeartBeat();
	void ReadLED(Byte &status);
	void ReadID(Byte *pByte);
	void WriteIDTF(Byte *pByte);
	void WriteLEDTF(Byte &writeLED);
	void CurrentSetp(Byte &setp);
	void SetpTable(Byte *setpTable);
	void DoReadId();
	void DoWriteId();
	void DoReadLED();
	void DoWriteLED();
	void DoSendStep(SStepInfo stepInfos[],Byte stepNum);
private:
	ACE_Thread_Mutex  m_sMutex;
	int m_iSerial3fd;
	Byte m_ucLastManualSts;  //之前的手动状态
};


#endif /* MAINBACKUP_H_ */

