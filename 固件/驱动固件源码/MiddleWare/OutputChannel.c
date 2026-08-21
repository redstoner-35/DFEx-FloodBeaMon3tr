/****************************************************************************/
/** \file OutputChannel.c
/** \Author redstoner_35
/** \Project Xtern Ripper Hyper Boost HV 4S-GaN Edition 
/** \Description 这个文件为中层设备驱动文件，负责根据上层逻辑层反馈的目标输出电流
值计算并操控PWMDAC输出指定的LED电流并完成LED的0电流冲击软起动保护、以及Current-
Pause功能。同时该文件负责完成DCDC输出模块的配置和自我测试。

**	History:
				2026年4月28日 14:40
														 1.针对4S输入的固件调整系统的预充DAC参数。
														 2.修改爆闪模式的下判定输出暂停成功的电压至24V适配
															 8串灯珠
														 3.修改输出通道自我测试的判定条件适配8串灯珠

				2025年12月26日 10:05 1.修改爆闪模式下判定输出暂停成功的电压至18V，提高
															 爆闪挡位的运行斜率。
														 2.针对新增的走夜路照明模式加入对应的entry确保一键
														   实时爆闪功能可以正常运行。
														 
				2025年12月20日 Initial Release
**	
*****************************************************************************/
/****************************************************************************/
/*	include files
*****************************************************************************/
#include "cms8s6990.h"
#include "PinDefs.h"
#include "GPIO.h"
#include "LEDMgmt.h"
#include "PWMCfg.h"
#include "delay.h"
#include "SpecialMode.h"
#include "TempControl.h"
#include "OutputChannel.h"
#include "ADCCfg.h"
#include "SelfTest.h"

/****************************************************************************/
/*	Local pre-processor symbols/macros for Parameter definition ('#define')
****************************************************************************/ 

//输出电流环VREF基准配置
#define CurrentCompFactor 2 //输出电流结果补偿系数（补偿硬件的增益误差带来的电流误差，单位%，-表示减小）
#define MainChannelShuntmOhm 1.00 //主通道的检流电阻阻值(mR)
#define CurrentSenseOpAmpGain 100 //电流检测放大器的增益

//极亮挡位MPPT缓升参数配置
#define TurboMPPTILEDStep 180 //输入MPPT进行电流尝试的步进值，单位1.5mA per Step

//输出PWMDAC预充电压配置
#define PWMDACPreCharge 190 	//PWMDAC在正常启动流程下的预充电压(LSB=0.1V，默认系统设置为14.2V)
#define StrobeHaltVolt 215    //爆闪DAC在待机状态下的输出关闭电压

/****************************************************************************/
/*	Local pre-processor symbols/macros for Parameter Processing and Fast Op-
/*  eration with Register Operation('#define')
****************************************************************************/ 
#define GracefulShutThres ((float)(1+OneLumenOut)/(float)10)			//自动计算系统输出放电和启动模块的阈值(单位V)
#define CalcPWMDACDuty(x) ((((1167000UL-(3500UL*x))/105)*24UL)/50UL)  //使用整数方式计算PWMDAC预充电压（输入LSB=0.1V，别问怎么来的问就是魔法deepseek）

//根据电流补偿值参数自动判断并应用补偿系数的自动定义
#ifdef CurrentCompFactor
	#define OCCurrentCompVal (100+CurrentCompFactor)  //输出通道电流补偿值计算
#else
  #warning "No Current Compensate Factor(for output channel current synth)has been entered!"
	#warning "Program will use default value without any compensation. Output current will be inaccurate!"
  #define OCCurrentCompVal 100
#endif

//根据LED特性计算PWMDAC在1LM挡位运行时的输出整定
#if defined(USING_LED_FV7011I)	
	//PWMDAC在1流明挡位下的预充电压(LSB=0.1V，NBT160特殊判定，需要提高输出电压)
	#define OneLumenOut 202
#elif ( defined(USING_LED_FV7212D) | defined(USING_LED_SFT90X) )
	//PWMDAC在1流明挡位下的预充电压(LSB=0.1V，垂直核心因为Vf高需要20VV才能有足够的亮度)
	#define OneLumenOut 198 
