#ifndef _SERIALCTRL_H_
#define _SERIALCTRL_H_

#include "Define.h"

typedef int     INT32;
typedef short   INT16;
typedef char    INT8;
typedef unsigned int UNIT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;

class CSerialCtrl
{
public:
	static CSerialCtrl* CreateInstance();

	void OpenALLSerial();
	int GetSerialFd1();
	int GetSerialFd2();
	int GetSerialFd3();
	int GetSerialFd4();
	int GetSerialFd5();

	INT32 OpenComPort (INT32 ComPort, INT32 baudrate, INT32 databit,const char *stopbit, char parity);
	void CloseComPort (void);
	INT32 ReadComPort (Byte *data, INT32 datalength);
	INT32 WriteComPort (Byte * data, INT32 datalength);
	/** 
	* export serial fd to other program to perform 
	* directly read, write to serial. 
	*  
	* @return serial's file description  */
		int getPortFd();
private:
	CSerialCtrl();
	~CSerialCtrl();

	//与GPS、GSM板通信
	int  m_iSerial1fd;
	int  m_iSerial2fd;
	//与备份PIC通信，内容包括备份灯色，手控.
	int  m_iSerial3fd;
	int	 m_iSerial4fd;
	int	 m_iSerial5fd;
};

#endif //_SERIALCTRL_H_

