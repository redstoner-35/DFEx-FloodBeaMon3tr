/****************************************************************************/
/** \file SH367303.c
/** \Author redstoner_35
/** \Project Xtern Ripper Laser Edition 
/** \Description 这个文件为底层设备驱动文件，负责实现BMS数字控制部分的I2C协议
								 以便于利用板载的SH367303芯片读取电池的每节电压和温度参数

**	History: Initial Release
**	
*****************************************************************************/
/****************************************************************************/
/*	include files
*****************************************************************************/
#include "GPIO.h"
#include "PinDefs.h"
#include "i2c.h"
#include "delay.h"
#include "WDCTL.h"
#include "SH367303.h"

/****************************************************************************/
/*	Local Special Function Register('sfr')
****************************************************************************/
sbit BMSSCL=BMSSCLGPIOP^BMSSCLGPIOx;
sbit BMSSDA=BMSSDAGPIOP^BMSSDAGPIOx;

/****************************************************************************/
/*	Local pre-processor symbols/macros('#define')
****************************************************************************/
#define SH36ADDR 0x1B    //SH367303的从机地址

/****************************************************************************/
/*	Function implementation - local('static')
****************************************************************************/

//计算封包的PEC8
static u8 PEC8Check(char *DIN,char Len)
	{
	unsigned char crcbuf=0x00;
	unsigned char i;
	do
		{
		//载入数据
		crcbuf=crcbuf^(*DIN);
		//计算
		i=8;
			do
			{
			if(crcbuf&0x80)crcbuf=(crcbuf<<1)^0x07;//最高位为1，左移之后和多项式XOR
			else crcbuf<<=1;//最高位为0，只移位不XOR
			}
		while(--i);
		//计算结束，载入下一个数据	
		DIN++;	
		}
	while(--Len);
	//输出结果
	return crcbuf;
	}
	
/****************************************************************************/
/*	Function implementation - Register Access
****************************************************************************/	
	
//写入芯片的寄存器
bit SH36_WriteReg(SH367303REGDef Reg,unsigned char Value)	
	{
	unsigned char buf[4];
	//填充缓存并校验CRC
	buf[0]=(SH36ADDR<<1)&0xFE;
	buf[1]=(char)Reg;
  buf[2]=Value;
	buf[3]=PEC8Check(buf,3);
	//数据计算完毕开始发送
	return I2C_SendByte(SH36ADDR,&buf[2],(char)Reg,2);	
	}

//读取芯片的16bit寄存器(成功返回0，否则返回1)
bit SH36_ReadReg16(SH367303REGDef Reg,unsigned short *Value)
	{
	unsigned char buf[6];
	unsigned char CRC;
	//读取数据
	if(I2C_ReadByte(SH36ADDR,&buf[3],(char)Reg,3))return 1;
	//按照帧格式填充剩余内容
	buf[0]=(SH36ADDR<<1)&0xFE;
	buf[1]=(unsigned char)Reg;
	buf[2]=buf[0]+1;             //写地址
	//计算校验和并比对
	CRC=PEC8Check(buf,5);
	if(CRC!=buf[5])return 1;   //CRC校验失败，数据损毁
	//数据校验通过，写数据
	*Value=(buf[3]<<8)|buf[4];   //写入LSB和MSB
	return 0;
	}

//读取芯片的8bit寄存器(成功返回0，否则返回1)
bit SH36_ReadReg8(SH367303REGDef Reg,unsigned char *Value)
	{
	unsigned char buf[6];
	unsigned char CRC;
	//读取数据
	if(I2C_ReadByte(SH36ADDR,&buf[3],(char)Reg,3))return 1;
	//按照帧格式填充剩余内容
	buf[0]=(SH36ADDR<<1)&0xFE;
	buf[1]=(unsigned char)Reg;
	buf[2]=buf[0]+1;             //写地址
	//计算校验和并比对
	CRC=PEC8Check(buf,5);
	if(CRC!=buf[5])return 1;   //CRC校验失败，数据损毁
	//数据校验通过，写数据
	*Value=buf[3];   //写入LSB和MSB
	return 0;
	}
	