#else 
	//PWMDAC在1流明挡位下的预充电压(LSB=0.1V，其余灯珠默认使用19.4V)
  #define OneLumenOut 195 	
#endif
	
/*自动计算系统PWMDAC的预充配置数值，请勿修改！！！！*/	
#define CVPreChargeDACVal CalcPWMDACDuty(PWMDACPreCharge)
#define OneLumenCVDACVal CalcPWMDACDuty(OneLumenOut)
#define StrobeHaltCVDACVal CalcPWMDACDuty(StrobeHaltVolt)

#if (OCCurrentCompVal < 90 | OCCurrentCompVal >110)
	//检测电流补偿值参数异常的自动判断，禁止修改以免程序异常！
  #error "Error : Illegal Current Compensate Factor for output channel current synth!"
	#error "You should put -10 to 10(including zero) into <CurrentCompFactor> define!"
#endif

#if (OneLumenCVDACVal>CVPreChargeDACVal)
	 //1LM挡位的目标输出电压所需要的PWMDAC数值高于预启动所需的数值，会引起闪烁故最终偏置的结果使用1LM的DAC值
	 #define CVPreStartDACVal OneLumenCVDACVal
	 #define OneLMDACVal OneLumenCVDACVal
#else
   //1LM挡位的目标输出电压所需要的PWMDAC数值低于预启动所需的数值，可以分开使用
	 #define CVPreStartDACVal CVPreChargeDACVal
	 #define OneLMDACVal OneLumenCVDACVal
#endif

#if (StrobeHaltCVDACVal <0 | StrobeHaltCVDACVal > 2399)
   #error "Error 009: Invalid CV PWMDAC Output Config Value for Strobe Mode Halt Output!"
#endif

#if (OneLumenCVDACVal < 0 | OneLumenCVDACVal > 2399)
   #error "Error 009: Invalid CV PWMDAC Output Config Value for One Lumen(Ultra Low mode)Output!"
#endif

#if (CVPreChargeDACVal < 0 | CVPreChargeDACVal > 2399)
   #error "Error 00A: Invalid CV PWMDAC Output Config Value for System StartUp!"
#endif

/****************************************************************************/
/*	Local type definitions('typedef')
****************************************************************************/
typedef enum  //输出通道状态机
	{
	//输出通道彻底关闭，待机状态
	OutCH_Standby=0,
	//输出通道正常启动流程
	OutCH_PWMDACPreCharge=1,  //PWMDAC预偏置
	OutCH_StartAUXPSU=2, //启动辅助PSU
	OutCH_EnableBoost=3, //启动主Boost
	OutCH_ReleasePreCharge=4, //逐步复位PreCharge DAC让输出电压慢慢爬升，从CV状态过渡到FB注入的CC状态
	OutCH_SubmitDuty=5, //应用占空比，LED爬升到目标电流
	//输出通道正常运行阶段
	OutCH_OutputEnabled=6,
	OutCH_1LumenOpenRun=7,
	//安全关闭阶段
	OutCH_GracefulShut=8,
	OutCH_WaitVOUTDecay=9,
	//输出通道在爆闪等阶段进入idle(LED断开，输出配置为19V)
	OutCH_EnterIdle=10,	
	OutCH_OutputIdle=11,
  //输出通道启动失败
	OutCH_PreChargeFailed=12
	}OutChFSMStateDef;

/****************************************************************************/
/*	Local SFR definitions('sfr' and 'sbit')
****************************************************************************/
sbit AUXEN=AUXENIOP^AUXENIOx; 					//辅助6V DCDC使能
sbit PWMDACEN=PWMDACENIOP^PWMDACENIOx;  //PWMDAC使能
sbit SYSHBLED=SYSHBLEDIOP^SYSHBLEDIOx;  //心跳LED
sbit BOOSTRUN=BOOSTRUNIOP^BOOSTRUNIOx;  //LTC3787的EN
sbit LEDMOS=LEDNegMOSIOP^LEDNegMOSIOx;  //LEDMOSFET

