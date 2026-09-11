/****************************************************************************/
/** \file LowVoltageProt.c
/** \Author redstoner_35
/** \Project Xtern Ripper Hyper Fan Ultra Edition
/** \Description 这个文件是上层应用层逻辑，负责实现系统的低电量阶梯降档和自动关机
								 逻辑，
**	History:
				2026年9月11日 Initial Release
**	
*****************************************************************************/
/****************************************************************************/
/*	include files
*****************************************************************************/
#include "BattDisplay.h"
#include "ModeSel.h"
#include "OutputChannel.h"
#include "SideKey.h"
#include "stdbool.h"

/****************************************************************************/
/*	Local pre-processor symbols/macros('#define') For Parameter definition
****************************************************************************/
#define BatteryAlertDelay 10  //电池警报延迟(1单位=0.125秒)
#define BatteryFaultDelay 2 	//电池故障强制跳档/关机的延迟(1单位=0.125秒)

/****************************************************************************/
/*	Local variable definitions('static')
****************************************************************************/	
static xdata unsigned char BattAlertTimer; //电池低电压告警处理

/****************************************************************************/
/* Local Function implementation - 'static'
****************************************************************************/	

//低电量保护函数
static void StartBattAlertTimer(void)
	{
	//启动定时器
	if(!BattAlertTimer)BattAlertTimer=1;
	}	

/****************************************************************************/
/* Global Function implementation - Timer Handler
****************************************************************************/		
	
//电池低电量报警计时器的处理函数
void BattAlertTIMHandler(void)
	{
	//电量警报
	if(BattAlertTimer&&BattAlertTimer<(BatteryAlertDelay+1))BattAlertTimer++;
	}	
	
/****************************************************************************/
/* Global Function implementation - Logic Handler
****************************************************************************/		
	
//电池低电量保护函数
void BatteryLowAlertProcess(bool IsNeedToShutOff,ModeIdxDef ModeJump)
	{
	unsigned char Thr=BatteryFaultDelay;
	bit IsChangingGear;
	//获取手电按键的状态
	if(getSideKey1HEvent())IsChangingGear=1;
	else IsChangingGear=getSideKeyHoldEvent();
	//控制计时器启停
	if(!IsBatteryFault) //电池没有发生低压故障
		{
		Thr=BatteryAlertDelay; //没有故障可以慢一点降档
		//当前在换挡阶段或者没有告警，停止计时器,否则启动
		if(!IsBatteryAlert||IsChangingGear)BattAlertTimer=0;
		else StartBattAlertTimer();
		}
  else StartBattAlertTimer();//发生低压告警立即启动定时器
	//定时器计时已满，执行对应的动作
	if(BattAlertTimer>Thr)
		{
		//当前挡位处于需要在触发低电量保护时主动关机的状态	
		if(IsNeedToShutOff)ReturnToOFFState();
		//当前处于换挡模式不允许执行降档但是需要判断电池是否过低然后强制关闭
		else if(IsChangingGear&&IsBatteryFault)ReturnToOFFState();
		//不需要关机，触发换挡动作
		else
			{
			BattAlertTimer=0;//重置定时器至初始值
			SwitchToGear(ModeJump); //复位到指定挡位
			}
		}
	}	
/*****************************  End Of File  ******************************/
