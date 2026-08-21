/****************************************************************************/
/** \file SH36_REG.c
/** \Author redstoner_35
/** \Project Xtern Ripper Laser Edition 
/** \Description 这个文件为中层驱动文件，负责实现BMS相关寄存器的采集并输出结果

**	History: Initial Release
**	
*****************************************************************************/
/****************************************************************************/
/*	include files
*****************************************************************************/
#include "SH367303.h"
#include "LEDMgmt.h"
#include "SH_REG.h"
#include "NTC.h"
#include "delay.h"
#include "SideKey.h"
#include "SysReset.h"
#include "cms8s6990.h"
#include "WDCTL.h"

/***************************************************************************/
/* Global preprocessor symbol for parameter definiton - (#define) */
/***************************************************************************/
#define BMSInitStrDepth 12  //BMS初始化步骤深度
#define TempCalValue -3 //电池温度修正值(℃)

/***************************************************************************/
/*	Local type definitions('typedef')
****************************************************************************/
typedef struct
	{
	SH367303REGDef RegAddr;    //寄存器地址
	char Value;      //下发参数值 
  char CheckMsk;   //回读检测标志位	
	}BMSConfigDef;	

/***************************************************************************/
/*	constant definitions('static code')
****************************************************************************/	
	
//电芯对应电池节数的查找表
static code SH367303REGDef VBATVoltLUT[4]=
  {
	SH_REG_CELL2H,  //VC2到VC1对应电池1
	SH_REG_CELL3H,	//VC3到VC2对应电池2
	SH_REG_CELL4H,	//VC4到VC3对应电池3
	SH_REG_CELL5H	  //VC2到VC1对应电池4
	};		
	
static code BMSConfigDef BMSInitSeq[BMSInitStrDepth]= //BMS初始化的发包指令
	{
	//Step 1，写INTEN寄存器使能VADC中断输出
		{
	  SH_REG_INTEN,
		0x04,						//令VADC_EN=1，使能VADC中断输出
		0x7F
		},
	//Step 2，写SCONF1寄存器清除异常标志位
		{
	  SH_REG_SCONF1,
		0x80,
		0x00
		},		
	//Step 3，再度写SCONF1寄存器使能充电检测
		{
	  SH_REG_SCONF1,
		0x01,		//令CHGR_EN=1，使能充放检测
		0x7F
		},	
	//Step 4，写SCONF2寄存器，使能充放电MOS
		{
	  SH_REG_SCONF2,
		0x03,  	//复位输出设置位复位功能，Alert脚输出低电平脉冲，使能充放电MOS
		0x0F
		},	
	//Step 5，写SCONF3寄存器，使能电压ADC开启连续转换电池电压和温度
		{
	  SH_REG_SCONF3,
		0x18,  	//使能VADC，采样方式为电压+温度采集，转换周期50mS
		0xFF
		},
	//Step 6，写SCONF4寄存器，关闭6-10节电芯的被动平衡功能（虽然硬件上没有但是芯片还是有这个功能所以关一下）
		{
	  SH_REG_SCONF4,
		0x00,  
		0x1F
		},	
	//Step 7，写SCONF5寄存器，关闭1-5节电芯的被动平衡功能（避免芯片功耗发热）
		{
	  SH_REG_SCONF5,
		0x00,  
		0x1F
		},			
	//Step 8，写SCONF6寄存器，配置CADC参数和硬件短路保护（虽然硬件没用上但是还是push一下配置）
		{
	  SH_REG_SCONF6,
		0xC0,  
		0xFF
		},
  //Step 9，写SCONF7寄存器配置硬件过充保护参数和充放检测阈值（虽然硬件没用上但是还是push一下配置）
		{
	  SH_REG_SCONF7,
		0x00,  
		0xFF
		},	
  //Step 10，写SCONF8寄存器配置一下过冲保护阈值的MSB（随便写个值例如4.35V，避免芯片异常）
		{
	  SH_REG_SCONF8,
		0x02,  
		0x03
		},	
	//Step 11，写SCONF9寄存器配置一下过冲保护阈值的LSB（随便写个值例如4.35V，避免芯片异常）
		{
	  SH_REG_SCONF9,
		0xDE,  
		0xFF
		},	
	//Step 11，写SCONF10寄存器上锁PowerDown功能避免系统意外关闭
		{
	  SH_REG_SCONF10,
		0x00,  
		0x00
		},			
	};
/****************************************************************************/
/*	Global variable definitions(declared in header file with 'extern')
****************************************************************************/
xdata BatteryStatuDef	BattState;  //电池状态存储
xdata ConsTempDataDef ConsBuf;    //一致性电压缓存	
	
/***************************************************************************/
/*	Function implementation - local('static')
****************************************************************************/

