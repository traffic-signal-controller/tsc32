#include "GaCountDown.h"
#include "Rs485.h"
#include "ManaKernel.h"
#include "DbInstance.h"
#include "string.h"
#define SEND_CLIENT_CNTOWN_BYTE 43
/**************************************************************
Function:       CGaCountDown::CGaCountDown
Description:    CGaCountDown�����ڵ���ʱ���ʼ������				
Input:          ��              
Output:         ��
Return:         ��
***************************************************************/
CGaCountDown::CGaCountDown()
{
	ACE_OS::memset(m_sGaSendBuf , 0 , GA_MAX_SEND_BUF );
	//ACE_OS::memset(m_sGaPhaseToDirec,0,(GA_MAX_DIRECT*GA_MAX_LANE)*sizeof(GBT_DB::PhaseToDirec)) ;
	//ACE_OS::memset(&sSendCDN,0,MAX_CLIENT_NUM*sizeof(SendCntDownNum)) ;
	ACE_OS::memset(m_ucLampBoardFlashBreak, 0x0 , MAX_LAMP_BOARD);
	for(Byte index = 0 ;index<MAX_CLIENT_NUM ; index++)
	{
		sSendCDN[index].bSend = false ;
		sSendCDN[index].bUsed = false ;
	}
	ACE_DEBUG((LM_DEBUG,"%s:%d Init GaCountDown object ok !\n",__FILE__,__LINE__));
}

/**************************************************************
Function:       CGaCountDown::~CGaCountDown
Description:    CGaCountDown��	��������	
Input:          ��              
Output:         ��
Return:         ��
***************************************************************/
CGaCountDown::~CGaCountDown()
{
	ACE_DEBUG((LM_DEBUG,"%s:%d Destruct GaCountDown object ok !\n",__FILE__,__LINE__));
}

/**************************************************************
Function:       CGaCountDown::CreateInstance
Description:    ����CGaCountDown��̬����
Input:          ��              
Output:         ��
Return:         ��̬����ָ��
***************************************************************/
CGaCountDown* CGaCountDown::CreateInstance()
{
	static CGaCountDown oGaCountDown;

	return &oGaCountDown;
}


