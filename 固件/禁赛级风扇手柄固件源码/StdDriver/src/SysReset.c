/****************************************************************************/
/** \file sysreset.c
/** \Author redstoner_35
/** \Project Xtern Ripper Hyper Fan Ultra Edition
/** \Description 这个文件负责底层执行MCU重启的设备驱动实现
**
**	History:
				2026年9月10日 Initial Release
**	
*****************************************************************************/
/****************************************************************************/
/*	include files
*****************************************************************************/
#include "cms8s6990.h"
#include "SysReset.h"

/****************************************************************************/
/*	Global function implantation(called by 'extern')
****************************************************************************/	

//生成系统复位
#pragma optimize(0)
void TriggerSoftwareReset(void)
	{
	EA=0;
	_nop_();
	TA=0xAA;
  TA=0x55;  //往TA寄存器写55AA解锁
  WDCON=0x80;  //令WDTCON[7]=1，触发系统复位
	while(1);
	}

//清除软件复位标志位
#pragma optimize(0)
void ClearSoftwareResetFlag(void)
	{
	EA=0;
	_nop_();
	TA=0xAA;
  TA=0x55;  //往TA寄存器写55AA解锁
  WDCON&=0x7F;  //令WDTCON[7]=0，触发系统复位
	}
/*************************  End Of File  ***********************/
