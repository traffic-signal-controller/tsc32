/***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   Cdkey.cpp
Date:       2013-1-1
Description:对信号机的序列号进行检查.
Version:    V1.0
History:
***************************************************************/
#include "Cdkey.h"

Cdkey::Cdkey()
{
	ACE_DEBUG((LM_DEBUG,"%s:%d construnt Cdkey object ok !\n",__FILE__,__LINE__));
}

Cdkey::~ Cdkey()
{
	ACE_DEBUG((LM_DEBUG,"%s:%d destroy Cdkey object ok !\n",__FILE__,__LINE__));
}

Cdkey* Cdkey::CreateInstance()
{
	static Cdkey Ccdkey;
	return &Ccdkey;
}


void Cdkey::InitCdkey()
{
	ACE_DEBUG((LM_DEBUG,"%s:%d init Cdkey object ok !\n",__FILE__,__LINE__));
}

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
	fd = open("/dev/sysinfo", O_RDWR);	
	if (fd < 0) 
	{		
		printf("open /dev/sysinfo error!\n");		
		return -1;	
	}	
	ret = read(fd, buf, 120);	
	for (i=0; i<120; i++) 
	{		
		printf("0x%x  ", buf[i]);	
	}	
	printf("\n");	
	ACE_DEBUG((LM_DEBUG,"%s:%d chip ID is !\n",__FILE__,__LINE__));
	for(i=0; i<8; i++) 
	{		
		id[i] = buf[112+i];
		cdkey[i] = id[i];
		ACE_DEBUG((LM_DEBUG,"%s:%d  0x%x !\n",__FILE__,__LINE__,id[i]));
	}
	//cdkey = id;
	close(fd);
	return true;
}
