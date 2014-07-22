/***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   SerialCtrl.cpp
Date:       2013-9-13
Description:���ڴ��������
Version:    V1.0
History:    
***************************************************************/
#include "SerialCtrl.h"
#include "ace/Synch.h"
#ifndef WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#endif

#define SERIAL0  "/dev/ttySAC0"    //����0
#define SERIAL1  "/dev/ttySAC1"    //����1
#define SERIAL2  "/dev/ttySAC2"    //����2
#define SERIAL3  "/dev/ttySAC3"    //����3

#define SERIALNUM1  1
#define SERIALNUM2  2
#define SERIALNUM3  3
#define setbitrate(x)  Bx
/**************************************************************
Function:       CSerialCtrl::CSerialCtrl
Description:    CSerialCtrl�๹�캯�����������ʼ������				
Input:          ��              
Output:         ��
Return:         ��
***************************************************************/
CSerialCtrl::CSerialCtrl()
{
	m_iSerial1fd = -1;
	m_iSerial2fd = -1;
	m_iSerial3fd = -1;
	ACE_DEBUG((LM_DEBUG,"%s:%d Init SerialCom object ok !\n",__FILE__,__LINE__));
}

/**************************************************************
Function:       CSerialCtrl::~CSerialCtrl
Description:    CSerialCtrl����������	
Input:          ��              
Output:         ��
Return:         ��
***************************************************************/
CSerialCtrl::~CSerialCtrl()
{
	#ifndef WINDOWS
	if( m_iSerial2fd > 0 )
	{
		close(m_iSerial2fd);
	}
	else if( m_iSerial1fd > 0 )
	{
		close(m_iSerial1fd);
	}
	else if( m_iSerial3fd > 0 )
	{
		close(m_iSerial3fd);
	}
	#endif
	ACE_DEBUG((LM_DEBUG,"%s:%d Destruct SerialCom object ok !\n",__FILE__,__LINE__));
}
	

/**************************************************************
Function:       CSerialCtrl::CreateInstance
Description:    ����	CSerialCtrl��̬����
Input:          ��              
Output:         ��
Return:         ��̬����ָ��
***************************************************************/
CSerialCtrl* CSerialCtrl::CreateInstance()
{
	static CSerialCtrl cSerialOperate;

	return &cSerialOperate;
}

/**************************************************************
Function:       CSerialCtrl::OpenSerial
Description:    �򿪴����豸�ļ�
Input:          ��              
Output:         ���ð�ť�ļ����
Return:         ��
***************************************************************/
void CSerialCtrl::OpenSerial(short iSerNum)
{
	int iTmpSerFd = -1 ;
#ifndef WINDOWS
	struct termios terminfo;
	if(iSerNum == SERIALNUM1)
	{
		m_iSerial1fd = open(SERIAL1, O_RDWR | O_NOCTTY | O_NONBLOCK);
		iTmpSerFd = m_iSerial1fd ;
	}
	else if(iSerNum == SERIALNUM2)
	{
		m_iSerial2fd = open(SERIAL2, O_RDWR | O_NOCTTY | O_NONBLOCK);
		iTmpSerFd = m_iSerial2fd ;
	}
	else if(iSerNum == SERIALNUM3)
	{
		m_iSerial3fd = open(SERIAL3, O_RDWR | O_NOCTTY | O_NONBLOCK);
		iTmpSerFd = m_iSerial3fd ;
	}
	
	cfmakeraw(&terminfo);

	terminfo.c_iflag |= IGNBRK | IGNPAR | IXANY;
	terminfo.c_cflag = CREAD | CS8 | CLOCAL | B38400;
	if (tcsetattr(iTmpSerFd, TCSANOW, &terminfo) == -1)
	{
		ACE_DEBUG((LM_DEBUG,"tcsetattr error\n"));
		close(iTmpSerFd);
		if(iSerNum == SERIALNUM1)
		{
			m_iSerial1fd = -1 ;
		}
		else if(iSerNum == SERIALNUM2)
		{
			m_iSerial2fd = -1 ;		
		}
		else if(iSerNum == SERIALNUM3)
		{
			m_iSerial3fd = -1 ;		
		}
	}
#endif
}

/**************************************************************
Function:       CSerialCtrl::GetSerialFd
Description:    ��ȡ�����豸���
Input:          ��              
Output:         ��
Return:         �����豸���
***************************************************************/
int CSerialCtrl::GetSerialFd(short iSerNum)
{
	int iTmpSerFd = -1 ;
	if(iSerNum == SERIALNUM1)
	{
		iTmpSerFd = m_iSerial1fd  ;
	}
	else if(iSerNum == SERIALNUM2)
	{
		iTmpSerFd = m_iSerial2fd  ;	
	}
	else if(iSerNum == SERIALNUM3)
	{
		iTmpSerFd = m_iSerial3fd  ;	
	}
	if ( iTmpSerFd < 0 )
	{
#ifndef WINDOWS
		close(iTmpSerFd);
#endif
		OpenSerial(iSerNum);
		if(iSerNum == SERIALNUM1)
		{
			iTmpSerFd = m_iSerial1fd  ;
		}
		else if(iSerNum == SERIALNUM2)
		{
			iTmpSerFd = m_iSerial2fd  ;	
		}
		else if(iSerNum == SERIALNUM3)
		{
			iTmpSerFd = m_iSerial3fd  ;	
		}

	}
	return iTmpSerFd;
}


int CSerialCtrl::serialWrite(char *buffer,int fd)
{
	//isBusy=1;
	if(strlen(buffer)==0)
		return 0;
	int writeCount = write(fd ,buffer ,strlen(buffer));
	//isBusy=0;
	return writeCount ;
}


void CSerialCtrl::set_speed(int fd, int speed)
{ 
	 
	
	int speed_arr[7] = { B38400, B19200, B9600, B4800, B2400, B1200, B300}; 
	int name_arr[7] = {38400, 19200, 9600, 4800, 2400, 1200, 300}; 

	struct termios Opt; 
	tcgetattr(fd, &Opt); 
	for (int i= 0; i < 7; i++) 
	{ 
		if (speed == name_arr[i]) 
		{ 
			tcflush(fd, TCIOFLUSH); 
			cfsetispeed(&Opt,speed_arr[i]); 
			cfsetospeed(&Opt, speed_arr[i]); 
			if(tcsetattr(fd, TCSANOW, &Opt) !=0)
			{
				ACE_DEBUG((LM_DEBUG,"Set serialCom bitrate error!\n"));
				return; 
			}			
			
			return; 
		} 
	 
	}
	tcflush(fd,TCIOFLUSH); 
} 


