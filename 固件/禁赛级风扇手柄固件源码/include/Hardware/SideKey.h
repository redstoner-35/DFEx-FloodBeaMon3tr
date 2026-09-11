/************************************************************************************/
/** \file SideKey.h
/** \Author redstoner_35
/** \Project Xtern Ripper Hyper Fan Ultra Edition
/** \Description 这个头文件为系统按键模块硬件驱动的外部声明文件，负责声明多模态按键识别
								 模块的初始化，按键事件获取以及特殊操作的处理。

**	History: Initial Release
**	
/************************************************************************************/
#ifndef _SideKey_
#define _SideKey_

/************************************************************************************/
/*	Global type definitions('typedef')
*************************************************************************************/
typedef enum
{
HoldEvent_None=0,
HoldEvent_H=1, //长按
HoldEvent_1H=2, //单击+长按
HoldEvent_2H=3, //双击+长按
HoldEvent_3H=4, //三击+长按
HoldEvent_4H=5, //四击+长按
HoldEvent_5H=6 //5击+长按
}HoldEventDef;

//按键事件结构体定义
typedef struct
{
char LongPressDetected;
char ShortPressCount;
char ShortPressEvent;
HoldEventDef HoldStat;
}KeyEventStrDef;

/************************************************************************************/
/* Extern Functions definition - Initialization & Key Logic callback                */
/************************************************************************************/
void SideKeyInit(void);            //初始化按键处理
void SideKey_TIM_Callback(void);   //连按检测计时的回调处理
void SideKey_LogicHandler(void);   //逻辑处理

/************************************************************************************/
/* Extern Functions definition - Key event query                                    */
/************************************************************************************/

bit IsKeyEventOccurred(void); //检测是否有事件发生
bit getSideKeyLongPressEvent(void); //获取侧按按键长按2秒事件的函数
bit getSideKey1HEvent(void); //获取侧按按键是否发生单击+长按事件
bit getSideKeyHoldEvent(void);//获得侧按按钮是否有一直按住不放的事件

char getSideKeyShortPressCount(void);//获取侧按按键的单击和连击次数
char getSideKeyNClickAndHoldEvent(void); //获取侧按按下N次+长按的按键数

/************************************************************************************/
/* Extern Functions definition - Realtime key GPIO state query                      */
/************************************************************************************/
bit GetSideKeyRawGPIOState(void); 	//获取侧部按键的GPIO实时状态（没有去抖）
char GetIfSideKeyTriggerInt(void); 	//获取侧按是否触发中断

/************************************************************************************/
/* Extern Functions definition - Special Operation for Key event detector           */
/************************************************************************************/
void MarkAsKeyPressed(void); 				//标记按键按下
void ClearShortPressEvent(void); 		//清除累计的短按事件
void SideKey_SetIntOFF(void);				//关闭侧按的GPIO中断

#endif /* _SideKey_ */

/*********************************  End Of File  ************************************/
