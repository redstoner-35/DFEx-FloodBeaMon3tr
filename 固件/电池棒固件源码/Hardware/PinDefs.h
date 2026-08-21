/************************************************************************************/
/** \file PinDefs.h
/** \Author redstoner_35
/** \Project Xtern Ripper Laser Edition 
/** \Description 这个文件为底层的硬件定义文件，定义了整个工程内所有外设对应的硬件PIN和模
拟输入ADC通道的映射关系。

**	History: Initial Release
**	
/************************************************************************************/
#ifndef PINDEFS
#define PINDEFS
/************************************************************************************/
/* Include files */
/************************************************************************************/
#include "GPIOCfg.h"

/************************************************************************************
以下是Mask掉不用的GPIO的指令(外设里面没有用到P2.3、P2.4和P0.3，把这些不用的IO全部
设置为输出0，推挽状态)
************************************************************************************/
#define MaskUnusedIO() do{P2&=0xE7;P2TRIS|=0x18;P0&=0xF7;P0TRIS|=0x08;}while(0)

/************************************************************************************
以下是系统的常规GPIO以及部分特殊的数字功能引脚，用于控制外部外设切换功能
************************************************************************************/
#define VINOKIOP GPIO_PORT_0
#define VINOKIOG 0
#define VINOKIOx GPIO_PIN_1		//Type-C接口输入OK的信号引脚(P0.1)

#define BATACTIOP GPIO_PORT_2
#define BATACTIOG 2
#define BATACTIOx GPIO_PIN_2		//电池激活和查看电量的ACT引脚(控制IP2366使能，P2.2)

#define CHGOFFIOP GPIO_PORT_0
#define CHGOFFIOG 0
#define CHGOFFIOx GPIO_PIN_0		//禁止IP2366进行充电的引脚(P0.0)

#define BALENIOP GPIO_PORT_3
#define BALENIOG 3
#define BALENIOx GPIO_PIN_2		//均衡的栅极驱动LDO使能引脚(P3.2)

#define BMSSCLGPIOP GPIO_PORT_2
#define BMSSCLGPIOG 2
#define BMSSCLGPIOx GPIO_PIN_5		//BMS芯片的SCL引脚(P2.5)

#define BMSSDAGPIOP GPIO_PORT_2
#define BMSSDAGPIOG 2
#define BMSSDAGPIOx GPIO_PIN_6		//BMS芯片的SDA引脚(P2.6)

/************************************************************************************
以下是系统的PWM输出引脚，用于对外输出PWM控制均衡模块
************************************************************************************/

#define PWMPOSIOP GPIO_PORT_3
#define PWMPOSIOG 3
#define PWMPOSIOx GPIO_PIN_1		//PWM正输出引脚(P3.1)

#define PWMNEGIOP GPIO_PORT_3
#define PWMNEGIOG 3
#define PWMNEGIOx GPIO_PIN_0		//PWM负输出引脚(P3.0)

/************************************************************************************
以下是系统的按键模块的GPIO引脚，负责驱动负责按键小板部分(包括指示灯和按键本身) 
************************************************************************************/
#define SideKeyGPIOP GPIO_PORT_0
#define SideKeyGPIOG 0
#define SideKeyGPIOx GPIO_PIN_5 	//侧按按键(P0.5)


#define RedLEDIOP GPIO_PORT_0
#define RedLEDIOG 0
#define RedLEDIOx GPIO_PIN_2		//红色指示灯(P0.2)	


#define GreenLEDIOP GPIO_PORT_0
#define GreenLEDIOG 0
#define GreenLEDIOx GPIO_PIN_4		//绿色指示灯(P0.4)

#endif /* PINDEFS */

/*********************************  End Of File  ************************************/