/****************************************************************************/
/*	Global variable definitions(declared in header file with 'extern')
****************************************************************************/
xdata volatile int Current; //目标电流(mA)
xdata int CurrentBuf; //存储当前已经上传的电流值 
bit IsCurrentRampUp;  //电流正在上升过程中的标记位（用于和MPPT试探联动）
bit IsOutputStarted;  //输出通道是否已经启动成功的标志位（特殊功能挡位下用于确保系统已启动再执行特殊功能）
bit IsOutputAllowedToRise; //信号量，极亮输出是否允许抬升
bit IsHighPowerStrobe;  //开启高频率爆闪的增强Halt标志位	
	
/****************************************************************************/
/*	Local variable definitions('static')
****************************************************************************/	
static bit IsEnableSlowILEDRamp; //标志位，是否启用慢速电流斜率控制
static xdata unsigned char PreChargeFSMTimer; //预充电状态机计时器
static xdata OutChFSMStateDef OutputFSMState; //输出控制状态机
static xdata unsigned char HBTimer; //心跳计时器

/****************************************************************************/
/*	Local constant definitions('static const')
****************************************************************************/	
static code OutChFSMStateDef NeedsOFFStateTable[]=
	{
	//该状态表记录了可以通过把目标电流设置为0实现关机的状态
	OutCH_1LumenOpenRun,	
	OutCH_ReleasePreCharge,
	OutCH_SubmitDuty,
	OutCH_OutputEnabled,
	OutCH_OutputIdle
	};

/****************************************************************************/
/*	Function implementation - local('static')
****************************************************************************/

//内部用于计算PWMDAC占空比的函数	
static float Duty_Calc(int CurrentInput)
	{
	float buf;
	//计算实际占空比
	buf=(float)CurrentInput*(float)MainChannelShuntmOhm; //输入传进来的电流(mA)并乘以检流电阻阻值(mR)得到检流电阻处的目标电压(uV)
	buf*=(float)(0.0015*(float)CurrentSenseOpAmpGain); //uV转mV并根据1.5mA per LSB换算得到实际的电流值并乘以检流放大器的增益得到运放端的整定值
	buf/=Data.MCUVDD*(float)1000; //计算出目标DAC输出电压和PWMDAC缓冲器供电电压(MCUVDD)之间的比值
	buf*=(float)OCCurrentCompVal; //转换为百分比(乘以指定的电流补偿值，补偿掉系统硬件增益误差带来的实际电流值误差)
	//结果输出	
	return buf;
	}
	
//输出通道停止主DCDC	
static void OutputChannel_StopDCDC(void)
	{
	BOOSTRUN=0;
	LEDMOS=0;
	AUXEN=0;
	}	
	
//复位PWMDAC并关闭输出
static void OutputChannel_ClearPWMDAC(void)
	{
	PWMDACEN=0;
	if(PreChargeDACDuty||PWMDuty>0)
		{
		PreChargeDACDuty=0;
		PWMDuty=0;
		IsNeedToUploadPWM=1;
		}
	}

/****************************************************************************/
/*	Global Function implementation - Initialization
****************************************************************************/

//初始化函数
void OutputChannel_Init(void)
	{
	GPIOCfgDef OCInitCfg;
	//设置结构体
	OCInitCfg.Mode=GPIO_Out_PP;
  OCInitCfg.Slew=GPIO_Fast_Slew;		
	OCInitCfg.DRVCurrent=GPIO_High_Current; //推MOSFET,需要高上升斜率
	//调用复位函数重置所有状态
  OutputChannel_DeInit();
	//开始配置IO	
	GPIO_ConfigGPIOMode(PWMDACENIOG,GPIOMask(PWMDACENIOx),&OCInitCfg);	
	GPIO_ConfigGPIOMode(AUXENIOG,GPIOMask(AUXENIOx),&OCInitCfg);			
	GPIO_ConfigGPIOMode(BOOSTRUNIOG,GPIOMask(BOOSTRUNIOx),&OCInitCfg);		
	GPIO_ConfigGPIOMode(LEDNegMOSIOG,GPIOMask(LEDNegMOSIOx),&OCInitCfg);
  GPIO_ConfigGPIOMode(SYSHBLEDIOG,GPIOMask(SYSHBLEDIOx),&OCInitCfg);	
	}

