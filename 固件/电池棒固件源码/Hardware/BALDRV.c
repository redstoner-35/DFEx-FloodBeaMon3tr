/****************************************************************************/
/** \file BALDRV.c
/** \Author redstoner_35
/** \Project Xtern Ripper Laser Edition 
/** \Description 这个文件负责实现全域均衡模块所需的带死区PWM信号输出

**	History: Initial Release
**	
*****************************************************************************/
/****************************************************************************/
/*	include files
*****************************************************************************/
#include "delay.h"
#include "GPIO.h"
#include "PinDefs.h"
#include "cms8s6990.h"
#include "BALDRV.h"

/****************************************************************************/
/*	Local Special Register definitions('sfr' and 'sbit')
****************************************************************************/
sbit PWMPOSPin=PWMPOSIOP^PWMPOSIOx;
sbit PWMNEGPin=PWMNEGIOP^PWMNEGIOx;
sbit BALEN=BALENIOP^BALENIOx;

/****************************************************************************/
/*	Local variable definitions('static')
****************************************************************************/
static bit IsBalON;
static BALPowerDef Power;

/****************************************************************************/
/*	Static Function implementation - Local Use
****************************************************************************/	

//根据目标的预设值自动更新PWM频率
static void BAL_UpdatePWMDT(void)
	{
	//设置死区
	switch(Power)
		{
		case BalPower_Max:PWM01DT=49;break;  //最大出力使用最小死区
		case BalPower_High:PWM01DT=90;break;
		case BalPower_MHigh:PWM01DT=150;break;
		case BalPower_Mid:PWM01DT=200;break;
		case BalPower_Low:PWM01DT=255;break;  //最大出力使用最大死区
		}
	//输出功率到高区域，使用较低频率
	if(Power>BalPower_High)
		{
		PWMP0H=0x03;
		PWMP0L=0x1F;       //031Fh=799（48MHz/2/(1+PWMP0)=30KHz）
		PWMD0H=0x01;  
		PWMD0L=0x8F;  		 //48MHz/4/(1+PWMP0)=0162h=399（周期固定配置为一半，生成50%的方波）
		}	
	//输出功率到低区域，升频到37KHz附近进一步降低出力	
	else if(Power>BalPower_Mid)
		{
		PWMP0H=0x02;
		PWMP0L=0x89;       //0289h=649（48MHz/2/(1+PWMP0)=36.92KHz）
		PWMD0H=0x01;  
		PWMD0L=0x44;  		 //48MHz/4/(1+PWMP0)=0144h=324（周期固定配置为一半，生成50%的方波）		
		}		
	//系统处于最小出力阶段，进一步升频至40KHz降低出力
	else 
		{
		PWMP0H=0x02;
		PWMP0L=0x57;       //0257h=599（48MHz/2/(1+PWMP0)=40KHz）
		PWMD0H=0x01;  
		PWMD0L=0x2B;  		 //48MHz/4/(1+PWMP0)=012Bh=299（周期固定配置为一半，生成50%的方波）		
		}
	}

/****************************************************************************/
/*	Global Function implementation - Initialization and De-Initialization
****************************************************************************/	

