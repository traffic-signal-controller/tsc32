
/***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   ComFunc.cpp
Date:       2013-9-25
Description:通用处理函数
Version:    V1.0
History:    201310081456  yiodng testmem()
***************************************************************/
#include <sys/ioctl.h>
#include <fcntl.h>
#include "ComFunc.h"
#include "ManaKernel.h"
#include "DbInstance.h"

// CRC校验表数组
Ushort const fcstab[] =
{
	0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
	0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
	0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
	0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
	0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
	0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
	0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
	0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
	0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
	0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
	0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
	0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
	0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
	0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
	0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
	0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
	0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
	0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
	0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
	0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
	0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
	0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
	0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
	0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
	0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
	0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
	0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
	0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
	0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
	0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
	0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
	0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0,
};


/**************************************************************
Function:       IsLeapYear
Description:    判断是否时润年
Input:          ucYear  年份              
Output:         无
Return:         false-非润年 true-润年
***************************************************************/
bool IsLeapYear(Uint ucYear)
{
	if ( ( 0==ucYear%4 && ucYear%100!=0)||(0==ucYear%400) )
	{
		return true;
	}
	return false;
}

/**************************************************************
Function:       IsRightDate
Description:    判断日期到合法性
Input:          ucYear  年份
				ucMonth 月份 
				ucDay   日期             
Output:         无
Return:         false-非润年 true-润年
***************************************************************/
bool IsRightDate(Uint ucYear,Byte ucMonth,Byte ucDay)
{
	Byte ucMaxDay = 31;

	if ( ucYear < 2000 || ucYear >= 2038 )
	{
		return false;
	}
	if ( ucMonth < 1 || ucMonth > 12 )
	{
		return false;
	}
	
	switch ( ucMonth )
	{
		case 2:
			if ( IsLeapYear(ucYear) )  //闰年
			{
				ucDay = 29;
			}
			else
			{
				ucDay = 28;
			}
			break;
		case 4:
			ucMaxDay = 30;
			break;
		case 6:
			ucMaxDay = 30;
			break;
		case 9:
			ucMaxDay = 30;
			break;
		case 11:
			ucMaxDay = 30;
			break;
		default:
			break;
	}

	if ( ucDay > ucMaxDay )
	{
		return false;
	}
	return true;

}

/**************************************************************
Function:       getCurrTime
Description:    获取当前时间，这个时间获取函数主要用于信号机定时器，
				避免修改时间到时候造成定时器停止
Input:          无   
Output:         无
Return:         ACE时间
***************************************************************/
ACE_Time_Value getCurrTime()
{
	ACE_Time_Value tv;
	struct timespec ts = {0};
	::clock_gettime(CLOCK_MONOTONIC, &ts);
	ACE_hrtime_t hrt = static_cast<ACE_hrtime_t> (ts.tv_sec) * ACE_U_ONE_SECOND_IN_NSECS+ static_cast<ACE_hrtime_t> (ts.tv_nsec);
	ACE_High_Res_Timer::hrtime_to_tv (tv, hrt);
	return tv;
}

/**************************************************************
Function:       GetCurTime
Description:    获取当前时间，用于除定时器时间获取外其他地方
Input:          无   
Output:         无
Return:         ACE时间
***************************************************************/
ACE_Time_Value GetCurTime()
{
#ifdef LINUX
	time_t ti;
	struct tm rtctm;
	int fd = open(DEV_RTC, O_WRONLY, 0);
	int ret = -1;
	if(fd>0)
	{
		ret = ioctl(fd, RTC_RD_TIME, &rtctm);
		close(fd);
	}
	ti = mktime(&rtctm);
	return ACE_Time_Value(ti);
#else
	return ACE_OS::gettimeofday();
#endif
}