//输出通道复位
void OutputChannel_DeInit(void)
	{
	//关闭主DCDC和心跳LED
	OutputChannel_StopDCDC();
	PWMDACEN=0;
	SYSHBLED=0; 
	//系统上电时电流配置为0
	Current=0;
	CurrentBuf=0;
	IsCurrentRampUp=0;
	IsEnableSlowILEDRamp=0;
	//复位状态机
	HBTimer=0;
	OutputFSMState=OutCH_Standby;
	}	
	
/****************************************************************************/
/*	Global Function implementation - Logic Handler
****************************************************************************/	
	
//输出通道状态机的计时处理函数
void OCFSM_TIMHandler(void)
	{
	//心跳LED控制	
	if(CurrentMode->ModeIdx==Mode_1Lumen)SYSHBLED=0; //极低亮禁用心跳LED省电
	else if(GetIfOutputEnabled())SYSHBLED=1;//输出已启用，LED配置为1		
	else //待机状态下慢闪,发生故障时禁用定时器持续快闪
		{
	  if(CurrentMode->ModeIdx!=Mode_Fault&&HBTimer<4)HBTimer++;
	  else
			{
			SYSHBLED=~SYSHBLED; //翻转LED
			HBTimer=0;
			}
		}
	//状态机计时器
	if(PreChargeFSMTimer)PreChargeFSMTimer--;
	}	
	
