#include "WDCTL.h"
#include "cms8s6990.h"
#include <intrins.h>

//喂狗
#pragma OPTIMIZE(0)
void WDT_Feed(void)
	{
	EA=0;
	_nop_();
	TA=0xAA;
	TA=0x55;
	WDCON|=0x01;
	_nop_();
	_nop_();
	EA=1;
	}

//关闭看门狗
#pragma OPTIMIZE(0)	
void StopWDT(void)
	{
	//停止看门狗
	EA=0;
	_nop_();
	TA=0xAA;
	TA=0x55;
	WDCON=0x00;
	EA=1;
	}	
	
//启动看门狗
#pragma OPTIMIZE(0)
void StartWDT(void)
	{
	//设置看门狗时间为48MHz/2^26=1.39S
	CKCON&=0x1F;
  CKCON|=0xE0;		
	//启动看门狗
	_nop_();
	TA=0xAA;
	TA=0x55;
	WDCON=0x02;
	}
