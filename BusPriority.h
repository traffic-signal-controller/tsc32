#ifndef _BUSPRIORITY_H_
#define _BUSPRIORITY_H_

#include "ComDef.h"
#include "ComStruct.h"

class CBusPriority
{
public:
	static CBusPriority* CreateInstance();	
	bool b_OpenBusPriority ;
	bool b_HandPriority   ; //是否处于公交优先调控中
	Byte p_BusDelayTime   ; //默认绿灯延长时间
	Byte p_BusRedLeftTime ; //默认红灯早断提前时间
	Byte p_BusStandardTime ; //默认公交车到达监测点后通过路口需要时间
	Byte p_BusRedCutType ; //红灯早断类型
	Byte p_AccessDeviceType ; //接入设备类型 0-线圈 1-射频 2-雷达3-其他
	bool GetBoolOpenBusPriority();	
	void HandBusPriority(SBusPriorityData* pBusPriorityData);
	static void *RunRecevBusPriority(void *arg);
	
private:
	CBusPriority();
	~CBusPriority();
	SBusPriorityData p_CurrentRequestBusData ;
};

#endif 