/**************************************************************
Function:       CGaCountDown::GaGetCntDownInfo
Description:    �жϸ�������/����(��ֱ����������)�Ƿ���Ҫ����ʱ
				�Լ������Ӧ�ĵ���ʱ�����͵�ɫ��Ϣ
Input:           ��              
Output:         ��
Return:          ��
***************************************************************/
void CGaCountDown::GaGetCntDownInfo()
{
	CManaKernel* pCWorkParaManager 	 = CManaKernel::CreateInstance();
	
	STscRunData* pRunData	= pCWorkParaManager->m_pRunData;
	STscConfig * pConfig	= pCWorkParaManager->m_pTscConfig;
	Byte ucCurStep                      = pRunData->ucStepNo;
	SStepInfo*   pStepInfo              = pRunData->sStageStepInfo + ucCurStep;
	
	bool bIsAllowPhase  = false;
	Byte ucPhaseId      = 0;
	Byte ucSignalGrpNum = 0;
	Byte ucLightLamp    = 0;
	Byte ucDirIndex     = 0;
	Byte ucLaneIndex    = 0;
	Byte ucSignalGrp[MAX_CHANNEL] = {0};

	for ( ucDirIndex=0; ucDirIndex<GA_MAX_DIRECT; ucDirIndex++ )
	{
		for ( ucLaneIndex=0; ucLaneIndex<GA_MAX_LANE; ucLaneIndex++ )
		{
		
			if(m_sGaPhaseToDirec[ucDirIndex][ucLaneIndex].ucOverlapPhase == 0 &&
			   m_sGaPhaseToDirec[ucDirIndex][ucLaneIndex].ucPhase == 0)
			 {
				m_bGaNeedCntDown[ucDirIndex][ucLaneIndex] = false;
				continue ;
			 }
			else if ( m_sGaPhaseToDirec[ucDirIndex][ucLaneIndex].ucOverlapPhase != 0 )
			{
				ucPhaseId     = m_sGaPhaseToDirec[ucDirIndex][ucLaneIndex].ucOverlapPhase;
				bIsAllowPhase = false;
			}
			else if ( m_sGaPhaseToDirec[ucDirIndex][ucLaneIndex].ucPhase != 0 )
			{
				ucPhaseId     = m_sGaPhaseToDirec[ucDirIndex][ucLaneIndex].ucPhase;
				bIsAllowPhase = true;
			}
		
			//��λ����+��λid-->ͨ����Ϣ(ryg)
			ucSignalGrpNum = 0;
			pCWorkParaManager->GetSignalGroupId(bIsAllowPhase ,ucPhaseId,&ucSignalGrpNum,ucSignalGrp);

			if ( ucSignalGrpNum > 0 ) //��λ��Ӧͨ����������0
			{
				ucLightLamp = (ucSignalGrp[0] - 1) * 3;  //ucLightLamp ͨ����ţ������жϵ�ǰͨ�������ʲô�ơ�
				if ( 1 == pStepInfo->ucLampOn[ucLightLamp] )
				{
					m_ucGaColor[ucDirIndex][ucLaneIndex] = GA_COLOR_RED;
				}
				else if ( 1 == pStepInfo->ucLampOn[ucLightLamp+1] )
				{
					m_ucGaColor[ucDirIndex][ucLaneIndex] = GA_COLOR_YELLOW;
					ucLightLamp = ucLightLamp + 1;
				}
				else if ( 1 == pStepInfo->ucLampOn[ucLightLamp+2] )
				{
					m_ucGaColor[ucDirIndex][ucLaneIndex] = GA_COLOR_GREEN;
					ucLightLamp = ucLightLamp + 2;
				}
				m_bGaNeedCntDown[ucDirIndex][ucLaneIndex] = true; //��λ�������з�����λ��ͨ�� �����и�����λ�и���ͨ��
				m_ucGaRuntime[ucDirIndex][ucLaneIndex]    = GaGetCntTime(ucLightLamp); //��ȡʣ��ʱ��
				if(pConfig->sSpecFun[FUN_COUNT_DOWN].ucValue == 2)
								{				
					for(Byte ucIndex = 0;ucIndex < MAX_CNTDOWNDEV;ucIndex++ )
					{
						if((pConfig->sCntDownDev[ucIndex].usPhase == ucPhaseId )||(pConfig->sCntDownDev[ucIndex].ucOverlapPhase == ucPhaseId ))  
						{							
							if(((pConfig->sCntDownDev[ucIndex].ucMode>>3)&0xf) == m_ucGaRuntime[ucDirIndex][ucLaneIndex])
							{
								pConfig->sCntDownDev[ucIndex].ucMode |= 1<<7 ;
								
								//ACE_OS::printf("\n%s:%d cntdown phase=%d ,color =%d , ucMode =%d \n",__FILE__,__LINE__,ucPhaseId,m_ucGaColor[ucDirIndex][ucLaneIndex],pConfig->sCntDownDev[ucIndex].ucMode);
								break ;
							}
						}	
					}					
				}
			}
			else
			{
				m_bGaNeedCntDown[ucDirIndex][ucLaneIndex] = false;//�÷���������λ��ͨ�����޵���ʱ��������λ�޸���ͨ�����޵���ʱ
			}
			
		}
	}
}


/**************************************************************
Function:       CGaCountDown::GaSendStepPer
Description:    ÿһ�β�����ʼ����һ��
Input:          ��              
Output:         ��
Return:         ��
***************************************************************/
void CGaCountDown::GaSendStepPer()
{
	CGbtMsgQueue *pMsgQueue = CGbtMsgQueue::CreateInstance();
	Byte SendClientData[SEND_CLIENT_CNTOWN_BYTE] ={0};
	SendClientData[0] = 0x86 ;
	SendClientData[1] = 0xe6 ;
	SendClientData[2] = 0x0 ;
	
	
	GaGetCntDownInfo();
	GaSetSendBuffer();

	for ( int iDir=0; iDir<GA_MAX_DIRECT; iDir++ )
	{
		CRs485::CreateInstance()->Send(m_sGaSendBuf[iDir], GA_MAX_SEND_BUF);
	}
	
	ACE_OS::memcpy((Byte*)SendClientData+3,m_sGaSendBuf,GA_MAX_DIRECT*GA_MAX_SEND_BUF);
	//ACE_DEBUG((LM_DEBUG,"%s:%d After send 485, then send to client!\n",__FILE__,__LINE__));
	for(Byte iClinetDex = 0 ;iClinetDex<MAX_CLIENT_NUM ;iClinetDex++)
	{		 
		if(sSendCDN[iClinetDex].bSend== true && sSendCDN[iClinetDex].bUsed == true )
		{
	 //	ACE_DEBUG((LM_DEBUG,"%s:%d Cilinet[%d] IP:%s port:%d  bSend=%d bUsed =%d !\n",__FILE__,__LINE__,iClinetDex,sSendCDN[iClinetDex].addClient.get_host_addr(),sSendCDN[iClinetDex].addClient.get_port_number(),sSendCDN[iClinetDex].bSend,sSendCDN[iClinetDex].bUsed));
		(pMsgQueue->m_sockLocal).send(SendClientData,SEND_CLIENT_CNTOWN_BYTE,sSendCDN[iClinetDex].addClient);	 	
		
		}
	}
}


