#ifndef _BUSPRIORITY_H_
#define _BUSPRIORITY_H_

#include "ComDef.h"
#include "ComStruct.h"

class CBusPriority
{
public:
	static CBusPriority* CreateInstance();	
	bool b_OpenBusPriority ;
	bool b_HandPriority   ; //�Ƿ��ڹ������ȵ�����
	Byte p_BusDelayTime   ; //Ĭ���̵��ӳ�ʱ��
	Byte p_BusRedLeftTime ; //Ĭ�Ϻ�������ǰʱ��
	Byte p_BusStandardTime ; //Ĭ�Ϲ�������������ͨ��·����Ҫʱ��
	Byte p_BusRedCutType ; //����������
	Byte p_AccessDeviceType ; //�����豸���� 0-��Ȧ 1-��Ƶ 2-�״�3-����
	bool GetBoolOpenBusPriority();	
	void HandBusPriority(SBusPriorityData* pBusPriorityData);
	static void *RunRecevBusPriority(void *arg);
	
private:
	CBusPriority();
	~CBusPriority();
	SBusPriorityData p_CurrentRequestBusData ;
};

#endif 

