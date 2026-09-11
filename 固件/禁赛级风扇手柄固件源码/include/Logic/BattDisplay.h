/************************************************************************************/
/** \file BattDisplay.h
/** \Author redstoner_35
/** \Project Xtern Ripper Hyper Fan Ultra Edition
/** \Description 这个头文件负责声明系统的电量报告逻辑的相关处理函数供其他业务逻辑调用。

**	History: Initial Release
**	
/************************************************************************************/
#ifndef _BattDisplay_
#define _BattDisplay_

/************************************************************************************/
/*	Global type definitions('typedef')
*************************************************************************************/

typedef enum
	{
	Battery_Plenty, //电池电量充足
	Battery_Mid, //电池电量较为充足
	Battery_Low, //电池电量不足
	Battery_VeryLow //电池电量严重不足
	}BattStatusDef;

/************************************************************************************/
/* Extern Flags and Variable definition */
/************************************************************************************/
extern bit IsBatteryAlert; 									//电池低电警告发生
extern bit IsBatteryFault; 									//电池低电量故障发生
extern xdata int CellVoltage; 							//滤波之后的电池电压

/************************************************************************************/
/* Extern Functions definition - Initialization And Cell config definition          */
/************************************************************************************/
void BattCellCountConfig(void); //进行电池节数识别和处理
void WaitBatteryVoltageOK(void); //等待电池电压就绪（安全保护）
void DisplayVBattAtStart(bit IsPOR); //在启动时显示电池电压


/************************************************************************************/
/* Extern Functions definition - Telemetry Logic handler                            */
/************************************************************************************/	
void BatteryTelemHandler(void);  //电池测量和指示灯控制	
void BattDisplayTIM(void); //电池电量显示函数处理


/************************************************************************************/
/* Extern Functions definition - API For Voltage/Temperature report system          */
/************************************************************************************/

bit IsVshowFSMInAction(void);				//查询函数，电压提示状态机是否在操作
void TriggerBattStatDisplay(void); 	//启动电池电量状态显示
void TriggerTShowDisplay(void); 		//启动温度显示
void TriggerVshowDisplay(void); 		//启动电池电压显示

	
#endif /* _BattDisplay_ */

/********************************  End Of File  *************************************/
