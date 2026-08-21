/****************************************************************************/
/** \file LEDMgmt.c
/** \Author redstoner_35
/** \Project Xtern Ripper Laser Edition 
/** \Description 这个文件负责实现侧按电子开关的红绿双色电量指示灯的初始化、驱动和
多种闪烁模式的实现

**	History: Initial Release
**	
*****************************************************************************/
/****************************************************************************/
/*	include files
*****************************************************************************/
#include "delay.h"
#include "LEDMgmt.h"
#include "GPIO.h"
#include "PinDefs.h"
#include "cms8s6990.h"

/****************************************************************************/
/*	Global variable definitions(declared in header file with 'extern')
****************************************************************************/
volatile LEDStateDef LEDMode; 

/****************************************************************************/
/*	Local type definitions('typedef')
****************************************************************************/

/****************************************************************************/
/*	Local variable and SFR definitions('static and sfr')
****************************************************************************/
static xdata char timer;

sbit RLED=RedLEDIOP^RedLEDIOx;
sbit GLED=GreenLEDIOP^GreenLEDIOx; 

/****************************************************************************/
/*	Function implementation - global ('extern') and local('static')
****************************************************************************/
//LED配置函数
void LED_Init(void)
	{
	GPIOCfgDef LEDInitCfg;
	//设置结构体
	LEDInitCfg.Mode=GPIO_Out_PP;
  LEDInitCfg.Slew=GPIO_Slow_Slew;		
	LEDInitCfg.DRVCurrent=GPIO_High_Current; //配置为低斜率大电流的推挽输出
	//初始化寄存器
	RLED=0;
	GLED=0;
	//配置GPIO
	GPIO_SetMUXMode(RedLEDIOG,RedLEDIOx,GPIO_AF_GPIO);
	GPIO_SetMUXMode(GreenLEDIOG,GreenLEDIOx,GPIO_AF_GPIO);
	GPIO_ConfigGPIOMode(RedLEDIOG,GPIOMask(RedLEDIOx),&LEDInitCfg); //红色LED(推挽输出)
	GPIO_ConfigGPIOMode(GreenLEDIOG,GPIOMask(GreenLEDIOx),&LEDInitCfg); //绿色LED(推挽输出)
	//初始化模式设置和变量
	timer=0;
	LEDMode=LED_OFF;
	}

//LED控制函数	
void LEDControlHandler(void)
	{
	char buf;	
	//据目标模式设置LED状态
	switch(LEDMode)
		{
		case LED_OFF:RLED=0;GLED=0;timer=0;break; //LED关闭
		case LED_Green:RLED=0;GLED=1;break;//绿色LED
		case LED_Red:RLED=1;GLED=0;break;//红色LED
		case LED_Amber:RLED=1;GLED=1;break;//黄色LED
	
		case LED_GreenBlink: //绿色慢闪
      RLED=0;			
			buf=timer&0x7F; //读取当前定时器的控制位
			if(buf<3)
				{
				buf++;
			  timer&=0x80;
				timer|=buf; //时间没到，继续计时
				}
			else timer=timer&0x80?0x00:0x80; //翻转bit 7并重置定时器
			GLED=timer&0x80?1:0; //根据bit 7载入LED控制位		
		  break;

		case LED_AmberBlink: //黄色慢闪	
		case LED_AmberBlinkFast:
		  buf=timer&0x7F; //读取当前定时器的控制位
			if(buf<(LEDMode==LED_AmberBlink?3:0))
				{
				buf++;
			  timer&=0x80;
				timer|=buf; //时间没到，继续计时
				}
			else timer=timer&0x80?0x00:0x80; //翻转bit 7并重置定时器
			if(timer&0x80){RLED=1;GLED=1;}
			else {RLED=0;GLED=0;}				//根据bit 7载入LED控制位
			break;

		case LED_RedBlink_Fast: //红色快闪	
		case LED_RedBlink: //红色闪烁
			GLED=0;
		  buf=timer&0x7F; //读取当前定时器的控制位
			if(buf<(LEDMode==LED_RedBlink?3:0))
				{
				buf++;
			  timer&=0x80;
				timer|=buf; //时间没到，继续计时
				}
			else timer=timer&0x80?0x00:0x80; //翻转bit 7并重置定时器
			RLED=timer&0x80?1:0; //根据bit 7载入LED控制位
			break;
		case LED_GreenBlinkThird: //绿色闪三次
		case LED_AmberBlinkThird:
		case LED_RedBlinkThird: //LED红色或者黄色闪烁3次
			timer&=0x7F; //去掉最上面的位
			if(timer>6)LEDMode=LED_OFF; //时间到，关闭识别
			else if((timer++)%2)//继续计时,符合条件则点亮LED
				{
				//根据LED颜色输出对应的指令，点亮对应的LED
				if(LEDMode==LED_GreenBlinkThird){GLED=1;RLED=0;}
				else if(LEDMode==LED_AmberBlinkThird){GLED=1;RLED=1;}
				else {GLED=0;RLED=1;}
				}		
			else 
				{
				//不符合条件LED熄灭
				RLED=0;
				GLED=0;
				}
		  break;
		}
	}

//制造一次快闪
void MakeFastStrobe(LEDStateDef LEDMode)
	{
	//打开LED
	switch(LEDMode)
		{
		case LED_Green:RLED=0;GLED=1;break;//绿色LED
		case LED_Red:RLED=1;GLED=0;break;//红色LED
		case LED_Amber:RLED=1;GLED=1;break;//黄色LED
		default:return; //非法值
		}
	delay_ms(10);
	//关闭LED
	RLED=0;
	GLED=0;
	}	
/*********************************  End Of File  ************************************/