/**************************************************************
Function:       CGaCountDown::GaSetSendBuffer
Description:    ���췢������
Input:          ��              
Output:         ��
Return:         ��
***************************************************************/
void CGaCountDown::GaSetSendBuffer()
{
	Byte ucDirIndex  = 0;
	Byte ucColor     = GA_COLOR_RED;
	Byte ucCountTime = 0;

	STscConfig* pTscCfg = CManaKernel::CreateInstance()->m_pTscConfig;

	char sDirec[4][48] = {"NORTH","EAST","SOUTH","WEST"};
	char sColor[3][48] = {"RED","YELLOW","GREEN"};

	if ( ( (pTscCfg->sSpecFun[FUN_PRINT_FLAGII].ucValue) & 1 ) != 0 )
	{
		ACE_DEBUG((LM_DEBUG,"\n%s:%d ",GACOUNTDOWN_CPP, __LINE__));
	}
	
	for ( ucDirIndex=0; ucDirIndex<GA_MAX_DIRECT; ucDirIndex++ )
	{
		//��ȡĳһ���򵹼�ʱ�Ƶ���ɫ������
		ComputeColorCount(ucDirIndex,ucColor,ucCountTime);
		if ( ( (pTscCfg->sSpecFun[FUN_PRINT_FLAGII].ucValue) & 1 ) != 0 )
		{
			ACE_DEBUG((LM_DEBUG," %s-%s-%d",sDirec[ucDirIndex],sColor[ucColor],ucCountTime));
		}
	
		//�������
		PackSendBuf(ucDirIndex,ucColor,ucCountTime);
		
	}

	
	
}


/**************************************************************
Function:       CGaCountDown::ComputeColorCount
Description:    ���ݷ������ĳ����ĵ���ʱ��ɫ��������,����������
				����ʱ���ȼ������̵��� ���̵��� ͬ��ͬ��ͬ�쵹��С
Input:          ucDirIndex-����(0 1 2 3)              
Output:         ucColor-��ɫ(r y g)
		        ucCountTime-����
Return:         false-ʧ��  true-�ɹ�
***************************************************************/
bool CGaCountDown::ComputeColorCount( Byte ucDirIndex , Byte& ucColor , Byte& ucCountTime )
{
	bool bRed           = false;
	bool bYellow        = false;
	bool bGreen         = false;
	Byte ucLaneIndex    = 0;
	Byte ucMinRedCnt    = 254;
	Byte ucMinYellowCnt = 254;
	Byte ucMinGreenCnt  = 254;

	if ( ucDirIndex >= GA_MAX_DIRECT )
	{
		return false;
	}

	//�� ֱ  ��  ��������
	for ( ucLaneIndex=0; ucLaneIndex<GA_MAX_LANE; ucLaneIndex++ )
	{
		if ( false == m_bGaNeedCntDown[ucDirIndex][ucLaneIndex] ) 
		{
			continue;
		}
		if(ucLaneIndex == GA_LANE_FOOT || ucLaneIndex == GA_LANE_SECOND_FOOT)
			continue ;
		switch ( m_ucGaColor[ucDirIndex][ucLaneIndex] )
		{
			case GA_COLOR_GREEN:
				bGreen = true;
				if ( ucMinGreenCnt > m_ucGaRuntime[ucDirIndex][ucLaneIndex] )
				{
					ucMinGreenCnt = m_ucGaRuntime[ucDirIndex][ucLaneIndex]; 
				}
				break;
			case GA_COLOR_YELLOW:
				bYellow = true;
				if ( ucMinYellowCnt > m_ucGaRuntime[ucDirIndex][ucLaneIndex] )
				{
					ucMinYellowCnt = m_ucGaRuntime[ucDirIndex][ucLaneIndex]; 
				}
				break;
			case GA_COLOR_RED:
				bRed = true;
				if ( ucMinRedCnt > m_ucGaRuntime[ucDirIndex][ucLaneIndex] )
				{
					ucMinRedCnt = m_ucGaRuntime[ucDirIndex][ucLaneIndex]; 
				}
				break;
			default:
				break;
		}
	}

	if ( bGreen )  //�̵Ƶ���ʱ
	{
		ucColor     = GA_COLOR_GREEN;
		ucCountTime = ucMinGreenCnt;
	}
	else if ( bYellow )  //�粻���Ƶ� �ڴ˴�������
	{
		ucColor     = GA_COLOR_YELLOW;
		ucCountTime = ucMinYellowCnt;
	}
	else if ( bRed )
	{
		ucColor     = GA_COLOR_RED;
		ucCountTime = ucMinRedCnt;
	}
	else
	{
		return false;
	}

	return true;
}


