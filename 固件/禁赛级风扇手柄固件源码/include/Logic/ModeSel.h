/************************************************************************************/
/** \file ModeSel.h
/** \Author redstoner_35
/** \Project Xtern Ripper Hyper Fan Ultra Edition
/** \Description 这个头文件负责声明系统的风扇输出控制逻辑的接口变量以及初始化和业务逻辑。

**	History: Initial Release
**	
/************************************************************************************/
#ifndef _ModeSel_
#define _ModeSel_

/************************************************************************************/
/*	Global type definitions('typedef')
*************************************************************************************/
typedef enum
	{
	LVPROT_Disable=0,  //该挡位关闭低电量保护
	LVPROT_Enable_Jump=1, //该挡位低电量保护开启，当电量低于阈值后执行跳档
	LVPROT_Enable_OFF=2		//该挡位低电量保护开启，当电量低于阈值后立即执行关机
	}LVProtectTypeDef;	

typedef struct
	{
	float TurboMinimumDuty;		 //最低占空比
	float TurboMinimumVoltage; //最低电压
	float TurboCurrentVoltage; //当前电压
	float TurboCurrentDuty;  	 //当前占空比
	unsigned int FullSpeedTime;      	 //全速时间
  char TurboRefreshCount;        //极速睡眠刷新
	unsigned int TurboRefreshTIM; //极速睡眠刷新计时器
	}TurboTimedStepDownDef;	

typedef enum
	{
	Mode_OFF=0, //关机
  //常用的5个循环挡位
	Mode_UltraLow,
	Mode_Low,
	Mode_Mid,
	Mode_MHigh,
	Mode_High,
	//全功率爆发	
	Mode_Turbo,
	//无级调节
	Mode_Ramp,
	//全功率暴力模式
	Mode_Boost
	}ModeIdxDef;	

typedef struct
	{
  ModeIdxDef ModeIdx;
	int LowVoltThres; //低电压检测电压(mV)
	unsigned char Speed; //风扇速度百分比(%)
  float TargetVOUT; //目标的输出电压(仅CV模式有效)		
	//低电量保护设置
  ModeIdxDef ModeWhenLVAutoFall;		//低电量触发保护之后，如果不执行关机则自动跳转的挡位
	LVProtectTypeDef LVConfig;        //低电量保护机制的类型
	//挡位切换设置
  ModeIdxDef ModeTargetWhenH;
	ModeIdxDef ModeTargetWhen1H;	 //模式挡位切换设置，长按和单击+长按切换到的目标挡位
	}ModeStrDef; 

/************************************************************************************/
/* Extern Variable definition - Mode control Systems                                */
/************************************************************************************/
extern ModeStrDef *CurrentMode; //当前模式结构体
extern xdata ModeIdxDef LastMode; //上一个挡位	
extern xdata float RampVoltage; //无极调速目标电压
extern xdata float RampDuty; //无极调速目标占空比
	
/************************************************************************************/
/* Extern Bit Flag definition - Mode control Systems                                */
/************************************************************************************/	
extern bit IsSystemLocked;        //系统是否已经锁定	
extern bit IsEnableIdleLED;       //是否开启有源夜光
extern bit IsEnable2SMode;        //是否开启2S模式	
extern bit IsEnableBattCfgLock;   //是否开启电池配置锁	
	
/************************************************************************************/
/* Extern Functions definition - Initialization & Logic Handler                     */
/************************************************************************************/	
void ModeFSMInit(void); 										//初始化模式状态机
void ModeSwitchFSM(void); 									//挡位状态机	
void HoldSwitchGearCmdHandler(void);        //长按换挡处理
void RampConfigAutoSaveHandler(void); 		  //无极调速自动保存处理	

/************************************************************************************/
/* Extern Functions definition - Turbo timed stepdown related                       */
/************************************************************************************/	
void AddTurboRefreshCountWhenSleep(void); //睡眠过程中定时唤醒补充极亮强制刷新次数
void TurboTimedStepDownPROC(void); 				//极速挡位时控降档处理


/************************************************************************************/
/* Extern Functions definition - Mode System operation                              */
/************************************************************************************/	
void SwitchToGear(ModeIdxDef TargetMode);  //换到指定挡位	
void ReturnToOFFState(void); 						   //关机函数

	
#endif /* _ModeSel_ */

/********************************  End Of File  *************************************/