//设置均衡管理系统的状态
void BAL_SetBalState(bit IsON,BALPowerDef TargetPower)
	{	
	//均衡状态未变更，不写寄存器
	if(IsBalON!=IsON)
		{
		//均衡被打开
		if(IsON)
			{
			//打开栅极驱动电源
			BALEN=1;
			PWMCNTE=0x00;  //关闭计数器
		
			//延迟5mS后设置目标开关频率
			delay_ms(5);
			Power=TargetPower;  		//标记频率已同步	
			BAL_UpdatePWMDT();
			//打开计数器，等待PWM完成周期更新后再打开输出
			PWMCNTE=0x03;
			PWMLOADEN=0x03; 
			while(PWMLOADEN);
			//关闭Mask功能，打开PWM互补输出。均衡开始运作
			PWMMASKE=0x00;
			
			}
		//关闭均衡
		else
			{
			//发送指令触发加载用于检测过零事件发生，然后在过零阶段切断输出
			PWMLOADEN=0x03; 
			while(PWMLOADEN);
			PWMMASKE=0x03;
			//延时5mS后，关闭栅极驱动电源
			delay_ms(5);
			BALEN=0;
			}
		//均衡启动完毕，更新结果
		IsBalON=IsON;
		}
	//更新频率
	if(Power!=TargetPower)
		{
		//关闭PWM输出，等待5mS让磁芯退磁
		PWMMASKE=0x03;
		PWMCNTE=0x00;
		delay_ms(5);
		//写周期寄存器更新死区设置系统出力
		Power=TargetPower;  								//标记死区参数已同步	
		BAL_UpdatePWMDT();
		//使能计数器，频率应用后开始输出
		PWMCNTE=0x03;
		PWMLOADEN=0x03; 
		while(PWMLOADEN);
		//PWM数据已经应用，关闭Mask开始对外发波
		PWMMASKE=0x00;
		}
	}

//初始化平衡驱动的PWM输出模块
void BAL_Init(void)
	{
	GPIOCfgDef PWMInitCfg;
	//设置结构体
	PWMInitCfg.Mode=GPIO_Out_PP;
  PWMInitCfg.Slew=GPIO_Fast_Slew;		
	PWMInitCfg.DRVCurrent=GPIO_High_Current; //推PWMDAC，不需要很高的上升斜率

	//初始化变量	
	IsBalON=0;
	Power=BalPower_Low;   //系统初始化时默认使用最低出力模式避免均衡变压器爆炸
	//配置GPIO
	BALEN=0;
	PWMPOSPin=0;
  PWMNEGPin=0; 				//PWM输出始终置0
	GPIO_ConfigGPIOMode(PWMPOSIOG,GPIOMask(PWMPOSIOx),&PWMInitCfg); 
	GPIO_ConfigGPIOMode(PWMNEGIOG,GPIOMask(PWMNEGIOx),&PWMInitCfg); 
	GPIO_ConfigGPIOMode(BALENIOG,GPIOMask(BALENIOx),&PWMInitCfg);
	//配置PWM发生器
	PWMCON=0x12; //PWM通道配置为互补模式，开启中心计数，关闭非对称计数功能	
	PWMOE=0x03; //打开PWM输出通道0和1
	PWM01PSC=0x01;//打开预分频器和计数器时钟 
  PWM0DIV=0xff;   //令Fpwmcnt=Fsys=48MHz(不分频)
  PWMPINV=0x00; //所有通道均设置为正常输出模式
	PWMCNTM=0x03; //通道0和1配置为自动加载模式
	PWMCNTCLR=0x03; //初始化PWM的时候复位通道0/3的定时器
	PWMDTE=0x01; 		//使能死区时间功能
	BAL_UpdatePWMDT();    //设置通道0和1的死区时间至默认最低出力
	PWMMASKD=0x00; 
	PWMMASKE=0x03; //PWM掩码功能启用，默认状态下禁止通道0和1的输出
	PWMFBKC=0x00;  //禁止PWM刹车功能
	//配置周期数据
	PWMP0H=0x03;
	PWMP0L=0x1F;       //031Fh=799（48MHz/2/(1+PWMP0)=30KHz）
	//配置占空比数据
	PWMD0H=0x01;  
	PWMD0L=0x8F;  					//48MHz/4/(1+PWMP0)=0162h=399（周期固定配置为一半，生成50%的方波）
	//使能PWM计数器并加载参数
	PWMCNTE=0x03;
	PWMLOADEN=0x03; 	
	while(PWMLOADEN); //等待PWM参数加载结束
	
	//PWM初始化完毕，将引脚启用为复用功能
	GPIO_SetMUXMode(PWMPOSIOG,PWMPOSIOx,GPIO_AF_PWMCH0);
  GPIO_SetMUXMode(PWMNEGIOG,PWMNEGIOx,GPIO_AF_PWMCH1);
	}
/*********************************  End Of File  ************************************/
