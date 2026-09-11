/****************************************************************************/
/** \file UnusedIO.c
/** \Author redstoner_35
/** \Project Xtern Ripper Hyper Fan Ultra Edition
/** \Description 这个文件负责操作GPIO寄存器，将系统内所有未使用的IO全部mask为低
								 电平对外输出模式，避免IO浮空引起的待机电流异常。
**
**	History:
				2026年9月11日 Initial Release
**	
*****************************************************************************/
/****************************************************************************/
/*	include files
*****************************************************************************/
#include "GPIO.h"
#include "PinDefs.h"
#include "cms8s6990.h"

/****************************************************************************/
/* Global Function implementation - (in header files with 'extern')
****************************************************************************/	
void MaskUnusedIO(void)
	{
	//Mask掉无用的IO强制输出0	
	P0TRIS|=P0UnusedPIN;
	P1TRIS|=P1UnusedPIN;
	P2TRIS|=P2UnusedPIN;
	P3TRIS|=P3UnusedPIN;
	//所有无用的IO强制输出0
	P0&=~P0UnusedPIN;
	P1&=~P1UnusedPIN;
	P2&=~P2UnusedPIN;
	P3&=~P3UnusedPIN;
	//所有对外IO关闭上下拉电阻
	P0UP&=~P0UnusedPIN;
	P0RD&=~P0UnusedPIN;
	
	P1UP&=~P1UnusedPIN;
	P1RD&=~P1UnusedPIN;
		
	P2UP&=~P2UnusedPIN;
	P2RD&=~P2UnusedPIN;
		
	P3UP&=~P3UnusedPIN;
	P3RD&=~P3UnusedPIN;
	}
/*****************************  End Of File  ******************************/