//如果BMS通信失败则进入该函数
static void SH_REGAccessFaultHandler(void)
	{
	extern bit TaskSel;
	unsigned char buf;
	//系统故障	,禁用看门狗避免无限重启
	StopWDT();	
	//切换到红色快闪指示通信异常
	LEDMode=LED_RedBlink_Fast;
	buf=0;
	do
		{
		//按下按键1秒强制系统重启
		delay_ms(10);
		if(!GetSideKeyRawGPIOState())buf++;
		else buf=0;
		if(buf==100)TriggerSoftwareReset();
		//执行LED控制
		if(!SysHFBitFlag)continue;
		TaskSel=TaskSel?0:1;
		if(TaskSel)LEDControlHandler();
		SysHFBitFlag=0;
		}
	while(1);
	}
	
//如果BMS的温度检测探头故障，则进入该函数
static void SH_TempFaultHandler(void)
	{
	extern bit TaskSel;
	unsigned char buf;
	//系统故障	,禁用看门狗避免无限重启
	StopWDT();	
	//切换到黄色色快闪指示NTC异常
	LEDMode=LED_AmberBlinkFast;
	buf=0;
	do
		{
		//按下按键1秒强制系统重启
		delay_ms(10);
		if(!GetSideKeyRawGPIOState())buf++;
		else buf=0;
		if(buf==100)TriggerSoftwareReset();
		//执行LED控制
		if(!SysHFBitFlag)continue;
		TaskSel=TaskSel?0:1;
		if(TaskSel)LEDControlHandler();
		SysHFBitFlag=0;
		}
	while(1);	
	}	

//上电的时候Push寄存器值
static void SH36_PushRegValue(void)
	{
	unsigned char index=0,retry;
	unsigned char buf;
	//循环按顺序下发指令	
	do
		{
		retry=0;
		//尝试下发命令
		while(retry<50)
			{
			retry++;
			//写寄存器
			if(SH36_WriteReg(BMSInitSeq[index].RegAddr,BMSInitSeq[index].Value))
				{
				//操作失败则尝试恢复
				SH36_I2C_Recovery();
				delay_ms(15);
				continue;
				}
				
			//写完之后回读
			if(SH36_ReadReg8(BMSInitSeq[index].RegAddr,&buf))
				{
				//操作失败则尝试恢复
				SH36_I2C_Recovery();
				delay_ms(15);
				continue;
				}
			buf&=BMSInitSeq[index].CheckMsk;  //对回读值进行mask
			
			//若回读结果正确，打断重试循环，否则延迟10mS后再度通信
			if(buf==(BMSInitSeq[index].Value&BMSInitSeq[index].CheckMsk))break;
			delay_ms(15);
			}
		//判断重试结果
    if(retry==50)SH_REGAccessFaultHandler();  //寄存器读取失败，进入错误处理		
		//下发成功，指向下一组命令下发
		index++;
		}
  while(index<BMSInitStrDepth);
	}

/***************************************************************************/
/*	Function implementation - Global
****************************************************************************/
//转移采集的电池和温度数据到缓存区	
void SH36_TransferBattState(void)
	{
	unsigned char i;
	for(i=0;i<4;i++)ConsBuf.BatteryVoltTemp[i]=BattState.CellVoltage[i]; //转移电压数据
	ConsBuf.VDiff=BattState.Vdiff;                                       //转移压差
	}	
	
