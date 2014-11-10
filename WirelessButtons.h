
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
	Byte m_ucLastManualSts;  //֮ǰ���ֶ�״̬	
	Byte m_ucdirectype ;       //��һ���򱣴���� ȡֵ��Χ0-3 ����������
	SThreadMsg sTscMsg;      //�߳���Ϣ����
	CManaKernel * pManaKernel ;
	bool bFirstOperate ;
	bool bTransitSetp ;
	Uint RecvoldDirec ;
	Byte m_ucAllowLampStatus ; //��ǰ������λ��ɫ״̬
	
	
	
};

class CWirelessBtnTimer : public ACE_Event_Handler
{
public: 
	CWirelessBtnTimer();
	~CWirelessBtnTimer();
	static CWirelessBtnTimer* CreateInstance();
	virtual int handle_timeout(const ACE_Time_Value &tCurrentTime, const void * = 0); //��ʱ���ص�����
	void Resettimeout();  //�������߰����ֿس�ʱ����
private:
	Uint m_uctimeout ;
};

#endif /* MANUAL_H_ */
