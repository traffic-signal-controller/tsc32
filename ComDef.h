#ifndef __COMDEF__H__
#define __COMDEF__H__

#include "Define.h"


/*
Can报文类型 占据ID识别符 bit26-bit28
*/
enum
{
	CAN_MSG_TYPE_000 = 0, //最高实时性  保留紧急使用
	CAN_MSG_TYPE_001 = 1, //灯色和绿冲突，灯泡检测
	CAN_MSG_TYPE_010 = 2, //灯色和绿冲突，灯泡检测
	CAN_MSG_TYPE_011 = 3, //灯色和绿冲突，灯泡检测
	CAN_MSG_TYPE_100 = 4, //检测数据、手控数据和电源板数据
	CAN_MSG_TYPE_101 = 5, //配置数据的发送，如检测器故障状态，检测器测速绑定
	CAN_MSG_TYPE_110 = 6, //没有实时性要求，系统保留后期模块连接
	CAN_MSG_TYPE_111 = 7  //本系统中被禁用
};

/*
*模块地址 占据ID识别符bit20-bit25 目的地址 占据ID识别符bit12-bit17
*/
enum
{
	BOARD_ADDR_MAIN         = 0x10 ,  //主控板
	BOARD_ADDR_LAMP1        = 0x13 ,  //灯控板1
	BOARD_ADDR_LAMP2        = 0x14 ,  //灯控板2
	BOARD_ADDR_LAMP3        = 0x15 ,  //灯控板3
	BOARD_ADDR_LAMP4        = 0x16 ,  //灯控板4
	BOARD_ADDR_LAMP5        = 0x17,   //灯控板5 
	BOARD_ADDR_ALLLAMP    = 0x19 ,  //灯控板组播
	BOARD_ADDR_POWER        = 0x20 ,  //电源模块
	BOARD_ADDR_DETECTOR1    = 0x24 ,  //检测器1
	BOARD_ADDR_DETECTOR2    = 0x25 ,  //检测器2
	BOARD_ADDR_ALLDETECTOR  = 0x26 ,  //检测器组播
	BOARD_ADDR_INTEDET1     = 0x29 ,  //检测器接口1
	BOARD_ADDR_INTEDET2     = 0x2A ,  //检测器接口2
	BOARD_ADDR_ALLINTEDET   = 0x2B ,  //检测器接口板组播
	BOARD_ADDR_FLASH        = 0x2E ,  //硬件黄闪器
	BOARD_ADDR_HARD_CONTROL = 0x30 ,  //硬件控制模块
	BOARD_ADDR_LED          = 0x32    //显示灯组 ADD:20130808 15 50
};

/*
*帧模式  占据ID识别符bit18-bit19
*/
enum
{
	FRAME_MODE_P2P        = 0 , //点对点
	FRAME_MODE_MULTICAST  = 1 , //组播
	FRAME_MODE_BROADCAST  = 2 , //广播
	FRAME_MODE_HEART_BEAT = 3   //广播心跳
};


/*
*GBT应用层协议数据类型
*/
enum
{
	DATA_HEAD_NOREPLY = 0 , //通用数据，不需要回复
	DATA_HEAD_RESEND  = 1 , //请求发送
	DATA_HEAD_CHECK   = 2 , //确认请求
	DATA_HEAD_OTHER   = 3   //未定义，保留
};

/* DELETE 20130920
const int DET_STAT_CYCLE = 60;   //占有率统计的周期 1分钟
enum
{
	DETECTOR_INTERFACE_BOARD1 = 0,  //检测器接口板1地址
	DETECTOR_INTERFACE_BOARD2 = 1 ,  //检测器接口板2地址
	DETECTOR_BOARD1           = 2  ,  //检测器板1
	DETECTOR_BOARD2           = 3     //检测器板2
};
*/





