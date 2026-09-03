#include "LEDMgmt.h"
#include "FastOp.h"
#include "ModeSel.h"
#include "ADCCfg.h"
#include "OutputChannel.h"

//温控参数
#define ThrottleHysteresis 2   //降档开启提示的指示阈值（施密特触发）
#define NoThrottleTemp 50
#define MaxThrottleTemp 65     //不降档和最大降档的温度
#define NoThrottleVlim 12.0
#define MaxThrottleVlim 8.0    //不降档和最大降档的电压限制
#define NoThrottleDutylim 100
#define MaxThrottleDutylim 50    //不降档和最大降档的占空比限制

//内部变量
static xdata unsigned char TempSensorStallTIM=0;        //温度传感器故障计时
static bit SysOverheatFlag=0;                           //系统过热flag
static bit IsThermalThrottle=0;                         //是否触发温控保护

//外部变量占空比和电压限制
xdata float DutyLimit=NoThrottleDutylim;                           //占空比限制
xdata float VoltageLimit=NoThrottleVlim;                        //电压限制

//获取温度系统是否允许开机
bit QueryIfSysThermalIsOK(void)
	{
	if(TempSensorStallTIM>9)return 0; //温度传感故障，不允许开机
	if(SysOverheatFlag)return 0;
	//其余情况允许开机
	return 1;
	}

//是否触发温控降档
bit IsThermalStepdown(void)
	{
	//系统没开机返回0
	if(CurrentMode->ModeIdx==Mode_OFF)return 0;
	//NTC异常返回0
	if(!Data.IsNTCOK)return 0;
	return IsThermalThrottle;  //温度超过降档阈值，警报
	}	
	
//过热保护处理
void OverHeatProtect(void)
	{
	//NTC故障，不执行
	if(!Data.IsNTCOK)return;
	//施密特滞回控制
	if(Data.Systemp>68)SysOverheatFlag=1;  //系统超过68摄氏度，flag置起
	else if(SysOverheatFlag&&Data.Systemp<50)SysOverheatFlag=0;   //Flag被置起，然后外壳温度低于50，系统过热Flag清除
	//如果系统处于开机状态且发生过热，强制关闭
	if(SysOverheatFlag&&CurrentMode->ModeIdx!=Mode_OFF)
		{
		ReturnToOFFState();
		LEDMode=LED_AmberBlinkFifth;  //系统过热，黄色闪五次并强制关机
		}
	}	
	
//温度传感检测
void TempSensorStallDetect(void)
	{
	if(!Data.IsNTCOK)
		{
		//传感器故障超过1秒
		if(TempSensorStallTIM==10)
			{
		  //系统处于开机状态，强制关闭
			if(CurrentMode->ModeIdx!=Mode_OFF)
				{
				ReturnToOFFState();
				LEDMode=LED_RedBlinkFifth;  //温度传感器故障超过1秒，红色闪五次并强制关机
				}
			//确保只执行一次
			TempSensorStallTIM=11;
			}
		//传感器故障发生但是还没到时间，累计计数
		else if(TempSensorStallTIM<10)TempSensorStallTIM++;
		}
	//传感器正常，累计计数
	else TempSensorStallTIM=0;
	}
	
//线性降额计算
void TempDegDetect(void)
	{
	float NewVLIM,NewDLim;
	//NTC故障或者转换未完成，不执行
	if(!Data.IsNTCOK||!IsADResultOK)return;
	IsADResultOK=0;   								//本次结果使用完毕，复位等待下次AD采样结束
	//执行降档指示判断
	if(Data.Systemp>NoThrottleTemp)IsThermalThrottle=1;
	else if(IsThermalThrottle&&Data.Systemp<(NoThrottleTemp-ThrottleHysteresis))IsThermalThrottle=0;	
		
	//没有降档或者系统处于关闭状态，不限制
	if(CurrentMode->ModeIdx==Mode_OFF||Data.Systemp<=NoThrottleTemp)
		{
		NewVLIM=NoThrottleVlim;
		NewDLim=NoThrottleDutylim;
		}
	//降到最大了，范围最高限制值
	else if(Data.Systemp>=MaxThrottleTemp)
		{
		NewVLIM=MaxThrottleVlim;
		NewDLim=MaxThrottleDutylim; 
		}
	//范围内，进行线性温度计算
	else
		{
		NewVLIM=(float)(MaxThrottleTemp-Data.Systemp)*((float)(NoThrottleVlim-MaxThrottleVlim)/(float)(NoThrottleTemp-MaxThrottleTemp));
		NewVLIM+=(float)MaxThrottleVlim;                                                                                      //使用线性法计算电压限制
		NewDLim=(float)(MaxThrottleTemp-Data.Systemp)*((float)(NoThrottleDutylim-MaxThrottleDutylim)/(float)(NoThrottleTemp-MaxThrottleTemp));
		NewDLim+=MaxThrottleDutylim;          //使用线性法计算占空比限制
		}
	//检查新的数值是否发生变更，如果变更，则应用新的参数
	if(NewDLim==DutyLimit&&NewVLIM==VoltageLimit)return;
	DutyLimit=NewDLim;
	VoltageLimit=NewVLIM;
	IsUpdateFanSpeed=1; 		//温控结果更新，需要应用新的风扇参数更新转速
	}
	