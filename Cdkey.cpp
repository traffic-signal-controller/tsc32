/***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   Cdkey.cpp
Date:       2013-1-1
Description:���źŻ������кŽ��м��.
Version:    V1.0
History:
***************************************************************/
#include "Cdkey.h"

/**************************************************************
Function:       Cdkey::Cdkey
Description:    CDKEY�Ĺ��캯��
Output:         ��              
Input:          ��
Return:         0
***************************************************************/
Cdkey::Cdkey()
{
	ACE_DEBUG((LM_DEBUG,"%s:%d Construnt Cdkey object ok !\n",__FILE__,__LINE__));
}

/**************************************************************
Function:       Cdkey::~Cdkey
Description:    CDKEY����������
Output:         ��              
Input:          ��
Return:         0
***************************************************************/
Cdkey::~ Cdkey()
{
	ACE_DEBUG((LM_DEBUG,"%s:%d Destroy Cdkey object ok !\n",__FILE__,__LINE__));
}

/**************************************************************
Function:       Cdkey* Cdkey::CreateInstance()
Description:    ����ģʽ����һ��cdkey����
Output:         ��              
Input:          ��
Return:         cdkey ָ��
***************************************************************/
Cdkey* Cdkey::CreateInstance()
{
	static Cdkey Ccdkey;
	return &Ccdkey;
}

/**************************************************************
Function:       Cdkey::InitCdkey()
Description:    �������г�ʼ��
Output:         ��              
Input:          ��
Return:         ��
***************************************************************/
void Cdkey::InitCdkey()
{
	ACE_DEBUG((LM_DEBUG,"%s:%d init Cdkey object ok !\r\n",__FILE__,__LINE__));
}
/**************************************************************
Function:        Cdkey::GetCdkey(char (&cdkey)[8])
Description:    �������г�ʼ��
Output:         char (&cdkey)[8]   8���ֽڵ����к�            
Input:          ��
Return:         1 �ɹ����أ�0 ����ʧ�� 
***************************************************************/
bool Cdkey::GetCdkey(char (&cdkey)[8])
{
	int i;	
	int ret;	
	int fd;	
	char buf[120];	
	char id[8];	
	//unsigned char CRP[8];	
	//char random[64];	
	//char host_out[20];	
	//char chip_out[20];	
	//char passwd[8] = {"0"};	
	//char passwd2[8] = {"1"};
	//ACE_DEBUG((LM_DEBUG,"%s:%d read all data from EEPROM !\n",__FILE__,__LINE__));
	fd = ACE_OS::open("/dev/sysinfo", O_RDWR);	
	if (fd < 0) 
	{		
		ACE_OS::printf("%s:%d Open /dev/sysinfo error!\n",__FILE__,__LINE__);		
		return -1;	
	}	
	ret = ACE_OS::read(fd, buf, 120);	
	//for (i=0; i<120; i++) 
	//{		
		//printf("0x%x  ", buf[i]);	
	//}	
	//printf("\n");	
	ACE_DEBUG((LM_DEBUG,"%s:%d chip ID is !\n",__FILE__,__LINE__));
	for(i=0; i<8; i++) 
	{		
		id[i] = buf[112+i];
		cdkey[i] = id[i];
		//ACE_DEBUG((LM_DEBUG,"%s:%d  0x%x !\n",__FILE__,__LINE__,id[i]));
	}
	//cdkey = id;
	ACE_OS::close(fd);
	return true;
}
