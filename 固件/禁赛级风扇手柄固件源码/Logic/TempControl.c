#include "LEDMgmt.h"
#include "FastOp.h"
#include "ModeSel.h"
#include "ADCCfg.h"
#include "OutputChannel.h"
#include "BattDisplay.h"

//温控温度参数
#define NoThrottleTemp 47  //温控停止温度
#define ThrottleMaintainTemp 49 //温控维持温度
#define PowerOffTemp 68         //过热关机温度

//积分温控系数
#define ThermalDutyStepK 0.01  //占空比K
#define ThermalVoltStepK 0.005  //电压K

//温控降档范围限制
#define NoThrottleVlim 12.0
#define MaxThrottleVlim 8.0    //不降档和最大降档的电压限制
#define NoThrottleDutylim 100
#define MaxThrottleDutylim 40    //不降档和最大降档的占空比限制

//内部变量
static xdata unsigned char TempSensorStallTIM;        //温度传感器故障计时
static bit SysOverheatFlag;                           //系统过热flag
static bit IsThermalThrottle;                         //是否触发温控保护

//外部变量占空比和电压限制
xdata float DutyLimit;                           //占空比限制
xdata float VoltageLimit;                        //电压限制

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
	//系统没开机返回0，如果当前处于电量查询阶段，也返回0避免干扰电量查询
	if(VshowFSMState!=BattVdis_Waiting)return 0;
	if(CurrentMode->ModeIdx==Mode_OFF)return 0;
	//NTC异常返回0
	if(!Data.IsNTCOK)return 0;
	return IsThermalThrottle;  //温度超过降档阈值，警报
	}	
	
//加载温控系统初始参数
void ThermalSystem_Init(void)
	{
	//加载内部计时变量和flag
	TempSensorStallTIM=0;
	SysOverheatFlag=0;
	IsThermalThrottle=0;
	//加载电压和占空比限制
	DutyLimit=NoThrottleDutylim;
	VoltageLimit=NoThrottleVlim;
	}	
	
//过热保护处理
void OverHeatProtect(void)
	{
	//NTC故障，不执行
	if(!Data.IsNTCOK)return;
	//施密特滞回控制
	if(Data.Systemp>PowerOffTemp)SysOverheatFlag=1;  //系统超过关机温度，flag置起
	else if(SysOverheatFlag&&Data.Systemp<ThrottleMaintainTemp)SysOverheatFlag=0;   //Flag被置起，然后外壳温度低于温控维持温度，系统过热Flag清除
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
	//传感器正常，清除计数器
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
	if(Data.Systemp>ThrottleMaintainTemp)IsThermalThrottle=1;
	else if(IsThermalThrottle&&Data.Systemp<NoThrottleTemp)IsThermalThrottle=0;	
		
	//没有降档或者系统处于关闭状态，不限制
	if(CurrentMode->ModeIdx==Mode_OFF||!IsThermalThrottle)
		{
		NewVLIM=NoThrottleVlim;
		NewDLim=NoThrottleDutylim;
		}
	//触发降档，当前是PWM模式，执行PWM积分温控缓慢降低出力
	else if(IsEnablePWMFan)
		{
		//占空比部分的积分温控
		NewDLim=(float)(Data.Systemp-ThrottleMaintainTemp); //计算误差值
		if(Data.Systemp>(PowerOffTemp-5)&&NewDLim>0)NewDLim*=2.5; //系统温度接近过热关机点，迅速提升比例系数快速下调功率
		if(NewDLim<(-2))NewDLim=(-2);                        //风扇把手是散热慢，加热快，所以限制回升速度最大2
		NewDLim=DutyLimit-(NewDLim*(float)ThermalDutyStepK);  //计算积分温控参数
		//占空比限幅
		if(NewDLim>NoThrottleDutylim)NewDLim=NoThrottleDutylim;
		if(NewDLim<MaxThrottleDutylim)NewDLim=MaxThrottleDutylim;  //限制占空比范围
		//电压直接载入不做变更
		NewVLIM=VoltageLimit;	
		}
	//当前是电压模式，执行电压温控
	else
		{
		//电压部分的积分温控	
		NewVLIM=(float)(Data.Systemp-ThrottleMaintainTemp); //计算误差值	
		if(Data.Systemp>(PowerOffTemp-5)&&NewVLIM>0)NewVLIM*=2.5; //系统温度接近过热关机点，迅速提升比例系数快速下调功率
		if(NewVLIM<(-2))NewVLIM=(-2);                        //风扇把手是散热慢，加热快，所以限制回升速度最大2
		NewVLIM=VoltageLimit-(NewVLIM*(float)ThermalVoltStepK); //计算电压的积分参数
		//电压限幅
		if(NewVLIM>NoThrottleVlim)NewVLIM=NoThrottleVlim;
		if(NewVLIM<MaxThrottleVlim)NewVLIM=MaxThrottleVlim;  //限制电压范围
    //PWM占空比直接载入不做变更
		NewDLim=DutyLimit;	
		}
		
	//检查新的数值是否发生变更，如果变更，则应用新的参数
	if(NewDLim==DutyLimit&&NewVLIM==VoltageLimit)return;
	DutyLimit=NewDLim;
	VoltageLimit=NewVLIM;
	IsUpdateFanSpeed=1; 		//温控结果更新，需要应用新的风扇参数更新转速
	}
	