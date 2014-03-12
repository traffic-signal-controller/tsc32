#ifndef _GACOUNTDOWN_H_
#define _GACOUNTDOWN_H_

#include "ComStruct.h"

#define GACOUNTDOWN_CPP "GaCountDown.cpp"

//const int GA_YELLOW_COUNT = 0;   //�ƵƲ�����ʱ >0�͵�
const int GA_MAX_DIRECT    = 8;   //����� �� �� �� �� 
const int GA_MAX_LANE      = 10;   //������� �� ֱ �� ����
const int GA_MAX_COLOR     = 3;   //�� �� ��
const int GA_MAX_SEND_BUF  = 5;  //����ʱЭ�鶨���ÿ֡�����ֽ���

enum
{
	GA_DIRECT_NORTH  = 0 ,  //��
	GA_DIRECT_EAST   = 1 ,  //��
	GA_DIRECT_SOUTH  = 2 ,  //��
	GA_DIRECT_WEST   = 3 ,  //��
	GA_DIRECT_NORTH_EAST  = 4 , //����
	GA_DIRECT_SOUTH_EAST  = 5 , //����
	GA_DIRECT_SOUTH_WEST  = 6 , //����
	GA_DIRECT_NORTH_WEST  = 7  //����
	
};

enum
{
	GA_LANE_LEFT     = 0 ,  //��   
	GA_LANE_STRAIGHT = 1 ,  //ֱ
	GA_LANE_RIGHT    = 2 ,  //��
	GA_LANE_FOOT     = 3 ,  //һ������
	GA_LANE_SECOND_FOOT    = 4 , //��������
	GA_LANE_LEFT_STRAIGHT  = 5 , //��ֱ
	GA_LANE_RIGHT_STRAIGHT = 6 , //��ֱ
	GA_LANE_LEFT_RIGHT_STRAIGHT = 7 , //��ֱ��
	GA_LANE_TRUNAROUND      = 8 , //��ͷ
	GA_LANE_OTHER            = 9 // ����
	
	
};

enum
{
	GA_COLOR_RED    = 0 , //��
	GA_COLOR_YELLOW = 1 , //��
	GA_COLOR_GREEN  = 2 , //��
	GA_COLOR_BLACK  = 3  , //��ɫ
	GA_COLOR_OTHER    = 4  //������ɫ
};

/*
*������������ʱ��(����GAT508-2004Э��)
*ÿ������ֻ��һ������ʱ��  Ϊ�����е���ʱ���ڿ��Ƿ�Χ
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
	void sendblack();    //���ͺ�����Ϣ
	void send8cntdown(); //����8�뵹��ʱ
	void SetClinetCntDown(ACE_INET_Addr& addrClient, Uint uBufCnt , Byte* pBuf) ; //ADD:201401121705
	bool m_bGaNeedCntDown[GA_MAX_DIRECT][GA_MAX_LANE]; //��Ҫ����ʱ�ķ���
	Byte m_ucGaRuntime[GA_MAX_DIRECT][GA_MAX_LANE];    //�õ�ɫ����ά�ֵ�ʱ��
	Byte m_ucGaColor[GA_MAX_DIRECT][GA_MAX_LANE];      //��ɫ
	GBT_DB::PhaseToDirec m_sGaPhaseToDirec[GA_MAX_DIRECT][GA_MAX_LANE];
	SendCntDownNum sSendCDN[MAX_CLIENT_NUM] ;  //ADD:201401121705

	Byte m_sGaSendBuf[GA_MAX_DIRECT][GA_MAX_SEND_BUF];   //���ͻ���
private:
};

#endif //_GACOUNTDOWN_H_
