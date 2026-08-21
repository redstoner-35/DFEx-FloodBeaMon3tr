/************************************************************************************/
/** \file ChgMgmt.h
/** \Author redstoner_35
/** \Project Xtern Ripper Laser Edition 
/** \Description 这个文件为充电管理中层模块相关的处理逻辑

**	History: Initial Release
**	
/************************************************************************************/
#ifndef _ChgMgmt_
#define _ChgMgmt_
/************************************************************************************/
/* Global type definiton - (typedef) */
/************************************************************************************/
//状态变量enum
typedef enum
	{
	Battery_Plenty, //电池电量充足
	Battery_Mid, //电池电量较为充足
	Battery_Low, //电池电量不足
	Battery_VeryLow //电池电量严重不足
	}BattStatusDef;
	
typedef enum
	{
  BattVdis_Waiting, //等待显示阶段
	BattVdis_PrepareDis, //准备显示
	BattVdis_DelayBeforeDisplay, //延迟一段时间
	BattVdis_Show10V, //显示十位
	BattVdis_Gap10to1V, //十位和个位之间的等待
	BattVdis_Show1V, //显示个位
	BattVdis_Gap1to0_1V, //个位和十分位之间的等待
	BattVdis_Show0_1V, //显示小数点后一位(0.1V)
	BattVdis_WaitShowChargeLvl, //等待一段时间后显示当前电量
	BattVdis_ShowChargeLvl, //显示电池电量的等待
	BattVdis_WaitShowTempState,
	BattVdis_ShowTempState	
	}BattVshowFSMDef; //电池电量显示处理	
	
typedef enum
	{
	ConsDisplay_Waiting, //等待一致性显示启动阶段
	ConsDisplay_PrePareValueReport, //准备数值显示
	ConsDisplay_WaitReportFinish,   //等待数值触发
	ConsDisplay_DelayALittle,       //延时一小会显示一致性最差的电池
	ConsDisplay_GetherBadBattery,   //获取一致性最差的电池
	ConsDisplay_ShowBatteryCount,   //显示电池数目	
	ConsDisplay_DelayAgain         //显示结束后再延迟一小会
	}ConsDisplayFSMDef;	
	
typedef enum
	{
	BattCons_OK,   //电池一致性良好(<60mV)
	BattCons_Mid,  //电池一致性一般(70到200mV)
	BattCons_Bad   //电池一致性非常非常差(200mV以上)
	}BattConsistencyDef;
	
typedef enum
	{
	Charge_Enabled,   //正常开启充电
	Charge_DisableUnderTemp,  //因为电池温度过低，充电被禁用
	Charge_DisableOverTemp,   //因为电池温度过高，充电被禁用
	Charge_DisableOverChg,     //因为电池一致性太差且单体过冲，充电被禁用	
	Charge_DisableBMSFault,    //BMS故障
	Charge_DisableNTCFault,		 //NTC故障
	Charge_DisableUserCommand, //用户执行禁充
	}ChargeDisableReason; //充电关闭的原因	
	
typedef enum
	{
	BAL_Disable,
	BAL_EnableAuto,       //由于电池一致性太差，均衡被启动
	BAL_EnableManual,     //均衡自动启动
	}BALEnableReason;	
	
typedef enum
	{
	ACT_Command_None,   //没指令
	ACT_Command_Start,  //单击唤醒2366
	ACT_Command_Stop,   //产生双击强制关闭2366
	ACT_Command_Reset,   //长按11秒，强制复位SOC
	ACT_Command_Active  //上电过程中激活4秒
	}ACTCommandDef;	
	
typedef struct
	{
	BattStatusDef BattState[4]; //四节电池的状态
	BattConsistencyDef ConsState; //一致性状态	
	BattStatusDef TotalBattState; //四节电池总电压的状态
	ChargeDisableReason ChargeState;		 				 //禁止充电
	BALEnableReason BalState;        //开启均衡
	
	}SysMgmtStoreDef;	
	
/************************************************************************************/
/* Extern Functions definition - Initialization and Special Operation */
/************************************************************************************/
void ChargeMgmt_Init(void); //初始化充电管理	
	
/************************************************************************************/
/* Extern Functions definition - Callback and Logic Handler */
/************************************************************************************/	
void ChargeMgmt_MainLEDHandler(void); //主LED控制器	
void ChargeMgmt_TIMHandler(void);     //计时处理	
void ChargeMgmtKeyLogic(void); //按键操作处理	
void ACTCommandLogicHandler(void);  //处理逻辑控制
void ChargeMgmt_BalHandler(void); //充电平衡检测处理	
void INFOTimerHandler(void);  //待机状态下每5秒提示一次用户的定时处理
	
#endif	/* _ChgMgmt_ */

/*********************************  End Of File  ************************************/