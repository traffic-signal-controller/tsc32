#ifndef _MAINBOARDLED_H_
#define _MAINBOARDLED_H_

#include "ComStruct.h"

class CMainBoardLed
{
public:
	static CMainBoardLed* CreateInstance();

	void DoModeLed(bool bLed3Value,bool bLed4Value);
	void DoTscPscLed(bool bValue);
	void DoAutoLed(bool bValue);
	void DoRunLed();
	void DoCan0Led();
	void DoCan1Led();
	bool IsEthLinkUp() ;
	void SetLedBoardShow();//ADD 2013 0809 15 40
	void DoLedBoardShow(); //ADD: 2013 0809 1700
	Byte LedBoardStaus[12]; //ADD 2013 0809 15 40
private:
	CMainBoardLed();
	~CMainBoardLed();
	void OpenDev();
	void CloseDev();
	
private:
	bool can0Bool;
	bool can1Bool;
	
};

#endif /*_MAINBOARDLED_H_*/