/************************************************************************************************/
/*
*信号机系统常量值定义，对应GBT协议部分常量对象
*/
const int MAX_TIMEGROUP          = 40;   //最大时基调度表数
const int MAX_SCHEDULE_PER_DAY   = 48;   //每天最大时段数
const int MAX_TIMEPATTERN        = 32;   //最大的配时方案表
const int MAX_SCHEDULETIME_TYPE  = 17;   //1-16 用户定义，17用于特殊方案 阶段配时方案类型???
const int MAX_SON_SCHEDULE       = 16;   //最大阶段配时表里的子阶段数
const int MAX_PHASE              = 32;   //最大相位表数
const int MAX_OVERLAP_PHASE      = 16;    //最大跟随相位表数
const int MAX_CONFLICT_PHASE     = 32;   //最大冲突表数
const int MAX_STATUSOUT_PER      = 8;    //每组状态输出包含的个数
const int MAX_STEP               = 64;   //最大的步伐数
const int MAX_LAMP_BOARD         = 8;    //最大灯驱板数
const int MAX_LAMP_NUM_PER_BOARD = 12;   //每块板的灯具数量
const int MAX_LAMPGROUP_PER_BOARD =4 ;
const int MAX_CHANNEL            = MAX_LAMP_BOARD * 4;                        //最大通道（信号组）表数 1板4通道
const int MAX_LAMP               = MAX_LAMP_BOARD * MAX_LAMP_NUM_PER_BOARD;   //最大灯具数  1通道3灯具 1板12灯具
const int MAX_DET_BOARD          = 4;    //最大检测器和接口板数量，2块检测其板两块接口板
const int MAX_DETECTOR_PER_BOARD = 16;   //每块检测器板包含的检测器通道数量
const int MAX_INTERFACE_PER_BOARD =32;  //每块接口板包含的通道数量
const int MAX_DETECTOR           = 2*(MAX_DETECTOR_PER_BOARD+MAX_INTERFACE_PER_BOARD) ;//MAX_DET_BOARD * MAX_DETECTOR_PER_BOARD;   //最大检测器数量

const int MAX_SPESTATUS_CYCLE    = 10;   //时段表里定义的特殊状态周期时长 
const int MIN_GREEN_TIME	     = 7;    //最小绿灯时长

const int MAX_REGET_TIME         = 10;   //100ms 1per 1s=10times
const int USLEEP_TIME            = 2000;
const int MAX_WORK_TIME          = 3;    //3次一样的才起作用
const int BOARD_REPEART_TIME     = 5;    //板状态判断的重复次数
const int MAX_DREC               = 80;  //最大方向数量   201310181705
const Byte MAX_CNTDOWNDEV        = MAX_PHASE ;  //支持最大的倒计时设备数32,相位倒计时

/************************************************************************************************/

/*
信号机的工作模式
*/
enum
{
	MODE_TSC  = 0 , //tsc模式
	MODE_PSC1 = 1 , //一次过街模式
	MODE_PSC2 = 2 , //二次过街模式
	MODE_OTHER =3   //其他待定模式
};

/*
信号机的工作状态
*/
enum
{
	SIGNALOFF = 0,  //关灯
	ALLRED       ,  //全红
	FLASH        ,  //闪光
	STANDARD        //标准
};

/*
信号机的控制模式
*/
enum
{
	CTRL_UNKNOWN         = 0  ,  //未知控制状态
	CTRL_SCHEDULE        = 1  ,  //多时段
	CTRL_UTCS            = 2  ,  //系统优化，即联网
	CTRL_WIRELESS        = 3  ,  //无电线缆协调
	CTRL_LINE            = 4  ,  //有线电缆协调
	CTRL_HOST            = 5  ,  //主从线控
	CTRL_MANUAL          = 6  ,  //手动控制
	CTRL_VEHACTUATED     = 7  ,  //单点全感应
	CTRL_MAIN_PRIORITY   = 8  ,  //单点主线优先半感应
    CTRL_SECOND_PRIORITY = 9  ,  //单点次线优先半感应
	CTRL_ACTIVATE        = 10 ,  //自适应
	CTRL_PANEL           = 11 ,  //面板控制
	CTRL_LAST_CTRL       = 12    //上次的控制方式
};

/*
信号机控制队列消息枚举类型
*/
enum
{
	TSC_MSG_SWITCH_STATUS = 0,  //工作状态切换
	TSC_MSG_SWITCH_CTRL      ,  //控制方式切换
	TSC_MSG_UPDATE_PARA      ,  //数据库得到更新
	TSC_MSG_NEXT_STEP        ,  //步进命令
	TSC_MSG_OVER_CYCLE       ,  //周期结束
	TSC_MSG_LOCK_STEP        ,  //锁定步伐
	TSC_MSG_LOCK_PHASE       ,  //锁定相位
	TSC_MSG_LOCK_STAGE       ,  //锁定阶段
	TSC_MSG_NEXT_STAGE       ,  //下一个阶段
	TSC_MSG_STATUS_READ      ,  //状态获取
	TSC_MSG_EXSTATUS_READ    ,  //扩展对象获取
	TSC_MSG_EVENT_READ       ,  //事件读取
	TSC_MSG_LOG_WRITE        ,  //日志记录
	TSC_MSG_CORRECT_TIME     ,  //时间校时  SpeTimePattern
	TSC_MSG_TIMEPATTERN      ,  //特定的时间方案
	TSC_MSG_GREENCONFLICT    ,   //绿冲突
	TSC_MSG_PATTER_RECOVER       //特定方案切换
};

