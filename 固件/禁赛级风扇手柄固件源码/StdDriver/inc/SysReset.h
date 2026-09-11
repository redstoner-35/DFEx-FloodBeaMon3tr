/****************************************************************************/
/** \file SysReset.h
/** \Author redstoner_35
/** \Project Xtern Ripper Hyper Fan Ultra Edition
/** \Description 这个文件提供了系统复位所需的底层硬件驱动的API函数声明
**
**	History:
				2026年9月11日 Initial Release
**	
*****************************************************************************/
#ifndef _SYSRESET_
#define _SYSRESET_

/****************************************************************************/
/* Extern Functions definition */
/****************************************************************************/	
void TriggerSoftwareReset(void); //生成系统复位
void ClearSoftwareResetFlag(void); //清除软件复位标志位


#endif /* _SYSRESET_ */

/*********************************  End Of File  ****************************/