//输出通道计算
void OutputChannel_Calc(void)
	{
	int TargetCurrent;
	unsigned char i;
	extern xdata int TurboILIM;
	//读取目标电流并应用温控加权数据
	if(Current>0)
		{
		//取出温控限流数据
		TargetCurrent=ThermalILIMCalc();
		//如果目标电流小于当前挡位的温控限制值，则应用当前设置的电流值
		if(Current<TargetCurrent)TargetCurrent=Current;
		}
	//电流值为0或者-1，直接读取目标电流值
	else TargetCurrent=Current;
	//检测系统是否需要关机，进入关机状态	
	if(!TargetCurrent)for(i=0;i<sizeof(NeedsOFFStateTable);i++)if(OutputFSMState==NeedsOFFStateTable[i])
	  {
		//如果输出状态机当前的状态为表内对应值，且系统目标电流为0，则进入关机处理
		OutputFSMState=OutCH_GracefulShut;
		DoVOUTSampleOnly=1;                  //确保快速采样模式开启，仅抓取输出电压
		//打断循环阻止继续查找
		break;
		}
	//进行输出通道状态机管理
	switch(OutputFSMState)
		{
		//输出通道故障	
		case OutCH_PreChargeFailed:
			 OutputChannel_DeInit(); //执行输出复位
		   break; 
		//输出通道待机状态
    case OutCH_Standby:
	     //复位DCDC控制和电流上升标记位
	     OutputChannel_StopDCDC();
	     IsCurrentRampUp=0;
	     IsOutputStarted=0;
		   DoVOUTSampleOnly=0;         //输出通道待机状态，确保VOUT采样模式关闭
		   IsOutputAllowedToRise=0;    //清除允许上升Flag
			 //复位PWMDAC
       OutputChannel_ClearPWMDAC();
			 //如果电流发生变更则进入启动状态
			 if(TargetCurrent>0)OutputFSMState=OutCH_PWMDACPreCharge;
			 break;
		//启动步骤1，送出PWMDAC
		case OutCH_PWMDACPreCharge:
       //启动电流整定DAC
		   PWMDACEN=1;
		   //配置PWMDAC占空比
		   if(CurrentMode->ModeIdx==Mode_1Lumen)CurrentBuf=CalcIREFValue(45); //1LM挡位下为了避免运放同相输入接地导致CC环拉死控制器，所以随便给一个初值
			 else CurrentBuf=TargetCurrent>CalcIREFValue(1500)?CalcIREFValue(1500):TargetCurrent;
			 PWMDuty=Duty_Calc(CurrentBuf);  //配置流程是如果当前电流大于1.5A，则钳位到1.5A，然后用这个初值配置PWMDAC
       //启动CV限压环DAC
       PreChargeDACDuty=CVPreStartDACVal;
       //上传占空比并跳转到下一步(启动主DCDC辅助PSU)
			 IsNeedToUploadPWM=1;
		   OutputFSMState=OutCH_StartAUXPSU;
		   break;
		//启动步骤2，启动辅助PSU（给LTC3787供电的电源）
		case OutCH_StartAUXPSU:
			 //等待PWM输出
		   if(IsNeedToUploadPWM)break;
		   delay_ms(20); //延时20mS
		   //启动辅助电源并跳转到下个状态
			 AUXEN=1;
		   PreChargeFSMTimer=20; //设置计时器最多等待2.5秒
		   OutputFSMState=OutCH_EnableBoost;
		   break;
		//启动步骤3，启动主DCDC并检查输出是否正常
		case OutCH_EnableBoost:
			//令3787EN=1，主Boost开始输出然后检测电压状态
			BOOSTRUN=1;
			//等待超时，报错
			if(!PreChargeFSMTimer)
				{
				ReportError(Fault_DCDCFailedToStart);
				OutputFSMState=OutCH_PreChargeFailed;
				}		
			//等待输出电压建立
			if(Data.OutputVoltage<17.8)break;
			if(CurrentMode->ModeIdx==Mode_1Lumen)OutputFSMState=OutCH_1LumenOpenRun;
			else OutputFSMState=OutCH_ReleasePreCharge;   //电压建立后如果是正常运行挡位，则跳转到正常运行阶段，否则跳转到1LM挡位
			break;
		//启动步骤4，逐步下调预充PWMDAC抬升输出电压
		case OutCH_ReleasePreCharge:
			//接通LED负极FET，LED开始发光
			LEDMOS=1;
			//开始逐步下调预充占空比把输出电压调到额定值
		  if(!IsNeedToUploadPWM)
				{
				//预充PWMDAC输出=0，说明预充完成,此时跳转到正常输出状态
				if(!PreChargeDACDuty)OutputFSMState=OutCH_OutputEnabled;	
				//继续进行调整，下调占空比
				else
					{
					//根据输出电流值计算下调斜率（斜率等于1+(额定电流*1.5/8)*1.5mA per Cycle）
					TargetCurrent=1+(TargetCurrent/8);
					if(TargetCurrent>200)TargetCurrent=200; 
					//根据指定的下调斜率值应用调整
					if(PreChargeDACDuty<TargetCurrent)PreChargeDACDuty=0;
					else PreChargeDACDuty-=TargetCurrent;	                 //PWMDAC在接近末尾的时候直接clear掉，否则进行逐次递减
					}					
				//标记占空比已更新，需要上传最新值	
				IsNeedToUploadPWM=1;
				}	
		  break;
		//启动步骤5：应用整定PWMDAC占空比抬升输出电流到目标值
		case OutCH_SubmitDuty:
			if(IsNeedToUploadPWM)break; //PWM正在应用中，阻止计算
		                             
			//保护LED的电流斜率限制器
			if((TargetCurrent-CurrentBuf)>CalcIREFValue(5000))IsEnableSlowILEDRamp=1; //监测到非常大的电流瞬态，避免冲爆灯珠采用软起
			if(!SysMode&&IsEnableSlowILEDRamp)
				{
				//执行switch根据挡位模式增大电流值
				switch(CurrentMode->ModeIdx)
					{
					case Mode_Turbo:
						//极亮MPPT系统，配合输入告警监测使用(在输入限流置起时停止电流爬升)
						if(!IsOutputAllowedToRise||IsInputLimited)break; 
						//正常增加电流
						CurrentBuf+=TurboMPPTILEDStep;					
						//本次抬升完毕，等待下次MPPT运算完成再继续抬升，确保MPPT每次是执行了再抬
						IsOutputAllowedToRise=0; 	
						break;  
					case Mode_FuckDog:
					case Mode_Beacon:
					case Mode_Strobe:CurrentBuf+=2000;break;
					case Mode_SOS:CurrentBuf+=800;break;
					default:CurrentBuf+=15;
					}						
				//每次执行判断，若电流已经抬升到大于目标值，进行限幅并标记电流抬升操作结束
				if(CurrentBuf>=TargetCurrent)
					{
					IsEnableSlowILEDRamp=0;
					CurrentBuf=TargetCurrent;
					}
				}
			else CurrentBuf=TargetCurrent; //直接同步		
			//更新占空比
			IsNeedToUploadPWM=1;
			PWMDuty=Duty_Calc(CurrentBuf);
			//占空比已同步，跳转到正常运行阶段
			if(TargetCurrent==CurrentBuf)
				{
				IsCurrentRampUp=1; //标记电流爬升结束
				OutputFSMState=OutCH_OutputEnabled;
				}		  
	    break;
		//正常运行，1流明开环运行挡位
		case OutCH_1LumenOpenRun:			
			//系统尝试进入普通月光，返回到下调占空比模式
		  if(TargetCurrent>2)OutputFSMState=OutCH_ReleasePreCharge;				
			//下调预充PWMDAC占空比让LED从关闭逐步过渡到正常发光
			if(PreChargeDACDuty>OneLMDACVal&&!IsNeedToUploadPWM)
				{
				PreChargeDACDuty--; 
				IsNeedToUploadPWM=1;
				}
			//接通LED负极FET，LED开始发光
			LEDMOS=1;		  				
		  break;
		//输出通道正常运行阶段
		case OutCH_OutputEnabled:
			//系统启动结束，标志位置1
			IsOutputStarted=1;
		  //输入2，进入1LM模式
			if(TargetCurrent==2)
 				{
 				#define OneLMEnterPreDACVal (OneLMDACVal+20)
 				//进入1流明开环运行模式
 				OutputFSMState=OutCH_1LumenOpenRun; 
 				PreChargeDACDuty=OneLMEnterPreDACVal;
				PWMDuty=Duty_Calc(CalcIREFValue(45));
 				#undef OneLMEnterPreDACVal
 				}
			if(TargetCurrent==-1)OutputFSMState=OutCH_EnterIdle;	//系统电流配置为-1，说明需要暂停LED电流，跳转到暂停流程
			if(TargetCurrent!=CurrentBuf)OutputFSMState=OutCH_SubmitDuty; //占空比发生变更，开始进行处理
			break;
		//输出通道软关机控制
		case OutCH_GracefulShut:
			//先关闭DCDC，延时1mS后接通LED的MOS利用LED进行放电
			BOOSTRUN=0;
		  delay_ms(1);
			LEDMOS=1; 
			//复位PWMDAC
			OutputChannel_ClearPWMDAC();
		  //跳转到等待输出电压衰减的过程
		  PreChargeFSMTimer=36; 										//等待输出电压衰减的过程最多等待4.5秒
		  OutputFSMState=OutCH_WaitVOUTDecay;
		  break;
		//DCDC关闭，等待输出电压衰减
		case OutCH_WaitVOUTDecay:
		  //等待输出电压衰减
		  if(Data.OutputVoltage>GracefulShutThres&&PreChargeFSMTimer)break;
		  //输出电压衰减结束，关闭ADC仅采电压的模式
		  DoVOUTSampleOnly=0;
			//输出电压衰减结束，执行DCDC复位函数关闭LEDMOS和辅助电源以及PWMDAC
			OutputChannel_ClearPWMDAC();
			OutputChannel_StopDCDC();
	    //返回待机状态
		  PreChargeFSMTimer=0; //复位计时器
	    OutputFSMState=OutCH_Standby;
			break;
		//需要暂时关闭LED，在进入idle之前的准备
		case OutCH_EnterIdle:
			//立即让预充PWMDAC把电压钳住
		  if(IsHighPowerStrobe)PreChargeDACDuty=StrobeHaltCVDACVal;
		  else PreChargeDACDuty=CVPreStartDACVal;
		
		  //刷新占空比，然后调整ADC策略立即切换到仅采样输出电压
		  IsNeedToUploadPWM=1;
		  DoVOUTSampleOnly=1;                  //为了增加响应速度，在进入Idle的时候立即让ADC改为仅抓取输出电压的模式
			//等待输出电压下降
			if(Data.OutputVoltage>(IsHighPowerStrobe?23.5:21.5))break;
		  LEDMOS=0; 											 	//断开LEDMOS切断电流
		  OutputFSMState=OutCH_OutputIdle; 	//进入idle状态
		  DoVOUTSampleOnly=0;              	//顺利结束下降阶段，恢复正常采样
		  break;
		//暂时关闭LED的等待
		case OutCH_OutputIdle:
			if(TargetCurrent>0) //LED电流重新打开，需要启动输出
				{
				LEDMOS=1; //打开LEDMOS，接通电流
				PreChargeDACDuty=0;
				IsNeedToUploadPWM=1; //令预充PWMDAC开始向下调整，LED发光
			  OutputFSMState=OutCH_SubmitDuty; //应用最新的占空比
				}
			break;
		//卡出来的非法状态回到默认待机
		default:OutputFSMState=OutCH_PreChargeFailed;
		}
	}

