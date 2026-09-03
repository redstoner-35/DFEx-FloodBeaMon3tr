/****************************************************************************
/** \file NTC.h
/** \Author [NTC resistor LUT generator BOT] @ redstoner_35
/** \Project Xtern Ripper Hyper Boost HV 4S-GaN Edition 
/** \Description 这个头文件负责声明根据NTC读回的阻值反向计算温度的函数以便于ADC
处理函数进行调用（该文件由机器自动生成，未经允许不得随意修改！！）

/** \AdditionINFO  
		This is an automatically generated file by NTC resistor LUT 
		generator. DO NOT EDIT UNLESS YOU FULLY UNDERSTAND WHAT THIS
		FILE ACTUALLY DOES!
		NTC PARAMETER:100.00KΩ @ 25℃ B4310
		Table temperature range:-20℃ to 85℃
		Total ROM space for table:378 Bytes
		Target MCU Architecture:8051 Based MCU

**	History: 
				2026年7月8日 11:46 Initial Release
				
**	
*****************************************************************************/
#ifndef _NTC_
#define _NTC_
/****************************************************************************/
/*	include files
*****************************************************************************/
#include <stdbool.h>

/****************************************************************************/
/*	Global Function prototype definition
*****************************************************************************/
int CalcNTCTemp(bool *IsNTCOK,unsigned long NTCRes);


#endif /* _NTC_ */

/******************************  End Of File  *******************************/