/*
通信进程gbt处理队列消息类型枚举
*/
enum
{
	GBT_MSG_FIRST_RECV = 0    ,  //首次接收
	GBT_MSG_DEAL_RECVBUF      ,  //处理接收到的BUF
	GBT_MSG_SEND_HOST         ,  //发往上位机
	GBT_MSG_TSC_STATUS        ,  //得到信号机状态
	GBT_MSG_TSC_EXSTATUS      ,  //得到信号机扩展状态
	GBT_MSG_SELF_REPORT       ,  //主动上报
	GBT_MSG_OTHER_OBJECT      ,  //gbt协议的其他类型对象
	GBT_MSG_EXTEND               //扩展对象
};




/*
设备状态类型枚举
*/
enum
{	
	DEV_IS_GOOD      = 0,  //正常

	DEV_ALWAYS_ON    = 1,  //常亮
	DEV_ALWAYS_OFF   = 2,  //常灭

	DEV_SILICON_SHOT = 3,  //可控硅击穿

	DEV_ALWAYS_ON_CLEAR,
	DEV_ALWAYS_OFF_CLEAR,

	DEV_IS_BROKEN,
	DEV_IS_SHORT_CIRCUIT,

	DEV_IS_BROKEN_CLEAR,
	DEV_IS_SHORT_CIRCUIT_CLEAR,

	DEV_IS_CONNECTED,
	DEV_IS_DISCONNECTED
};

/*
*主控板显示灯状态类型枚举
*/
enum
{
	LED_OP_START   = 0 ,

	LED_RUN_ON     = 1 ,
	LED_RUN_OFF    = 2 ,

	LED_GPS_ON     = 3 ,
	LED_GPS_OFF    = 4 ,

	LED_RS485_ON   = 5 , 
	LED_RS485_OFF  = 6 
	
};

/*
*串口9位通信功能枚举
*/
enum
{
	com_9bit_save_old 	    = 0x11,
	com_9bit_enable         = 0x12,
	com_9bit_disable        = 0x13,
	com_9bit_restore_old    = 0x14
};

/*
*通道类型枚举
*/
enum
{
	CHANNEL_OTHER   = 1 ,  //其他类型通道
	CHANNEL_VEHICLE = 2 ,  //机动车通道
	CHANNEL_FOOT    = 3 ,  //行人通道
	CHANNEL_OVERLAP = 4    //跟随相位通道
};

/*倒计时类型设置*/
enum
{
	CNTDOWN_15 = 1 ,      //黑屏15秒倒计时
	CNTDOWN_8  = 2,       //8秒倒计时
	CNTDOWN_NORMAL       //普通倒计时
};

/*受控按钮状态*/
enum
{
	MAC_CTRL_NOTHING    = 0x00 , //未有任何手控操作
	MAC_CTRL_ALLOFF     = 0x01 , //灭灯
	MAC_CTRL_ALLRED     = 0x02 , //全红
	MAC_CTRL_FLASH      = 0x03 , //黄闪
	MAC_CTRL_NEXT_PHASE = 0x04 , //下一相位
	MAC_CTRL_NEXT_DIR   = 0x05 , //下一方向
	MAC_CTRL_NEXT_STEP  = 0x06 , //步进
	MAC_CTRL_OTHER      = 0x07 , //保留
	MAC_CTRL_MANUL      = 0x08

};
/*
*信号机特定功能类型枚举
*/
enum
{
	FUN_SERIOUS_FLASH  = 0  , //严重故障黄闪
	FUN_COUNT_DOWN     = 1  , //主动倒计时
	FUN_GPS            = 2  , //gps启用
	FUN_MSG_ALARM      = 3  , //短信报警
	FUN_CROSS_TYPE     = 4  , //过街方式 0-tsc 1-one psc 2-twice psc
	FUN_STAND_STAGEID  = 5  , //待机阶段号
	FUN_CORSS1_STAGEID = 6  , //行人通行阶段号1
	FUN_CROSS2_STAGEID = 7  , //行人通行阶段号2
	FUN_TEMPERATURE    = 8  , //温度计用户
	FUN_VOLTAGE        = 9  , //电压检测开关
	FUN_DOOR           = 10 , //门开关
	FUN_COMMU_PARA     = 11 , //通信接口
	FUN_PORT_LOW       = 12 , //端口号低字节
	FUN_PORT_HIGH      = 13 , //端口号高字节
	FUN_PRINT_FLAG     = 14 , //打印标志  0检测器 1发电源板   2收电源板 3发灯控板 4收灯控板 5发黄闪器 6收黄闪器 7步伐信息
	FUN_PRINT_FLAGII   = 15 , //打印标志  0倒计时 1StartTime 2CostTime
	FUN_CAM			   = 16 , // 摄像头
	FUN_3G             = 17 , //3G通信
	FUN_WLAN		   = 18 , //WLAN无线网络
	FUN_CNTTYPE	       = 19,  //倒计时类型
	FUN_COUNT                 // 总到特定功能数量值
};

