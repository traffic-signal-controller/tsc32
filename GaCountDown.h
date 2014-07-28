#ifndef _GACOUNTDOWN_H_
#define _GACOUNTDOWN_H_

#include "ComStruct.h"

#define GACOUNTDOWN_CPP "GaCountDown.cpp"

//const int GA_YELLOW_COUNT = 0;   //黄灯不倒计时 >0就倒
const int GA_MAX_DIRECT    = 8;   //最大方向 北 东 南 西 
const int GA_MAX_LANE      = 10;   //最大类型 左 直 右 人行
const int GA_MAX_COLOR     = 3;   //红 黄 绿
const int GA_MAX_SEND_BUF  = 5;  //倒计时协议定义的每帧数据字节数

enum
{
	GA_DIRECT_NORTH  = 0 ,  //北
	GA_DIRECT_EAST   = 1 ,  //东
	GA_DIRECT_SOUTH  = 2 ,  //南
	GA_DIRECT_WEST   = 3 ,  //西
	GA_DIRECT_NORTH_EAST  = 4 , //东北
	GA_DIRECT_SOUTH_EAST  = 5 , //东南
	GA_DIRECT_SOUTH_WEST  = 6 , //西南
	GA_DIRECT_NORTH_WEST  = 7  //西北
	
};

enum
{
	GA_LANE_LEFT     = 0 ,  //左   
	GA_LANE_STRAIGHT = 1 ,  //直
	GA_LANE_RIGHT    = 2 ,  //右
	GA_LANE_FOOT     = 3 ,  //一次人行
	GA_LANE_SECOND_FOOT    = 4 , //二次人行
	GA_LANE_LEFT_STRAIGHT  = 5 , //左直
	GA_LANE_RIGHT_STRAIGHT = 6 , //右直
	GA_LANE_LEFT_RIGHT_STRAIGHT = 7 , //左直右
	GA_LANE_TRUNAROUND      = 8 , //调头
	GA_LANE_OTHER            = 9 // 其他
	
	
};

enum
{
	GA_COLOR_RED    = 0 , //红
	GA_COLOR_YELLOW = 1 , //黄
	GA_COLOR_GREEN  = 2 , //绿
	GA_COLOR_BLACK  = 3  , //黑色
	GA_COLOR_OTHER    = 4  //其他颜色
};


/*
*国标主动倒计时类(根据GAT508-2004协议)
*每个方向只有一个倒计时牌  为此人行倒计时不在考虑范围
*/
class CGaCountDown
{
public:
	static CGaCountDown* CreateInstance();

	void GaGetCntDownInfo();
	void GaUpdateCntDownCfg();
	void GaSendStepPer();

	static Byte m_ucGaDirVector[GA_MAX_DIRECT][GA_MAX_LANE];
	 

	CGaCountDown();
	~CGaCountDown();

	Byte GaGetCntTime(Byte ucSignalId);
	void GaGetDirLane(Byte ucTableId , Byte& ucDir , Byte& ucLane);
	void GaSetSendBuffer();
	bool ComputeColorCount( Byte ucDirIndex , Byte& ucColor , Byte& ucCountTime );
	bool PackSendBuf( Byte ucDirIndex , Byte ucColor , Byte ucCountTime );
	void sendblack();    //发送黑屏信息
	void send8cntdown(); //发送8秒倒计时
	void GaGetDirecColorTime();
	void SetClinetCntDown(ACE_INET_Addr& addrClient, Uint uBufCnt , Byte* pBuf) ; //ADD:201401121705
	bool m_bGaNeedCntDown[GA_MAX_DIRECT][GA_MAX_LANE]; //需要倒计时的方向
	Byte m_ucGaRuntime[GA_MAX_DIRECT][GA_MAX_LANE];    //该灯色还需维持的时间
	Byte m_ucGaColor[GA_MAX_DIRECT][GA_MAX_LANE];      //灯色
	GBT_DB::PhaseToDirec m_sGaPhaseToDirec[GA_MAX_DIRECT][GA_MAX_LANE];
	SendCntDownNum sSendCDN[MAX_CLIENT_NUM] ;  //ADD:201401121705

	Byte m_sGaSendBuf[GA_MAX_DIRECT][GA_MAX_SEND_BUF];   //发送缓存
    Byte m_ucLampBoardFlashBreak[MAX_LAMP_BOARD]; //201407231400
private:
};

#endif //_GACOUNTDOWN_H_