/****************************************************************************/
/*	Global Function implementation - Output Controller Status Query
****************************************************************************/		

//外部获取输出是否正常启用的函数
bit GetIfOutputEnabled(void)	
	{
	/**********************************************	
	系统在开环运行、应用占空比和输出已正常启用的时
	候，返回启用状态。这里为什么不用三个if或者查表
	是利用了三种状态处于连续的位置：
	OutCH_SubmitDuty为ID5
	OutCH_OutputEnabled为ID6
	OutCH_1LumenOpenRun为ID7
	这样子比三个If或者switch要省很多空间。
	**********************************************/
	if(OutputFSMState>4&&OutputFSMState<8)return 1;
	//否则返回0
	return 0;
	}

//获取系统是否在安全关机阶段
bit GetIfSystemInPOFFSeq(void)
	{
	/************************************************
	如果系统处在软关机的等待阶段则返回1，不允许系统接
	受任何新的用户操作，直到输出已经完成放电，输出电
	容里面没有多余的电能后才允许继续接收用户操作。
	************************************************/
	if(OutputFSMState==OutCH_GracefulShut||
		 OutputFSMState==OutCH_WaitVOUTDecay)return 1;
	//系统已经关闭，返回0
	return 0;
	}	

/****************************************************************************/
/*	Global Function implementation - Power-On Self Test Handler
****************************************************************************/			

