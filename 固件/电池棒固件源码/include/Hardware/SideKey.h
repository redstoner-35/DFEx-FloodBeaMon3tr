/************************************************************************************/
/** \file SideKey.h
/** \Author redstoner_35
/** \Project Xtern Ripper Laser Edition
/** \Description 这个头文件为系统底层设备驱动的接口定义头文件，定义了系统的多模态按键
								 操作识别模块的接口函数供中层和高层的驱动使用
								 

**	History: Initial Release
**	
/************************************************************************************/
#ifndef _SideKey_
#define _SideKey_
/************************************************************************************/
/* Global type definiton - (typedef) */
/************************************************************************************/
typedef enum
{
HoldEvent_None=0,
HoldEvent_H=1, //长按
HoldEvent_1H=2, //单击+长按
HoldEvent_2H=3, //双击+长按
HoldEvent_3H=4, //三击+长按
HoldEvent_4H=5, //四击+长按
HoldEvent_5H=6, //五击+长按
HoldEvent_6H=7 //六击+长按
}HoldEventDef;

/************************************************************************************/
/* Extern Functions definition - Initialization and Special Operation */
/************************************************************************************/
void SideKeyInit(void);							//初始化按键控制器
void SideKey_SetIntOFF(void);				//关闭侧按的GPIO中断
void MarkAsKeyPressed(void); 				//标记按键按下
void ClearShortPressEvent(void); 		//清除累计的短按事

/************************************************************************************/
/* Extern Functions definition - Callback and Logic Handler */
/************************************************************************************/
void SideKey_TIM_Callback(void);//连按检测计时的回调处理
void SideKey_LogicHandler(void);//逻辑处理

/************************************************************************************/
/* Extern Functions definition - Handler For Query Key Event */
/************************************************************************************/
bit getSideKeyHoldEvent(void);						//获得侧按按钮是否有一直按住的事件
bit IsKeyEventOccurred(void); 						//获取是否有任意的按键动作事件发生
bit getSideKey1HEvent(void); 							//获取侧按按键是否有单击+长按事件
bit getSideKeyLongPressEvent(void); 			//获取侧按按键长按2秒事件
char getSideKeyNClickAndHoldEvent(void); 	//获取侧按按下N次+长按的按键数
char getSideKeyShortPressCount(void);			//获取侧按按键的连击（包括单击）按键次数

/************************************************************************************/
/* Extern Functions definition - Handler For Query Real time key state */
/************************************************************************************/
bit GetSideKeyRawGPIOState(void); 	//获取侧部按键的GPIO实时状态（没有任何去抖）
char GetIfSideKeyTriggerInt(void); 	//获取侧按是否触发中断（曾经有按键按下事件）

#endif	/* _SideKey_ */

/*********************************  End Of File  ************************************/
