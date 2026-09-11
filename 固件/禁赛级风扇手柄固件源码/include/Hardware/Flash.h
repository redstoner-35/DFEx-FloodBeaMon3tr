/************************************************************************************/
/** \file Flash.h
/** \Author redstoner_35
/** \Project Xtern Ripper Hyper Fan Ultra Edition
/** \Description 这个头文件为为上层系统提供了底层Flash操作的硬件接口，用于实现驱动设置的
						     配置保存功能。

**	History: Initial Release
**	
/************************************************************************************/
#ifndef _Flash_
#define _Flash_

/************************************************************************************/
/*	Global type definitions('typedef')
*************************************************************************************/
typedef enum
	{
	DataFlash_Read=0x11,
	DataFlash_Write=0x19,
	DataFlash_Erase=0x1D
	}FlashOperationDef;

/************************************************************************************/
/* Extern Functions definition                                                      */
/************************************************************************************/
void SetFlashState(bit IsUnlocked);     //设置Flash状态
void Flash_Operation(FlashOperationDef Operation,int ADDR,char *Data); //对Flash进行操作


#endif /* _Flash_ */

/********************************  End Of File  *************************************/
