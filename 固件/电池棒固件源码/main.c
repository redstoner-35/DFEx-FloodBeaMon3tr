/****************************************************************************/
/** \file main.c
/** \Author redstoner_35
/** \Project Xtern Ripper Laser Edition 
/** \Description 这个文件负责系统的主函数处理

**	History: Initial Release
**	
*****************************************************************************/
/****************************************************************************/
/*	include files
*****************************************************************************/
#include "cms8s6990.h"
#include "GPIO.h"
#include "delay.h"
#include "SideKey.h"
#include "LEDMgmt.h"
#include "SH367303.h"
#include "SH_REG.h"
#include "PinDefs.h"
#include "ChgMgmt.h"
#include "SysReset.h"
#include "BALDRV.h"
#include "WDCTL.h"

/****************************************************************************/
/*	Local variable  definitions('static')
****************************************************************************/
bit TaskSel=0;  //选择处理的系统任务

/****************************************************************************/
/*	External Function prototypes definition
****************************************************************************/


/****************************************************************************/
/*	Main Function
****************************************************************************/
void main(void)
	{
	//时钟和RSTCU初始化
  StartWDT();               //强制开启看门狗		
	ClearSoftwareResetFlag(); //清除系统软件复位Flag
  StartSystemTimeBase(); //启动系统定时器提供系统定时和延时函数
	//初始化外设
	LED_Init(); //初始化侧按LED
	SH36_HardwareInit(); //初始化SH367303的硬件总线
  SideKeyInit(); //侧按初始化
  SH36_RegInit(); //下发SH367303的初始化寄存器配置，并进行初次转换
	BAL_Init();     //启动均衡管理系统	
	ChargeMgmt_Init(); //初始化充放管理和业务逻辑
		
	//系统启动成功，禁用所有空着的IO
	MaskUnusedIO();	
	//主循环	
  while(1)
		{
	  //实时处理
		SideKey_LogicHandler(); //处理侧按事务
		ChargeMgmtKeyLogic();   //处理均衡部分的按键操作
		ACTCommandLogicHandler();  //处理IP2366激活控制的逻辑
    ChargeMgmt_MainLEDHandler();  //处理LED控制
	  ChargeMgmt_BalHandler();    //关闭平衡处理

		//8Hz交错定时处理
		if(!SysHFBitFlag)continue;  //时间没到，跳过处理
		WDT_Feed();           			//进行喂狗	

		//Task0，处理计算量比较大的任务
    if(!TaskSel)
			{
			LEDControlHandler();//侧按指示LED控制函数	
			ChargeMgmt_TIMHandler();  //充电逻辑的计时处理
			//处理结束，对任务选择进行翻转处理下一组
			TaskSel=1;
		  }			
		//Task1，处理计算量比较小的计时任务
		else
			{	
			
			INFOTimerHandler();   //定时器提示用户的处理
			SideKey_TIM_Callback();//侧按按键的监测定时器处理			
			//处理结束，对任务选择进行翻转处理下一组
			TaskSel=0;
			}
		//处理完毕，令flag清零
		SysHFBitFlag=0;	
		}
	}
