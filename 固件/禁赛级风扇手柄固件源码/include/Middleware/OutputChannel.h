/************************************************************************************/
/** \file OutputChannel.h
/** \Author redstoner_35
/** \Project Xtern Ripper Hyper Fan Ultra Edition
/** \Description 这个头文件负责声明系统的风扇输出控制逻辑的接口变量以及初始化和业务逻辑。

**	History: Initial Release
**	
/************************************************************************************/
#ifndef _OutputChannel_
#define _OutputChannel_

/************************************************************************************/
/*	Global type definitions('typedef')
*************************************************************************************/
typedef struct
	{
	unsigned char SysMinSpeed;
	float SysMinVolt;
	int SysStartUpDuty;
	}MinMaxDutyVOutDef;

	
/************************************************************************************/
/* Extern Flags and Variable definition */
/************************************************************************************/
extern xdata float TargetVoltage;          //目标风扇电压(仅电压模式生效)
extern xdata float TargetfanSpeed; 				 //目标风扇速度
extern bit IsUpdateFanSpeed; 							 //请求更新风扇速度	
extern xdata MinMaxDutyVOutDef VMinMaxCfg; //存储系统风扇调速参数的最小最大数据	
extern bit IsEnablePWMFan; 								 //固件模式标志位，是否开启PWM风扇模式	
	

/************************************************************************************/
/* Extern Functions definition - Initialization                                     */
/************************************************************************************/
void OutputChannel_Init(void);	//初始化输出通道状态机	
void OutputChannel_DeInit(void); //输出通道强制复位	

/************************************************************************************/
/* Extern Functions definition - Output Channel Logic and query                     */
/************************************************************************************/	
void OutputChannel_Calc(void);  //输出通道运算处理
bit GetIfFanOutputEnabled(void); //获取风扇输出是否已经开启

	
#endif /* _OutputChannel_ */

/********************************  End Of File  *************************************/
