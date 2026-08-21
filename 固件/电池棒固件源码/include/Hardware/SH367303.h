/************************************************************************************/
/** \file SH367303.h
/** \Author redstoner_35
/** \Project Xtern Ripper Laser Edition 
/** \Description 这个文件为SH367303芯片的驱动函数头文件

**	History: Initial Release
**	
/************************************************************************************/
#ifndef _SH367303_
#define _SH367303_
/************************************************************************************/
/* Include files */
/************************************************************************************/
#include "stdbool.h"

/************************************************************************************/
/* Global type definiton - (typedef) */
/************************************************************************************/
typedef enum
	{
	SH_REG_Flag1=0x00,
	SH_REG_Flag2=0x01,
	SH_REG_BSTATUS=0x02,
	SH_REG_INTEN=0x03,
	SH_REG_SCONF1=0x04,
	SH_REG_SCONF2=0x05,
	SH_REG_SCONF3=0x06,
	SH_REG_SCONF4=0x07,
	SH_REG_SCONF5=0x08,
	SH_REG_SCONF6=0x09,
	SH_REG_SCONF7=0x0A,
	SH_REG_SCONF8=0x0B,
	SH_REG_SCONF9=0x0C,
	SH_REG_SCONF10=0x0D,
	SH_REG_CELL1H=0x0E,
	SH_REG_CELL1L=0x0F,
  SH_REG_CELL2H=0x10,
  SH_REG_CELL2L=0x11,
	SH_REG_CELL3H=0x12,
  SH_REG_CELL3L=0x13,
  SH_REG_CELL4H=0x14,
  SH_REG_CELL4L=0x15,
  SH_REG_CELL5H=0x16,
  SH_REG_CELL5L=0x17,
  SH_REG_TS1H=0x22,
  SH_REG_TS1L=0x23,
	SH_REG_TS2H=0x24,
	SH_REG_TS2L=0x25,
	SH_REG_TEMP1H=0x26,
	SH_REG_TEMP1L=0x27,
	SH_REG_TEMP2H=0x28,
	SH_REG_TEMP2L=0x29
	}SH367303REGDef;


/************************************************************************************/
/* Extern Functions definition - Initialization and Special Operation */
/************************************************************************************/
void SH36_HardwareInit(void);
void SH36_I2C_Recovery(void);   //进行总线死锁恢复
	
/****************************************************************************/
/*	Function implementation - Register Access
****************************************************************************/		

bit SH36_WriteReg(SH367303REGDef Reg,unsigned char Value)	; 	//写芯片的寄存器(成功返回0，否则返回1)
bit SH36_ReadReg16(SH367303REGDef Reg,unsigned short *Value); //读取芯片的16bit寄存器(成功返回0，否则返回1)
bit SH36_ReadReg8(SH367303REGDef Reg,unsigned char *Value);		//读取芯片的8bit寄存器(成功返回0，否则返回1)


#endif	/* _SH367303_ */

/*********************************  End Of File  ************************************/
