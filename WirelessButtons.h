
#ifndef WIRELESSBUTTONS_H_
#define WIRELESSBUTTONS_H_

#include "ComDef.h"	
#include "ManaKernel.h"

class CWirelessBtn 
{
public:
	CWirelessBtn();
	virtual ~CWirelessBtn();
	static CWirelessBtn* CreateInstance();	
	void RecvMacCan(SCanFrame sRecvCanTmp);  //ADD:201410211450
	void BackToAuto();
	void BackToAutoByManual();
	void EntryWirelessManul();
	void HandleSpecialDirec(Uint *RecvDircData); //ADD:20141105
	bool SetCurrenStepFlash(Byte LampColor); //ADD:201411071644
	
private:	
	Byte m_ucLastManualSts;  //之前的手动状态	
	Byte m_ucdirectype ;       //下一方向保存变量 取值范围0-3 ，北东南西
	SThreadMsg sTscMsg;      //线程消息变量
	CManaKernel * pManaKernel ;
	bool bFirstOperate ;
	bool bTransitSetp ;
	Uint RecvoldDirec ;
	Byte m_ucAllowLampStatus ; //当前方向相位灯色状态
	
	
	
};

class CWirelessBtnTimer : public ACE_Event_Handler
{
public: 
	CWirelessBtnTimer();
	~CWirelessBtnTimer();
	static CWirelessBtnTimer* CreateInstance();
	virtual int handle_timeout(const ACE_Time_Value &tCurrentTime, const void * = 0); //定时器回调函数
	void Resettimeout();  //重置无线按键手控超时计数
private:
	Uint m_uctimeout ;
};

#endif /* MANUAL_H_ */
