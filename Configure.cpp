/*================================================================================================== 
 * 项目名称: 读取配置文件库 
 *     功能: 提供类和方法,实现从配置文件读取参数值 
 *     作者: 鲁仁华 
 *     联系: renhualu@live.com 
 * 最近修改: 2014-4-25 
 *     版本: v1.0.2 
  ==================================================================================================*/  
  
#include <sys/timeb.h>   
#include <string>  
#include <iostream>  
#include <map>    
#include <ace/OS.h>    
#include "Configure.h"  
#include <iostream>
using namespace std;

Configure* Configure::CreateInstance()
{
	static Configure con;
	return &con;
}
Configure::Configure():impExp_(NULL)
{
  bool binitfile = InitConfig() ;
 	if(!binitfile)
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d Init configure file fail !\n",__FILE__,__LINE__));
			 
	}
     if(open("tsc.ini") == -1)
  	{
		cout<<"open configure file error!\n";
	
 	 }      
}
 
Configure::~Configure()
{
       if (impExp_)
              delete impExp_;
       impExp_ = NULL;
}
 
int Configure::open(const ACE_TCHAR * filename)
{
       if (this->config.open() == -1)
	   {
		return -1 ;
       }
       this->impExp_=new ACE_Ini_ImpExp(config);
	
      return this->impExp_->import_config(filename);
		
	
}
 
int Configure::GetString(const char *szSection, const char *szKey, ACE_TString &strValue)
{
       if (config.open_section(config.root_section(),ACE_TEXT(szSection),0,this->root_key_)==-1)
              return -1;
       return config.get_string_value(this->root_key_,szKey,strValue);
}
 
int Configure::GetInteger(const char* szSection,const char* szKey,Uint& nValue)
{
       ACE_TString strValue;
       if (config.open_section(config.root_section(),ACE_TEXT(szSection),0,this->root_key_)==-1)
              return -1;
       if (config.get_string_value(this->root_key_,szKey,strValue) == -1)
              return -1;
       nValue = ACE_OS::atoi(strValue.c_str());
       if (nValue == 0 && strValue != "0")

              return -1;
       return nValue;
}

bool Configure::InitConfig()
{
	FILE* fConfig = NULL ;
	if ( (fConfig = ACE_OS::fopen(ACE_TEXT("tsc.ini"), "r")) == NULL )
	{	
		if((fConfig = ACE_OS::fopen(ACE_TEXT("tsc.ini"), "w+")) == NULL)
			return false ;
		ACE_OS::fputs("\n#Traffic Signal Control Configure",fConfig);
		ACE_OS::fputs("\n[APPDESCRIP]",fConfig);
		ACE_OS::fputs("\napplication   =Gb.aiton",fConfig);
		ACE_OS::fputs("\ndatebase      =GbAitonTsc.db",fConfig);
		ACE_OS::fputs("\nversion       =1.0.1",fConfig);
		ACE_OS::fputs("\ndescription   =32 Phase Traffic Singal Controner",fConfig);
		
		ACE_OS::fputs("\n[COMMUNICATION]",fConfig);
		ACE_OS::fputs("\nstandard      =GBT20999",fConfig);
		ACE_OS::fputs("\nprotocol      =udp",fConfig);
		ACE_OS::fputs("\nport          =5435 ",fConfig);

		ACE_OS::fputs("\n[CONTACT]",fConfig);
		ACE_OS::fputs("\ncompany       =XiaMenAiTon",fConfig);
		ACE_OS::fputs("\nlinkman       =Chen",fConfig);
		ACE_OS::fputs("\ntelephone     =0592-5212811",fConfig);
		ACE_OS::fputs("\naddress       =No151,BanMei,HuLi,XiaMen,FuJian,China",fConfig);
		ACE_OS::fputs("\nWebSite       =http://www.aiton.com.cn	",fConfig);
		ACE_OS::fclose(fConfig);
		return true ;
	}
	return true ;
}
void Configure::ShowConfig()
{
	Configure *pMyconfig =  Configure::CreateInstance() ;
  ACE_TString vstring ;
  if(pMyconfig->open("tsc.ini") == -1)
  {
	cout<<"open configure file error!\n";
	return ;
  }
 cout<<"#..................Show Tsc Configure....................# "<<endl ;
 pMyconfig->GetString("APPDESCRIP","application",vstring);
	cout<<"AppName :" <<vstring.c_str()<<endl ;
  pMyconfig->GetString("APPDESCRIP","datebase",vstring);
	cout<<"DataBaseName :" <<vstring.c_str()<<endl ;	
   pMyconfig->GetString("APPDESCRIP","version",vstring);
	cout<<"Version :" <<vstring.c_str()<<endl ;
  pMyconfig->GetString("APPDESCRIP","description",vstring);
	cout<<"Description :" <<vstring.c_str()<<endl ;

 pMyconfig->GetString("COMMUNICATION","standard",vstring);
	cout<<"Standard :" <<vstring.c_str()<<endl ;
  pMyconfig->GetString("COMMUNICATION","protocol",vstring);
	cout<<"Protocol :" <<vstring.c_str()<<endl ;	
   pMyconfig->GetString("COMMUNICATION","port",vstring);
	cout<<"Portnumber :" <<vstring.c_str()<<endl ;

  pMyconfig->GetString("CONTACT","company",vstring);
	cout<<"Company :" <<vstring.c_str()<<endl ;
pMyconfig->GetString("CONTACT","linkman",vstring);
	cout<<"LinkMan :" <<vstring.c_str()<<endl ;
pMyconfig->GetString("CONTACT","telephone",vstring);
	cout<<"Telephone :" <<vstring.c_str()<<endl ;
pMyconfig->GetString("CONTACT","address",vstring);
	cout<<"Address :" <<vstring.c_str()<<endl ;
pMyconfig->GetString("CONTACT","WebSite",vstring);
	cout<<"WebSite :" <<vstring.c_str()<<endl ;

cout<<"#................End Show Tsc Configure..................# "<<endl ;
}


int Configure::close()
{
       if (impExp_)
       {
              delete impExp_;
              impExp_ = NULL;
       }
       return 0;
} 