/**************************************************************
Function:       CGaCountDown::PackSendBuf
Description:    ������͵�����
Input:          ucDirIndex-����(0 1 2 3)              
Output:         ucColor-��ɫ(0r 1y 2g)
*        		ucCountTime-����
Return:         false-ʧ��  true-�ɹ�
***************************************************************/
bool CGaCountDown::PackSendBuf( Byte ucDirIndex , Byte ucColor , Byte ucCountTime )
{
	const Byte ucFrameHead = 0xFE;    //�涨֡ͷ
	static Byte sColorGaDef[4] = {3,2,1,0};  //index: 0-r 1-y 2-g 3-��
	                                         //value: 3-r 2-y 1-g 0-��
	static Byte sDirGaDef[8]= {0,1,2,3,4,5,6,7};  //index��0-�� 1-�� 2-�� 3-�� 4 5 6 7
				                      			  //value: 0-�� 1-�� 2-�� 3-�� 4 5 6 7
	Byte ucColorDir = 0;
	Byte ucThousand = 0;
	Byte ucHundred  = 0;
	Byte ucTen      = 0;
	Byte ucEntries  = 0;
	Byte ucCheckSum = 0;

	if ( ucDirIndex > GA_MAX_DIRECT	|| ucColor > 4 )
	{
		return false;
	}

	ACE_OS::memset(m_sGaSendBuf[ucDirIndex] , 0 , GA_MAX_SEND_BUF);

	m_sGaSendBuf[ucDirIndex][0] = ucFrameHead;   //֡ͷ

	ucColorDir |= sColorGaDef[ucColor];       //D0-D1 ��ʾ��ɫ
	ucColorDir |= (sDirGaDef[ucDirIndex]<<2); //D2-D4 ��ʾ����
	ucColorDir |= 0x20;                       //D5λΪ1  D6-D7��չ
	m_sGaSendBuf[ucDirIndex][1] = ucColorDir;

	ucThousand = ucCountTime / 1000;
	ucHundred  = (ucCountTime %1000)/100;
	ucTen      = ((ucCountTime%1000)%100)/10;
	ucEntries  = ucCountTime % 10;
	m_sGaSendBuf[ucDirIndex][2] |= ucHundred;
	m_sGaSendBuf[ucDirIndex][2] |= (ucThousand<<4);
	m_sGaSendBuf[ucDirIndex][3] |= ucEntries;
	m_sGaSendBuf[ucDirIndex][3] |= (ucTen<<4);

	ucCheckSum = 0x7f & (m_sGaSendBuf[ucDirIndex][1] ^ m_sGaSendBuf[ucDirIndex][2] ^ m_sGaSendBuf[ucDirIndex][3]);
	m_sGaSendBuf[ucDirIndex][4] = ucCheckSum;
	ACE_DEBUG((LM_DEBUG,"%s:%d Direc=%d Color=%d CntDownTime=%d 485:FE %02x %02x %02x %02x   \n",__FILE__,__LINE__,ucDirIndex,ucColor,ucCountTime,m_sGaSendBuf[ucDirIndex][1], m_sGaSendBuf[ucDirIndex][2], m_sGaSendBuf[ucDirIndex][3], m_sGaSendBuf[ucDirIndex][4]));
	return true;
}


