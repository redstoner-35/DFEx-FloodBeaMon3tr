/************************************************************************************/
/** \file LEDMgmt.h
/** \Author redstoner_35
/** \Project Xtern Ripper Hyper Fan Ultra Edition
/** \Description 这个头文件负责声明系统多模态侧按LED控制器的相关函数和外部控制flag供
						     上层业务逻辑调用。

**	History: Initial Release
**	
/************************************************************************************/
#ifndef _LEDMgmt_
#define _LEDMgmt_

/************************************************************************************/
/*	Global type definitions('typedef')
*************************************************************************************/
typedef enum
	{
	LED_OFF=0, //关闭
	LED_Green=1, //绿色常亮
	LED_Red=2, //红色常亮
	LED_RedBlink=3, //红色闪烁
	LED_Amber=4, //黄色常亮
	LED_RedBlink_Fast=5, //红色快闪
	LED_AmberBlinkFast=6, //黄色快速闪烁
	LED_RedBlinkFifth=7, //红色快闪五次
	LED_RedBlinkThird=8, //红色快闪三次
	LED_GreenBlinkThird=9,//绿色快闪三次
	LED_AmberBlinkFifth=10 //黄色快闪五次
	}LEDStateDef;

/************************************************************************************/
/* Extern Flags and Variable definition */
/************************************************************************************/
extern volatile LEDStateDef LEDMode;   //外部设置LED模式的index

/************************************************************************************/
/* Extern Functions definition - Initialization & Logic callback                    */
/************************************************************************************/
void LED_DeInit(void);
void LED_Init(void);					 //初始化和关闭LED控制器
void LEDControlHandler(void);	 //LED控制器逻辑
	
	
/************************************************************************************/
/* Extern Functions definition - Special Operation                                  */
/************************************************************************************/	
void MakeFastStrobe(LEDStateDef LEDMode);	//制造一次快闪
	
/************************************************************************************/
/* Extern Fast Operation Macro definition */
/************************************************************************************/	

//判断LED控制器当前是否处于一次性执行提示，以避免打断一次性闪烁执行
#define IsOneTimeStrobe() (LEDMode>6)		 


#endif /* _LEDMgmt_ */

/********************************  End Of File  *************************************/
