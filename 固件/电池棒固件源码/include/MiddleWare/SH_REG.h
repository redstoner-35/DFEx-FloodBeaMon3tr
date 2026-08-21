/************************************************************************************/
/** \file SH_REG.h
/** \Author redstoner_35
/** \Project Xtern Ripper Laser Edition 
/** \Description 这个文件为SH367303芯片的驱动函数头文件

**	History: Initial Release
**	
/************************************************************************************/
#ifndef _SH_REG_
#define _SH_REG_
/************************************************************************************/
/* Include files */
/************************************************************************************/
#include "stdbool.h"

/************************************************************************************/
/* Global type definiton - (typedef) */
/************************************************************************************/
typedef enum
	{
	SH_ConvertOK,     //本次读取成功，芯片已经顺利完成转换并更新本地电池电压结果
	SH_WaitEOC,       //本次读取失败，芯片通信成功但是未完成转换
	SH_I2CCommErr,    //本次读取失败，芯片通信异常
	}SHADCConvertResultDef;

typedef struct
	{
	float CellVoltage[4];  //四节电芯电压
	float Vmin;            //整个系统电压最低的电芯(V)
	float Vdiff;           //电芯压差(mV)
	char CellTemp;         //电芯温度
	char BMSTemp;          //BMS温度
	bool IsNTCOK;          //热敏电阻是否OK
	}BatteryStatuDef;	

typedef struct
	{
	float BatteryVoltTemp[4];  //四节电池对应的电压
	float VDiff;               //一致性缓存数据
	}ConsTempDataDef;             //一致性缓存数据		
	
/************************************************************************************/
/* Extern Flags and Variable definition */
/************************************************************************************/		
extern xdata BatteryStatuDef	BattState;  //电池状态存储	
extern xdata ConsTempDataDef ConsBuf;     //一致性电压缓存	
	
/************************************************************************************/
/*	External Function prototypes definition
/************************************************************************************/
SHADCConvertResultDef SH36_ConvertSysState(void); //执行转换更新	
void SH36_RegInit(void); //进行SH36的寄存器初始化和转换
void SH36_SendSleepCommand(void);	//强制系统进入休眠模式
void SH36_I2C_Recovery(void);     //SH36总线死锁恢复
void SH36_TransferBattState(void); //转移采集的电池和温度数据到缓存区
	
#endif	/* _SH_REG_ */

/*********************************  End Of File  ************************************/