/**************************************************************
Function:       CGaCountDown::GaGetCntTime
Description:    ����ͨ����ȡ��ͨ��ͬһ��ɫʣ��ĳ���ʱ��
Input:          ucSignalId: ͨ��Id  1-16            
Output:         ��
Return:         �õ�ɫ��ʣ��ʱ��
***************************************************************/
Byte CGaCountDown::GaGetCntTime(Byte ucSignalId)
{
	STscRunData* pRunData  = CManaKernel::CreateInstance()->m_pRunData;
	Byte ucCurStep         = pRunData->ucStepNo;	
	SStepInfo*   pStepInfo = NULL;	
	Byte ucRuntime         = 0;
	
	if ( pRunData->ucStepTime >= pRunData->ucElapseTime )
	{
		ucRuntime = pRunData->ucStepTime - pRunData->ucElapseTime;
	}
	else
	{
		ucRuntime = 0;
	}

	//���������õ�ɫ������������ͨ���ĺ�ƿ��ܻ����ͨ�����̵Ʋ�����������ʱ��Ҳһֱ��
	for (int iStep = ((ucCurStep+1) % pRunData->ucStepNum); iStep != ucCurStep;iStep = (iStep + 1) % pRunData->ucStepNum) 
	{
		pStepInfo = pRunData->sStageStepInfo + iStep;
		if ( 1 == pStepInfo->ucLampOn[ucSignalId] ) 
		{
			ucRuntime += pStepInfo->ucStepLen;
		} 
		else 
		{
			break;
		}
	}

	return ucRuntime;
}


/**************************************************************
Function:       CGaCountDown::GaUpdateCntDownCfg
Description:    ���µ���ʱ���ò��� ���ݿ����ʱ��Ҫ����
Input:          ��         
Output:         ��
Return:         ��
***************************************************************/
void CGaCountDown::GaUpdateCntDownCfg()
{
	Byte ucRecordCnt = 0;
	Byte ucDirIndex  = 0;
	Byte ucLaneIndex = 0;
	Byte iIndex      = 0;

	GBT_DB::PhaseToDirec*   pPhaseToDirec = NULL;    
	GBT_DB::TblPhaseToDirec tTblPhaseToDirec;

	/******��������λ��Ӧ��******/
	(CDbInstance::m_cGbtTscDb).QueryPhaseToDirec(tTblPhaseToDirec);
	pPhaseToDirec = tTblPhaseToDirec.GetData(ucRecordCnt);
	ACE_OS::memset(m_sGaPhaseToDirec  , 0, (GA_MAX_DIRECT*GA_MAX_LANE)*sizeof(GBT_DB::PhaseToDirec));
	iIndex = 0;
	while ( iIndex < ucRecordCnt )
	{
		if(pPhaseToDirec[iIndex].ucPhase ==0 && pPhaseToDirec[iIndex].ucOverlapPhase == 0)			
		{
			iIndex++;
			continue ;
		}
		ACE_DEBUG((LM_DEBUG,"%s:%d Direc=%d phase=%d overlapphase=%d \n",__FILE__,__LINE__,pPhaseToDirec[iIndex].ucId,pPhaseToDirec[iIndex].ucPhase,pPhaseToDirec[iIndex].ucOverlapPhase));
		GaGetDirLane(pPhaseToDirec[iIndex].ucId , ucDirIndex , ucLaneIndex );
		if ( ucDirIndex < GA_MAX_DIRECT && ucLaneIndex < GA_MAX_LANE )
		{
			ACE_OS::memcpy(&m_sGaPhaseToDirec[ucDirIndex][ucLaneIndex],&pPhaseToDirec[iIndex],sizeof(GBT_DB::PhaseToDirec));
		}
		iIndex++;
	}
}