//让SH367303采集电池和温度数据
SHADCConvertResultDef SH36_ConvertSysState(void)
	{
	unsigned char buf,i;
	unsigned short regout;
	float Vbuf;
	//读取FLAG2寄存器检测VADC是否转换完毕
	if(SH36_ReadReg8(SH_REG_Flag2,&buf))return SH_I2CCommErr;
	//转换完毕
	if(buf&0x01)
		{
		//清零压差结果	
		BattState.Vdiff=0;	
		//读取BMS采集器内置温感的温度
		if(SH36_ReadReg16(SH_REG_TEMP1H,&regout))	
			{
			if(SH36_ReadReg16(SH_REG_TEMP2H,&regout))return SH_I2CCommErr;	
			}
		regout&=0x0FFF;																//去掉无效的高位避免干扰结果
    Vbuf=(0.17*(float)regout)-(float)270;         //使用手册自带公式计算温度值
		BattState.BMSTemp=(char)Vbuf;                 //Vbuf值转回去char填写BMS温度
		//循环读取四节电芯的电压，读取的同时计算压差
		Vbuf=0;	
		for(i=0;i<4;i++)
			{
			//读取电芯电压
			if(SH36_ReadReg16(VBATVoltLUT[i],&regout))return SH_I2CCommErr;
			regout&=0x0FFF;
			Vbuf=6*((float)regout);
			BattState.CellVoltage[i]=Vbuf/(float)4096;        //V
			//计算压差第一步，遍历所有电池找到电压最高的电池
			if(BattState.Vdiff<BattState.CellVoltage[i])BattState.Vdiff=BattState.CellVoltage[i];
			}
		//第二步，找到电压最低的电池
		Vbuf=6;
		for(i=0;i<4;i++)if(Vbuf>BattState.CellVoltage[i])Vbuf=BattState.CellVoltage[i];
		BattState.Vmin=Vbuf;                                      //存储电压最低的电池用于管理一致性
		//计算压差第三步，将最高电压的电池-最低电压的电池得到压差
		BattState.Vdiff=BattState.Vdiff-Vbuf;
		if(BattState.Vdiff<0)BattState.Vdiff*=-1; //压差始终为正
		BattState.Vdiff*=1000;                    //压差转mV	
		//读取温度结果
		if(SH36_ReadReg16(SH_REG_TS2H,&regout))return SH_I2CCommErr;	
		regout&=0x0FFF;   			//去掉无效的高位避免干扰结果
		Vbuf=(float)regout;
		Vbuf=Vbuf/((float)4096-Vbuf)*(float)10000;  //ADC公式转欧姆                       
		if(Vbuf>(float)2009298||Vbuf<(float)3588)
			{
			BattState.CellTemp=0;
			BattState.IsNTCOK=false;  //阻值超出NTC许可范围，报错
			}
		else 
			{
		  Vbuf-=100;																															//减去NTC引脚串联的100欧姆电阻
			BattState.CellTemp=(CalcNTCTemp(&BattState.IsNTCOK,(unsigned long)Vbuf)+TempCalValue); //在许可范围内正常转换温度
			}
		//如果BMS芯片读出的温度值异常，则报告错误
    if(BattState.BMSTemp>125||BattState.BMSTemp<-20)BattState.IsNTCOK=false;		
		}
	//芯片未就绪未完成结果刷新，不更新结果
	else return SH_WaitEOC;	
	//转换成功返回OK
  return SH_ConvertOK;	
	}
	
//强制系统进入休眠模式
void SH36_SendSleepCommand(void)
	{
	while(1)
		{
		//发送命令关闭中断和VADC
		SH36_WriteReg(SH_REG_INTEN,0x00);
		SH36_WriteReg(SH_REG_SCONF3,0x00);
		//发送命令清除系统所有的Flag
		SH36_WriteReg(SH_REG_SCONF1,0x80);
		//发送命令关闭充放MOS
		if(SH36_WriteReg(SH_REG_SCONF2,0x00))continue;
		//连续发送0x33解锁低功耗写入模式然后发送SCONF1寄存器的PD_EN，强迫芯片LDO掉电
		if(SH36_WriteReg(SH_REG_SCONF10,0x33))continue;
		if(SH36_WriteReg(SH_REG_SCONF1,0x20))continue;
		}
	}	
	
//进行SH36的寄存器初始化和转换
void SH36_RegInit(void)
	{
	unsigned char retry=100,NTCOKCount,ntcconvretry=20;
	SHADCConvertResultDef Result;
	//初始化寄存器下发配置
	SH36_PushRegValue();
	//尝试执行首次转换，如果失败则进入错误处理
	do
		{
		//循环等待期间喂狗避免系统卡死	
		WDT_Feed();
		//尝试执行转换
		Result=SH36_ConvertSysState();
		if(Result==SH_ConvertOK)break;
		//如果转换结果为I2C错误，执行I2C恢复程序
    if(Result==SH_I2CCommErr)SH36_I2C_Recovery();			
		//本次转换未完成，延迟10mS再试
		delay_ms(10);
		retry--;
		}		
	while(retry);
	//转换结束，如果转换超时，也进入错误处理	
	if(!retry)SH_REGAccessFaultHandler();		
	//检查NTC情况
	SH36_TransferBattState();   //转移初次读取的结果到对应缓存
  retry=100;
	NTCOKCount=0;
	do
		{
		//循环等待期间喂狗避免系统卡死	
		WDT_Feed();
		//尝试执行转换
		Result=SH36_ConvertSysState();
		if(Result==SH_ConvertOK)
			{
			//转换完毕，如果在转换结束后NTC为正常，则累计计数
			if(BattState.IsNTCOK)NTCOKCount++;
			else NTCOKCount=0;
			//NTC持续正常超过100mS，系统正常运行，退出
			if(NTCOKCount==10)return;
			}
		//如果转换结果为I2C错误，执行I2C恢复程序
    if(Result==SH_I2CCommErr)
			{
			ntcconvretry--;
			if(!ntcconvretry)SH_REGAccessFaultHandler(); //温度转换期间出现错误，尝试恢复总线，若错误次数过多，则报错
			SH36_I2C_Recovery();			
			}
		//本次转换未完成，延迟20mS再试
		delay_ms(10);
		retry--;
		}		
	while(retry); 		
  //NTC就绪超时，系统存在异常指示温度检测故障，锁死
	SH_TempFaultHandler();
	}
/*********************************  End Of File  ************************************/
