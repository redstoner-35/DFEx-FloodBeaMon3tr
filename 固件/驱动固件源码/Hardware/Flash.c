/****************************************************************************/
/** \file Flash.c
/** \Author redstoner_35
/** \Project Xtern Ripper Hyper Boost HV 4S-GaN Edition
/** \Description 这个文件负责实现对于MCU内置的Data Flash的驱动并完成读取、写入
擦除等操作。为中层的配置NVRAM存储模块提供底层驱动。

**	History: Initial Release
**	
*****************************************************************************/
/****************************************************************************/
/*	include files
*****************************************************************************/
#include "cms8s6990.h"
#include "Flash.h"

/****************************************************************************/
/*	Local pre-processor symbols/macros('#define')
****************************************************************************/
#define Flash_Busy_Msk 0x01  //Flash正忙标志位
#define Flash_Unlock_Cmd 0xAA //解锁Flash的命令（写入到MLOCK寄存器）
#define Flash_Lock_Cmd 0x55   //锁定Flash的命令（写入到MLOCK寄存器）

/****************************************************************************/
/*	Function implementation - global ('extern')
****************************************************************************/

//解锁Flash
void UnlockFlash(void)
	{
	EA=0;
	_nop_();
	MLOCK = Flash_Unlock_Cmd;	
	}

//重新把Flash锁上(该函数必须成对使用，和解锁函数配对)
void LockFlash(void)
	{
	MLOCK = Flash_Lock_Cmd;		
	_nop_();
	EA=1; //重新启用中断
	}

/*******************************************
对系统的Data Flash进行操作（读取和写入数据）
需要注意的是，这个函数必须先调用UnlockFlash()
函数解锁Flash后才能进行操作，否则会触发MCU的
HardFault！！
*******************************************/
void Flash_Operation(FlashOperationDef Operation,int ADDR,char *Data)
	{
	if(Operation==DataFlash_Write)MDATA=*Data; //写入模式下需要写数据	
	MADRL = ADDR&0xFF;
	MADRH = (ADDR>>8)&0xFF; //设置地址
	_nop_();	
	MCTRL = (unsigned char)Operation; //对数据区进行读取操作
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	while(MCTRL & Flash_Busy_Msk); //等待读取结束
	if(Operation==DataFlash_Read)*Data=MDATA; //返回数据
	}
/*************************  End Of File  ***********************/