/**************************************************************
Function:       CGaCountDown::GaUpdateCntDownCfg
Description:    ��ȡ������ת��  �磺�� ֱ
Input:          ucTableId - ��������λ���id         
Output:         ucDir     - ����
*        		ucLane    - ת��
Return:         ��
***************************************************************/
void CGaCountDown::GaGetDirLane(Byte ucTableId , Byte& ucDir , Byte& ucLane)
{	

	Byte ucDirCfg        = (ucTableId >> 5) & 0x07;
	Byte ucCrossWalkFlag = (ucTableId >> 3) & 0x03;
	Byte ucVelhileFlag   = ucTableId & 0x07;

	switch ( ucDirCfg )
	{
		case 0:
			ucDir = GA_DIRECT_NORTH; //��
			break;
		case 2:
			ucDir = GA_DIRECT_EAST; //��
			break;
		case 4:
			ucDir = GA_DIRECT_SOUTH; //��
			break;
		case 6:
			ucDir =  GA_DIRECT_WEST; //��
			break;
		case 1:
			ucDir = GA_DIRECT_NORTH_EAST ; //
			break ;
		case 3:
			ucDir = GA_DIRECT_SOUTH_EAST ;
			break ;
		case 5:
			ucDir = GA_DIRECT_SOUTH_WEST;
			break ;
		case 7:
			ucDir = GA_DIRECT_NORTH_WEST ;
		default:
			ucDir  = GA_MAX_DIRECT;
			break;
	}
	
	if ( ucCrossWalkFlag ==1)
	{
		ucLane = GA_LANE_FOOT;
	}
	else if(ucCrossWalkFlag ==2)
	{
		ucLane = GA_LANE_SECOND_FOOT ;
	}
	else if ( ucVelhileFlag == 0x0 )
	{
		ucLane = GA_LANE_TRUNAROUND;
	}
	else if ( ucVelhileFlag == 0x1 )
	{
		ucLane = GA_LANE_LEFT;
	}
	else if ( ucVelhileFlag == 0x2 )
	{
		ucLane = GA_LANE_STRAIGHT;
	}
	else if(ucVelhileFlag == 0x3)
	{

		ucLane = GA_LANE_LEFT_STRAIGHT ;
	}
	else if ( ucVelhileFlag == 0x4 )
	{
		ucLane = GA_LANE_RIGHT;
	}
	else if ( ucVelhileFlag == 0x5 )
	{
		ucLane = GA_LANE_OTHER;
	}
	else if(ucVelhileFlag == 0x6)
	{
		ucLane = GA_LANE_RIGHT_STRAIGHT;
	}
	else if(ucVelhileFlag == 0x7)
	{
		ucLane = GA_LANE_LEFT_RIGHT_STRAIGHT;
	}
}

/**************************************************************
Function:       CGaCountDown::sendblack
Description:    ���ڷ��͵���ʱ����
Input:          ��         
Output:         ��
Return:         ��
***************************************************************/
void CGaCountDown::sendblack()
{
	
	for ( Byte iDir=0; iDir< GA_MAX_DIRECT; iDir++ )
	{
		PackSendBuf( iDir,GA_COLOR_BLACK , 199) ;			
		CRs485::CreateInstance()->Send(m_sGaSendBuf[iDir], GA_MAX_SEND_BUF);		
	}

}

