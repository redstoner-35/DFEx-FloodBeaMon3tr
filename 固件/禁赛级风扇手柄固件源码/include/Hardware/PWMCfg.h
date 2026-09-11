/************************************************************************************/
/** \file PWMCfg.h
/** \Author redstoner_35
/** \Project Xtern Ripper Hyper Fan Ultra Edition
/** \Description 这个头文件为系统PWM模块硬件驱动的外部声明文件，负责声明PWM输出模块的初
始化、特殊操作和事件处理函数。

**	History: Initial Release
**	
/************************************************************************************/
#ifndef _PWM_
#define _PWM_

/************************************************************************************/
/* Extern Flags and Variable definition */
/************************************************************************************/
extern xdata float CVDACTargetDuty;				//恒压注入DAC的目标占空比
extern xdata unsigned int FanPWMDuty; 		//风扇的PWM输出的目标PWM常数
extern bit IsNeedToUploadPWM; 						//更新PWM寄存器应用输出的使能

/************************************************************************************/
/* Extern Functions definition - Initialization & Logic callback                    */
/************************************************************************************/
void PWM_Init(void);
void PWM_DeInit(void);        //初始化和关闭PWM控制器
void PWM_OutputCtrlHandler(void);  //执行逻辑处理


/************************************************************************************/
/* Extern Functions definition - Special Operation                                  */
/************************************************************************************/	
int FanPWMStepConstant(void);  //获取风扇PWM常数用于运算

#endif /* _PWM_ */

/********************************  End Of File  *************************************/