//在I2C通信故障时进行总线恢复
void SH36_I2C_Recovery(void)
	{
	unsigned char i;
	GPIOCfgDef SH36InitCfg;
	//先喂狗，然后复位硬件I2C IP
	WDT_Feed();           //进行喂狗
	I2CMCR=0x80; //强制复位总线
	//配置基础参数
	SH36InitCfg.Mode=GPIO_Out_PP;
  SH36InitCfg.Slew=GPIO_Fast_Slew;		
	SH36InitCfg.DRVCurrent=GPIO_High_Current; 
	
	//配置SCL SDA为GPIO
	GPIO_ConfigGPIOMode(BMSSCLGPIOG,GPIOMask(BMSSCLGPIOx),&SH36InitCfg);  //SCL为推挽
	SH36InitCfg.Mode=GPIO_IPU;	
	GPIO_ConfigGPIOMode(BMSSDAGPIOG,GPIOMask(BMSSDAGPIOx),&SH36InitCfg); 
	GPIO_SetMUXMode(BMSSCLGPIOG,BMSSCLGPIOx,GPIO_AF_GPIO); 
	GPIO_SetMUXMode(BMSSDAGPIOG,BMSSDAGPIOx,GPIO_AF_GPIO); 
	//进行总线恢复操作
	BMSSCL=1;
	for(i=0;i<9;i++)
		{
		WDT_Feed();           //进行喂狗
		delay_ms(1);
		BMSSCL=0;
		//检测是否恢复
    if(BMSSDA)break;   //BMS SDA已经恢复为高，	退出	
		//继续Clock
		delay_ms(1);	
		BMSSCL=1;       //时钟产生上升沿	
 		}
	//配置SDA为Output，模拟发送Stop时序	
	SH36InitCfg.Mode=GPIO_Out_PP;	
	GPIO_ConfigGPIOMode(BMSSDAGPIOG,GPIOMask(BMSSDAGPIOx),&SH36InitCfg); 	
	BMSSDA=0;
	BMSSCL=0;
	delay_ms(1);
  BMSSCL=1;	
	delay_ms(1);
	BMSSDA=1;	
	//恢复完毕，将总线交还给硬件I2C控制器
	SH36InitCfg.Mode=GPIO_IPU;		
		
	GPIO_SetMUXMode(BMSSCLGPIOG,BMSSCLGPIOx,GPIO_AF_SCL); //配置为SCL
	GPIO_ConfigGPIOMode(BMSSCLGPIOG,GPIOMask(BMSSCLGPIOx),&SH36InitCfg); 
	 
	//配置GPIO(SDA)		
	GPIO_SetMUXMode(BMSSDAGPIOG,BMSSDAGPIOx,GPIO_AF_SDA); //配置为SDA
	GPIO_ConfigGPIOMode(BMSSDAGPIOG,GPIOMask(BMSSDAGPIOx),&SH36InitCfg); 
	
	//配置I2C IP核
	I2C_EnableMasterMode(); //启用主控发送模式
	I2C_ConfigCLK(0x0B); //Fsclk=Fsys/(2*10*(12+1))=200KHz (这芯片I2C太快会导致卡死) 
  //发送指令复位TWI寄存器
  delay_ms(1);
  SH36_WriteReg(SH_REG_SCONF1,0x81);
  SH36_WriteReg(SH_REG_SCONF1,0x01);	  //写入SCONF1清除TWI超时标记
	}		
	
/****************************************************************************/
/*	Function implementation - Hardware and Reg Init
****************************************************************************/

//初始化SH367303的硬件	
void SH36_HardwareInit(void)
	{
	GPIOCfgDef SH36InitCfg;
	//配置基础参数
	SH36InitCfg.Mode=GPIO_IPU;
  SH36InitCfg.Slew=GPIO_Fast_Slew;		
	SH36InitCfg.DRVCurrent=GPIO_High_Current; 
	
	//配置GPIO(SCL)
	BMSSCL=1;
	GPIO_SetMUXMode(BMSSCLGPIOG,BMSSCLGPIOx,GPIO_AF_SCL); //配置为SCL
	GPIO_ConfigGPIOMode(BMSSCLGPIOG,GPIOMask(BMSSCLGPIOx),&SH36InitCfg); 
	 
	//配置GPIO(SDA)		
	GPIO_SetMUXMode(BMSSDAGPIOG,BMSSDAGPIOx,GPIO_AF_SDA); //配置为SDA
	GPIO_ConfigGPIOMode(BMSSDAGPIOG,GPIOMask(BMSSDAGPIOx),&SH36InitCfg); 
	
	//配置I2C IP核
	I2C_EnableMasterMode(); //启用主控发送模式
	I2C_ConfigCLK(0x0B); //Fsclk=Fsys/(2*10*(12+1))=200KHz (这芯片I2C太快会导致卡死) 
	}

/*********************************  End Of File  ************************************/
