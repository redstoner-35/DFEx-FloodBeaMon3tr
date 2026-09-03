/************************************************************************************/
/** \file ADCCfg.h
/** \Author redstoner_35
/** \Project Xtern Ripper Hyper Fan Ultra Edition
/** \Description 个头文件为系统ADC模块硬件驱动的外部声明文件，负责声明ADC转换引擎的初
始化、特殊操作和事件处理函数，并声明ADC结果输出的外部变量。

**	History: Initial Release
**	
/************************************************************************************/
#ifndef _ADCEngine_
#define _ADCEngine_
/************************************************************************************/
/* Include files */
/************************************************************************************/
#include "stdbool.h"

/*************************************************************************************/
/*	Global type definitions('typedef')
**************************************************************************************/
typedef struct
	{
  int Systemp; //系统温度
	float BatteryVoltage; //等效单节电池电压(V)
	float RawBattVolt; //原始的电池电压(V)
	float MCUVDD; //单片机的VDD
	bool IsNTCOK; //NTC是否OK
	}ADCResultStrDef;

/************************************************************************************/
/* Extern Flags and Variable definition */
/************************************************************************************/
extern ADCResultStrDef Data;		//ADC结果输出
extern bit IsNotAllowAsync; 		//是否启用异步转换，1=是
extern bit IsADResultOK;        //异步模式下指示转换结束
	
/************************************************************************************/
/* Extern Functions definition */
/************************************************************************************/	
void ADC_Init(void);
void ADC_DeInit(void);
void SystemTelemHandler(void);

/************************************************************************************/
/* Extern Fast Operation Macro definition */
/************************************************************************************/
#define EnableADCAsync() IsNotAllowAsync=0
#define DisableADCAsync() IsNotAllowAsync=1  //配置ADC使能和除能异步操作的宏

#endif /* _ADCEngine_ */

/*********************************  End Of File  ************************************/