/**************************************************************
Function:       CopyFile
Description:    复制文件
Input:          无   
Output:         无
Return:         无
***************************************************************/
void CopyFile(char src[], char dest[])
{
#ifdef LINUX	
	int sd, dd;
	char buf[100];
	int n;

	if (strcmp(dest, src) == 0)
		return;

	sd = open(src, O_RDONLY);
	dd = open(dest, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (sd == -1 || dd == -1)
		goto out;

	while ((n = read(sd, buf, sizeof(buf))) > 0) {
		write(dd, buf, n);
	}

out:
	if (sd != -1)
		close(sd);
	if (dd != -1)
		close(dd);
#endif
}


/**************************************************************
Function:       CopyFile
Description:    调整文件的大小，超过iMaxFileLine时，删除掉一半，
				防止文件过大
Input:          无   
Output:         无
Return:         无
***************************************************************/
void AdjustFileSize(char* file,int iMaxFileLine)
{
#ifdef LINUX
	FILE *fp;
	FILE *fp2;
	int line = 0;
	int ch;
	if ((fp = fopen(file, "r")) == NULL)
	{
		return;
	}
	while ( !feof(fp) ) 
	{
		ch = fgetc(fp);
		if (ch == '\n')
		{
			line++;
		}
	}
	if ( line < iMaxFileLine ) 
	{
		fclose(fp);
		return;
	}
	fseek(fp, 0, SEEK_SET);
	while (!feof(fp) && ( line > iMaxFileLine / 2 ) ) 
	{
		ch = fgetc(fp);
		if (ch == '\n')
		{
			line--;
		}
	}
	if ( (fp2 = fopen("tmp.txt", "w")) != NULL) 
	{
		while ((ch = fgetc(fp)) != EOF) 
		{
			fputc(ch, fp2);
		}
		fclose(fp2);
		fclose(fp);
		CopyFile((char*)"tmp.txt", file);
		remove((char*)"tmp.txt");
	} 
	else 
	{
		fclose(fp);
	}
#endif
}


/**************************************************************
Function:       RecordTscStartTime
Description:    记录系统开始运行时间，并写入日志			
Input:          无        
Output:         无
Return:         无
***************************************************************/
void RecordTscStartTime()
{
#ifdef LINUX
	ACE_OS::system("echo At $(date) tsc restart !>> TscRun.log");
#endif
	//unsigned long mRestart = 0 ;
	//(CDbInstance::m_cGbtTscDb).GetSystemData("ucDownloadFlag",mRestart);
	CManaKernel::CreateInstance()->SndMsgLog(LOG_TYPE_REBOOT,0,0,0,0);	
}

/**************************************************************
Function:       TestMem
Description:    测试系统内存稳定性	
Input:          无              
Output:         无
Return:         无
***************************************************************/
void TestMem(char* funName,int iLine)
{
#ifdef LINUX
	static int iMemCnt = 0;
	int iCurMemCnt     = 0;
	//FILE* fp = NULL;
	system("ps | grep Gb.aiton > mem.txt");

	int sd;
	char buf[100] = {0};
	char cTmp[36] = {0};
	int n;

	sd = open("mem.txt", O_RDONLY);
	n = read(sd, buf, sizeof(buf));
	close(sd);

	ACE_OS::memcpy(cTmp,buf+15,5);

	iCurMemCnt = atoi(cTmp);


	if ( iCurMemCnt != iMemCnt )  
	{
		iMemCnt = iCurMemCnt;
		
		FILE* file = NULL;
		char tmp[64] = {0};
		struct tm *now;
		time_t ti;

		file = fopen("moreMem.txt","a+");
		if ( NULL == file )
		{
			return;
		}

		ti = time(NULL);
		now = localtime(&ti);
		sprintf(tmp,"%d-%d-%d %d %d:%02d:%02d %s:%d memcnt:%d.\n", now->tm_year + 1900,
			now->tm_mon + 1, now->tm_mday, now->tm_wday, now->tm_hour,
			now->tm_min, now->tm_sec,funName,iLine,iCurMemCnt);
		fputs(tmp,file);
		fclose(file);
	}

	//ACE_DEBUG((LM_DEBUG,"buf:%s cTmp:%s iMemCnt:%d \n",buf,cTmp,iMemCnt));
#endif
}


/**************************************************************
Function:       CompareTime
Description:    时间大小比较
Input:          ucHour1 ：比较时间1的小时值 
			    ucMin1  ：比较时间1的分钟值  
			    ucHour1 ：比较时间2的小时值 
			    ucMin1  ：比较时间2的分钟值    
Output:         无
Return:         0:time2比time1小   1：time2比time1大,或相等 
***************************************************************/
bool CompareTime(Byte ucHour1,Byte ucMin1,Byte ucHour2,Byte ucMin2)
{
	if ( ucHour2 > ucHour1 )
	{		
		return 1;
	}
	else if ( ucHour2 == ucHour1 )
	{
		if ( ucMin2 >= ucMin1 )
		{			
			return 1;
		}
	}	
	return 0;
}


/**************************************************************
Function:       GetEypDevID
Description:    对字符串异或加密处理
Input:          src ：待加密字符串指针 
			    dec  ：加密后字符串指针 
Output:         无
Return:         -1 -加密字符串失败 0 - 加密字串串成功
***************************************************************/
int GetEypChar(char *src ,char *dec)
{  
   if(src == NULL || dec ==NULL)
   		return -1 ;
   char *p=src; 
   int i=0; 
   while(*p!='\0') 
   { 
     dec[i]=0x01 ^ *p; /* 异或处理加密*/
     p++; 
     i++; 
  } 
 	dec[i]='\0';
 	return 0 ;
}  


/**************************************************************

Function:       GetStrHwAddr
Description:    获取网卡物理地址字符串
Input:          StrHwAddr ：网卡物理地址字符串 
Output:         无
Return:         -1 获取失败  0 -获取成功
***************************************************************/
int GetStrHwAddr(char * strHwAddr)
{
        //char strHwAddr[32];
        int fdReq = -1;
        struct ifreq typIfReq;
        char *hw;
        memset(&typIfReq, 0x00, sizeof(typIfReq));
        fdReq = socket(AF_INET, SOCK_STREAM, 0);
        strcpy(typIfReq.ifr_name, "eth0");
        if(ioctl(fdReq, SIOCGIFHWADDR, &typIfReq) < 0)
        {
            printf("fail to get hwaddr %s %d\n", typIfReq.ifr_name, fdReq);
            close(fdReq);
            return -1;
        }
        hw = typIfReq.ifr_hwaddr.sa_data;
        sprintf(strHwAddr, "%02x%02x%02x%02x%02x%02x", *hw, *(hw+1), *(hw+2), *(hw+3), *(hw+4), *(hw+5));
        close(fdReq);  
       
        return 0;
}

/**************************************************************
Function:       SaveEnyDevId
Description:    保存设置设备ID加密存储
Input:          SysEnyDevId ：系统加密ID指针 
Output:         无
Return:         -1 - 产生加密设备ID失败 0 -产生加密设备ID成功
***************************************************************/
int GetSysEnyDevId(char *SysEnyDevId)
{
    char strHwAddr[32]={0};
    char enyDevId[32] = {0};
    if(GetStrHwAddr(strHwAddr) == -1)
       return -1;
    if(GetEypChar(strHwAddr ,enyDevId)==-1)
       return -1;     
     ACE_OS::strcpy(SysEnyDevId,enyDevId); 
      
     return 0;
}
/**************************************************************
Function:       GetMainBroadCdKey
Description:    保存设置设备ID加密存储
Input:          CdKey ：系统加密ID指针 
Output:         无
Return:         -1 - 产生加密设备ID失败 0 -产生加密设备ID成功
***************************************************************/
int GetMainBroadCdKey(char *CdKey)
{

	char CCdkey[8] = {0};
	if(!Cdkey::GetCdkey(CCdkey))
	{
		return -1;
	}
     ACE_OS::strcpy(CdKey,CCdkey); 
     ACE_DEBUG((LM_DEBUG,"%s:%d***GetMainBroadCdKey*** CdKey : %X !\n",__FILE__,__LINE__,&CdKey));
     return 0;
}
/**************************************************************
Function:      VaildSN
Description:    对核心板的序列号与系统的序列号进行对比
Input:          无 
Output:         无
Return:         1 - 合法 0 -非法
***************************************************************/
bool VaildSN()
{
	char fileSN[8] = {0};
	char deviceSN[8] = {0};
	int i,bol;
	ReadTscSN(fileSN);
	GetMainBroadCdKey(deviceSN);
	
	bol = ACE_OS::strcmp(fileSN,deviceSN);
	if(bol == 0)
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d***VaildSN*** SN VAILD !\n",__FILE__,__LINE__));
		return true;
	}
	else
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d***VaildSN*** SN INVALID !\n",__FILE__,__LINE__));
		return false;
	}
}
/**************************************************************
Function:      ReadTscSN
Description:    读取系统的序列号
Input:          无 
Output:         将8个字节的序列号保存到cdkey 指针 中
Return:         无
***************************************************************/
void ReadTscSN(char *cdkey)
{
	
	char tmp[8];
	FILE *infile;
	int rc;
	infile = fopen("sn.dat", "rb");
	if(infile == NULL)
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d read File error !\n",__FILE__,__LINE__));
		return ;
	}
	rc = fread(tmp,sizeof(char), 8,infile);
	if(rc ==0)
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d read File error !\n",__FILE__,__LINE__));
		return ;
	}
	ACE_OS::strcpy(cdkey,tmp); 
	fclose(infile);
}

