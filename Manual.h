
#ifndef MANUAL_H_
#define MANUAL_H_

#include "ComDef.h"

#define DEV_MANUAL_KEYINT "/dev/tiny6410_buttons"  //MOD:0517 14:59
						 
class Manual 
{
public:
	Manual();
	virtual ~Manual();
	static Manual* CreateInstance();
	void DoManual();
	void SetPanelStaus(Byte ucStatus);
private:
	Byte m_ucManualSts;  //手控状态
	Byte m_ucManual;     //0自动运行 1手动控制
	Byte m_ucRedFlash ; //手动全红控制或者黄闪控制 1-全红或黄?0-全红黄闪归位
	Byte m_ucLastManual;     //之前是否为手动的
	Byte m_ucLastManualSts;  //之前的手动状态
	void OpenDev();
	int m_iManualFd;
	Byte ManualKey ;  //当前处于手控时，选择的手控操作按钮

};

#endif /* MANUAL_H_ */