/*
*信号及日志类型枚举
*/
enum 
{
	LOG_TYPE_LAMP         = 0 ,  //信号灯
	LOG_TYPE_GREEN_CONFIG = 1 ,  //绿冲突
	LOG_TYPE_DETECTOR     = 2 ,  //检测器
	LOG_TYPE_VOLTAGE      = 3 ,  //电压
	LOG_TYPE_TEMPERATURE  = 4 ,  //温度
	LOG_TYPE_POWERBOARD   = 5 ,  //电源板
	LOG_TYPE_LAMPBOARD    = 6 ,  //灯控板
	LOG_TYPE_DETBOARD     = 7 ,  //检测器板
	LOG_TYPE_FLASHBOARD   = 8 ,  //黄闪器板
	LOG_TYPE_REBOOT       = 9 ,  //程序重启
	LOG_TYPE_CORRECT_TIME = 10,  //系统时间修改
	LOG_TYPE_DOOR_WARN    = 11,  //门报警
	LOG_TYPE_MANUAL       = 12,  //机器手动
	LOG_TYPE_OUTLAMP_ERR  = 13,  //灯色输出异常
	LOG_TYPE_OUTLAMP_ERR2 = 14,  //灯色输出异常2
	LOG_TYPE_OTHER        = 15,
	LOG_TYPE_CAN                //CAN总线     
};

/*************************GBT协议消息处理常量定义************************/
const int MAX_GBT_MSG_LEN        = 484;   //gbt协议消息的最大长度
const int MIN_GBT_MSG_LEN        = 3;     //gbt协议消息的最小长度
const int MAX_CLIENT_NUM         = 4;     //最大的客户端连接数
const int DEFAULT_GBT_PORT       = 8801;  //默认开辟的端口号
const int DEFAULT_BROADCAST_PORT = 8808;  //默认开辟的端口号
const Uint MAX_GBT_PORT          = 10024; //最大端口号    
const Uint MIN_GBT_PORT          = 1024;  //最小端口号
const int MAX_BUF_LEN            = 8192;  //最大帧的长度
/*************************GBT协议消息处理常量定义************************/

/*
GBT协议数据包类型枚举
*/
enum 
{
	GBT_SEEK_REQ      = 0,  //查询请求
	GBT_SET_REQ       = 1,  //设置请求
	GBT_SET_REQ_NOACK = 2,  //设置请求，但不需确认应答

	GBT_SELF_REPORT   = 3,  //主动上报

	GBT_SEEK_ACK      = 4,  //查询应答
	GBT_SET_ACK       = 5,  //设置应答
	GBT_ERR_ACK       = 6,  //出错应答

	GBT_OTHER_ACK     = 7   //其他类型
};



/*
GBT错误类型枚举
*/
enum
{
	GBT_ERROR_LONG     = 1,  //消息太长
	GBT_ERROR_TYPE        ,  //消息类型
	GBT_ERROR_OBJECT_VALUE,  //对象值超过范围
	GBT_ERROR_SHORT       ,  //消息太短
	GBT_ERROR_OTHER          //其他错误
};


enum
{
	LED_BOARD_SHOW  = 2 

};
const int MAX_ADJUST_CYCLE = 3;




#endif  //__COMDEF__H__