//输出通道初始化之后，等待电池就绪的函数
void OutputChannel_WaitVBattReady(void)
	{
	unsigned char retry=200;
	//打开心跳LED指示系统上电
	SYSHBLED=1;
	//开始自检
	do
		{
		//延迟10mS，等待电池电池测量
		delay_ms(10);
		SystemTelemHandler();
		SYSHBLED=retry&0x08?1:0;
		//电池电压小于10V，电压未稳定，继续等待
   	if(Data.RawBattVolt<10.0)continue;		
		//旁路二极管完成输出电容预充，系统可以继续启动了
		if((Data.RawBattVolt-Data.OutputVoltage)<0.25)return;	
		}
	while(--retry);
	//等待2秒后电池电压仍然异常，系统无法工作，亮红灯锁死
	LEDMode=LED_Red;
	LEDControlHandler();
	while(1)
		{
		//心跳LED高频快闪指示系统上电异常
		delay_ms(20);
		SYSHBLED=~SYSHBLED;
		}
	}
	
//输出通道试运行
void OutputChannel_TestRun(void)
	{
	unsigned char retry=100;
	float BeforeDCDCVolt;
	//记录DCDC启动前的输出电压并暂时关闭心跳LED
  SystemTelemHandler();		
	SYSHBLED=0;
	BeforeDCDCVolt=Data.OutputVoltage;
	//打开辅助电源和PWMDAC
	AUXEN=1;
	PWMDACEN=1;
	PWM_ForceEnableOut(1);
	//延迟50mS后，令3787EN=1，启动输出
	delay_ms(50);
	BOOSTRUN=1; 
	//启动输出后循环读取DCDC的输出电压检查DCDC模块，预充系统是否正常
	do
		{
		SystemTelemHandler();
		//DCDC输出过压，立即关闭系统并报错
		if(BeforeDCDCVolt<17.5&&Data.OutputVoltage>20.0)
			{
			ReportError(Fault_DCDCPreChargeFailed);
			break;
			}
		//DCDC输出正常建立，退出
		else if(retry<90&&Data.OutputVoltage>18.4)break;
		//检查失败，延时5mS后再试
		delay_ms(5);
		}
	while(--retry);		
	//检查结束，关闭DCDC并复位PWMDAC
	PWM_ForceEnableOut(0);
	OutputChannel_DeInit();
	//根据超时结果判断是否异常
	if(!retry)ReportError(Fault_DCDCFailedToStart);
	}
/*********************************  End Of File  ************************************/