/**************************************************************
Function:       CGaCountDown::send8cntdown
Description:    ���ڸ�Ӧ����ʱ��8�뵹��ʱ����
Input:          ��         
Output:         ��
Return:         ��
***************************************************************/
void CGaCountDown:: send8cntdown()
{
	CManaKernel * p_ManaKernel = CManaKernel::CreateInstance();
	
	bool bSend8CntDwon[GA_MAX_DIRECT] ={false};

	Byte iStepNum = p_ManaKernel->m_pRunData->ucStepNo ;
	Uint iAllowPhase = p_ManaKernel->m_pRunData->sStageStepInfo[iStepNum].uiAllowPhase ;
	Byte iNdex = 0 ;
	Byte iColor = 0 ;
	Byte iCostTime = 0 ;
	ACE_OS::printf("%s:%d StepNum= %d iAllowPhase = %d \n",__FILE__,__LINE__,iStepNum,iAllowPhase);
	
	while(iNdex < MAX_PHASE)
	{
		if ( (iAllowPhase>>iNdex) & 1 )
		{
			for(Byte iDirec = 0 ;iDirec < GA_MAX_DIRECT ;iDirec++)
			{
				for(Byte iLan = 0 ;iLan < (GA_MAX_LANE-1) ;iLan++)
				{
					if((iNdex+1) == m_sGaPhaseToDirec[iDirec][iLan].ucPhase)
					{
						bSend8CntDwon[iDirec] = true ;
						ACE_OS::printf("%s:%d Phase= %d Direc = %d \n",__FILE__,__LINE__,iNdex+1,iDirec);
					}
				}
			}
				
		}

		iNdex++;
	}
	iNdex = 0 ;
	ACE_OS::printf("%s:%d Begin Send 8 cntdown \n",__FILE__,__LINE__);
	 while(iNdex < GA_MAX_DIRECT)
	 {
		if(bSend8CntDwon[iNdex] == true)
		{
			PackSendBuf( iNdex , GA_COLOR_GREEN , 8 ) ;			
			CRs485::CreateInstance()->Send(m_sGaSendBuf[iNdex], GA_MAX_SEND_BUF);
		}
		else
		{			
			ComputeColorCount(iNdex,iColor,iCostTime);
			PackSendBuf( iNdex , GA_COLOR_RED , iCostTime-8 ) ;			
			CRs485::CreateInstance()->Send(m_sGaSendBuf[iNdex], GA_MAX_SEND_BUF);
		}
		iNdex++;	
	 }
			
}


/**************************************************************

Function:       CGaCountDown::SetClinetCntDown
Description:    ������������λ�����͸������򵹼�ʱ��Ϣ
Input:          ��         
Output:         ��
Return:         ��
***************************************************************/
void CGaCountDown::SetClinetCntDown(ACE_INET_Addr& addremote, Uint uBufCnt , Byte* pBuf)
	
{
	Byte ucSndType  = pBuf[3]; 
	Byte ucRecvOptType = pBuf[0] & 0xf;
	Byte iClientdex = 0 ;
	Byte sErrMsg[3] = {0x86 , GBT_ERROR_OTHER , 0};
	CGbtMsgQueue* pGbtMsgQueue = CGbtMsgQueue::CreateInstance();
	if(uBufCnt >4)
	{		
		pGbtMsgQueue->m_sockLocal.send(sErrMsg ,3, addremote);
		return ;
	}

	for( ;iClientdex<MAX_CLIENT_NUM;iClientdex++)
	{
		if(strcmp(sSendCDN[iClientdex].addClient.get_host_addr(),addremote.get_host_addr()) != 0 && sSendCDN[iClientdex].bUsed ==true)
		{
			continue ;
		}
		else 
		{	
			sSendCDN[iClientdex].addClient.set(ACE_INET_Addr(addremote));
			sSendCDN[iClientdex].bSend = ucSndType ;
			ucSndType == 1?sSendCDN[iClientdex].bUsed =true:sSendCDN[iClientdex].bUsed =false ;
			break;
		}

	}
	if(iClientdex == MAX_CLIENT_NUM)
	{
		pGbtMsgQueue->m_sockLocal.send(sErrMsg ,3, addremote);
		return ;
	}
	else if ( GBT_SET_REQ == ucRecvOptType || GBT_SET_REQ_NOACK == ucRecvOptType )  //����
	{
		//��ȷ��Ӧ��
		if ( GBT_SET_REQ == ucRecvOptType )
		{
			
			Byte sSendMsg[4] = {0};

			sSendMsg[0] = ( pBuf[0] & 0xf0 ) | GBT_SET_ACK;
			sSendMsg[1] = pBuf[1];
			sSendMsg[2] = pBuf[2];
			pGbtMsgQueue->m_sockLocal.send(sSendMsg ,3, addremote);
		}
	}
	else 
	{
		pGbtMsgQueue->m_sockLocal.send(sErrMsg ,3, addremote);
	}

}
	
/***************************************************************

Function:       CGaCountDown::GaSendStepPer
Description:    ����ʽ����ʱÿ���жϵ���ʱʱ��
Input:          	��              
Output:         ��
Return:          ��
***************************************************************/
void CGaCountDown::GaGetDirecColorTime()
{
	GaGetCntDownInfo();	
}
	



