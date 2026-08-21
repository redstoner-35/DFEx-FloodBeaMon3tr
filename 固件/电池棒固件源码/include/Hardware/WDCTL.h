#ifndef _WDCTL_
#define _WDCTL_

//函数
void HaltWhenWDTRestart(void); //复位之后需要等300mS再初始化
void WDT_Feed(void); //喂狗
void StartWDT(void); //启动看门狗
void StopWDT(void); //停止看门狗

#endif