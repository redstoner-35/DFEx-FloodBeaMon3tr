/************************************************************************************/
/** \file BALDRV.h
/** \Author redstoner_35
/** \Project Xtern Ripper Laser Edition 
/** \Description 这个文件负责声明均衡控制模块的操控接口供其余上层应用使用

**	History: Initial Release
**	
/************************************************************************************/
#ifndef _BALDRV_
#define _BALDRV_
/************************************************************************************/
/************************************************************************************/
/* Global type definiton - (typedef) */
/************************************************************************************/
typedef enum
	{
  BalPower_Max=0,
	BalPower_High=1,
  BalPower_MHigh=2,
  BalPower_Mid=3,
	BalPower_Low=4,
	}BALPowerDef;

/************************************************************************************/
/* Extern Functions definition - Initialization and Special Operation */
/************************************************************************************/

void BAL_Init(void);  //初始化平衡驱动的PWM输出模块
void BAL_SetBalState(bit IsON,BALPowerDef TargetPower); //设置全域均衡的状态

#endif /* _BALDRV_ */

/*********************************  End Of File  ************************************/
