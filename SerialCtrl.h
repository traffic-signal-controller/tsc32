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

	//��GPS��GSM��ͨ��
	int  m_iSerial1fd;
	int  m_iSerial2fd;
	//�뱸��PICͨ�ţ����ݰ������ݵ�ɫ���ֿ�.
	int  m_iSerial3fd;
	int	 m_iSerial4fd;
	int	 m_iSerial5fd;
};

#endif //_SERIALCTRL_H_

