#ifndef _SERIALCTRL_H_
#define _SERIALCTRL_H_

class CSerialCtrl
{
public:
	static CSerialCtrl* CreateInstance();
	void OpenSerial(short iSerNum);
	int  GetSerialFd(short iSerNum);

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

