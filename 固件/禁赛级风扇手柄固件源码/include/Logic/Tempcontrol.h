/************************************************************************************/
/** \file TempControl.h
/** \Author redstoner_35
/** \Project Xtern Ripper Hyper Fan Ultra Edition
/** \Description 这个头文件为系统温度保护逻辑的处理函数声明。向其余的控制系统声明了温度保
								 护系统的处理逻辑和获取温控系统状态的API。

**	History: Initial Release
**	
/************************************************************************************/
#ifndef _TempControl_
#define _TempControl_
/************************************************************************************/
/* Include files */
/************************************************************************************/

/************************************************************************************/
/* Extern Functions definition - Initialization and logic handler                   */
/************************************************************************************/	
void TempSensorStallDetect(void);   //温度传感检测
void OverHeatProtect(void);        //过热保护处理
void TempDegDetect(void); 				 //温控逻辑计算
void ThermalSystem_Init(void);     //初始化温控系统

/************************************************************************************/
/* Extern Functions definition - Status API for Temp control query                  */
/************************************************************************************/
bit QueryIfSysThermalIsOK(void);   //获取温度系统是否允许开机
bit IsThermalStepdown(void);       //是否触发温控降档
float QueryDutyLimit(void);
float QueryVoltageLimit(void);     //获取温控系统当前的电压和占空比限制

#endif /* _TempControl_ */

/*********************************  End Of File  ************************************/
