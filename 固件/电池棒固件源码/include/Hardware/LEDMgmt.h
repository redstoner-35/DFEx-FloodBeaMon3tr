/************************************************************************************/
/** \file LEDMgmt.h
/** \Author redstoner_35
/** \Project Xtern Ripper Laser Edition
/** \Description 这个头文件为系统底层设备驱动的接口定义头文件，定义了系统的多模态侧部按键
								 LED指示灯模块的接口函数和操作流程供中层和高层的驱动使用
								 

**	History: Initial Release
**	
/************************************************************************************/
#ifndef _LEDMgmt_
#define _LEDMgmt_
/************************************************************************************/
/* Global type definiton - (typedef) */
/************************************************************************************/
typedef enum
	{
	LED_OFF=0, //关闭
	LED_Red=1, //红色常亮
	LED_Amber=2, //黄色常亮
	LED_Green=3, //绿色常亮
	LED_RedBlink=4, //红色闪烁
	LED_RedBlink_Fast=5, //红色快闪
	LED_AmberBlink=6,
	LED_AmberBlinkFast=7, //黄色快速闪烁
	LED_GreenBlink=8,     //绿色闪烁
	LED_RedBlinkThird=9, //红色快闪三次
	LED_AmberBlinkThird=10,//黄色快闪三次
	LED_GreenBlinkThird=11 //滤色快闪三次
	}LEDStateDef;
/************************************************************************************/
/* Extern Functions definition - Initialization and Special Operation */
/************************************************************************************/

void LED_Init(void);											//初始化管理器
void MakeFastStrobe(LEDStateDef LEDMode);	//制造一次快闪	
	
/************************************************************************************/
/* Extern Functions definition - Callback and Logic Handler */
/************************************************************************************/
void LEDControlHandler(void);

/************************************************************************************/
/* Extern Flags and Variable definition */
/************************************************************************************/	
extern volatile LEDStateDef LEDMode;		//外部设置侧按LED模式的index	

/************************************************************************************/
/* Global preprocessor symbol for Fast Operation - (#define) */
/************************************************************************************/	
#define IsOneTimeStrobe() (LEDMode>8)	
	
#endif	/* _LEDMgmt_ */

/*********************************  End Of File  ************************************/