/**************************************************************
Function:       RecordTscStartTime
Description:    记录系统开始运行时间，并写入日志			
Input:          无        
Output:         无
Return:         无
***************************************************************/
void RecordTscSN()
{
#ifdef LINUX
	
	char tmp[8] = {0};
	int i;
	FILE *outfile, *infile;
	infile = fopen("sn.dat","rb");
	if(infile != NULL)
	{
		ACE_DEBUG((LM_DEBUG,"\n%s:%d***RecordTscSN*** SN file is exsit !\n",__FILE__,__LINE__));
		fclose(infile);
		return ;
	}
	outfile = ACE_OS::fopen("sn.dat", "wb" );
	GetMainBroadCdKey(tmp);
	if( outfile == NULL)
    {
        ACE_DEBUG((LM_DEBUG,"%s:%d  open file error !\n",__FILE__,__LINE__));
        return;
    }   
	//for(i=0; i<8; i++) 
	//{	
		//fwrite( tmp, sizeof(char), 8, outfile );
		//ACE_DEBUG((LM_DEBUG,"%s:%d  0x%x !\n",__FILE__,__LINE__,tmp[i]));	
	//}
	ACE_OS::fwrite( tmp, sizeof(char), 8, outfile );
	ACE_OS::fclose(outfile);
#endif	
}


/**************************************************************
Function:       sum8
Description:    校验和计算			
Input:           buf-待校验数据起始地址 len-待校验数据长度        
Output:         无
Return:         校验结果
Date:           2015-05-27
***************************************************************/

Byte sum8(Byte  *buf,Byte len)
{
    Byte Sumtmp =0x0;
    for(;len;--len,++buf)
    {
        Sumtmp += *buf;
    }
    return Sumtmp;
}

/**************************************************************
Function:       fcs16
Description:    CRC校验计算			
Input:           buf-待校验数据起始地址 len-待校验数据长度        
Output:         无
Return:         校验CRC结果值
Date:           2015-05-27
***************************************************************/

Ushort fcs16(Byte  *buf,Ushort len)
{
	Ushort fcs = 0x0;
	while (len--)
	{
		fcs = (fcs << 8) ^ fcstab[(fcs >> 8) ^ *buf++];
	}
	return (fcs);
}

