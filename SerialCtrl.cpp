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

#define SERIAL0  "/dev/ttyO0"    //����0
#define SERIAL1  "/dev/ttyO1"    //����1
#define SERIAL2  "/dev/ttyO2"    //����2
#define SERIAL3  "/dev/ttyO3"    //����3
#define SERIAL4  "/dev/ttyO4"	//485
#define SERIAL5  "/dev/ttyO5"    //485

#define SERIALNUM1  1
#define SERIALNUM2  2
#define SERIALNUM3  3
#define SERIALNUM4  4
#define SERIALNUM5  5

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
	m_iSerial4fd = -1;
	m_iSerial5fd = -1;
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
	else if( m_iSerial4fd > 0 )
	{
		close(m_iSerial4fd);
	}
	else if( m_iSerial5fd > 0 )
	{
		close(m_iSerial5fd);
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
	else if(iSerNum == SERIALNUM4)
	{
		m_iSerial4fd = open(SERIAL4, O_RDWR | O_NOCTTY | O_NONBLOCK);
		iTmpSerFd = m_iSerial4fd ;
	}
	else if(iSerNum == SERIALNUM5)
	{
		m_iSerial5fd = open(SERIAL5, O_RDWR | O_NOCTTY | O_NONBLOCK);
		iTmpSerFd = m_iSerial5fd ;
	}
	cfmakeraw(&terminfo);

	terminfo.c_iflag |= IGNBRK | IGNPAR | IXANY;
	terminfo.c_cflag = CREAD | CS8 | CLOCAL | B38400;
	if (tcsetattr(iTmpSerFd, TCSANOW, &terminfo) == -1)
	{
		ACE_DEBUG((LM_DEBUG,"tcsetattr error ��ȡ��������ʱ��ش�-1����������errnoΪEAGAIN��\n"));
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
		else if(iSerNum == SERIALNUM4)
		{
			m_iSerial4fd = -1 ;		
		}
		else if(iSerNum == SERIALNUM4)
		{
			m_iSerial4fd = -1 ;		
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
	else if(iSerNum == SERIALNUM4)
	{
		iTmpSerFd = m_iSerial4fd  ;	
	}
	else if(iSerNum == SERIALNUM5)
	{
		iTmpSerFd = m_iSerial5fd  ;	
	}
	//
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
		else if(iSerNum == SERIALNUM4)
		{
			iTmpSerFd = m_iSerial4fd  ;	
		}
		else if(iSerNum == SERIALNUM5)
		{
			iTmpSerFd = m_iSerial5fd  ;	
		}
	}
	return iTmpSerFd;
}



