/****************************************************************************/
/** \file NTC.c
/** \Author [NTC resistor LUT generator BOT] @ redstoner_35
/** \Project Xtern Ripper Hyper Boost HV 4S-GaN Edition 
/** \Description 这个文件负责实现根据NTC读回的阻值反向计算温度的功能（该文件由
机器自动生成，未经允许不得随意修改！！）

/** \AdditionINFO  
		This is an automatically generated file by NTC resistor LUT 
		generator. DO NOT EDIT UNLESS YOU FULLY UNDERSTAND WHAT THIS
		FILE ACTUALLY DOES!
		NTC PARAMETER:100.00KΩ @ 25℃ B4310
		Table temperature range:-18℃ to 85℃
		Total ROM space for table:378 Bytes
		Target MCU Architecture:8051 Based MCU

**	History: 
				2026年7月8日 11:46   根据使用的新NTC在25至85度的B值重新生成新的NTC查找
														 表解决系统温度测量误差过大的问题。配合硬件部分的
														 更改。
														 
				2025年12月26日 10:05 删除掉超出外壳理论温度上限的温度检测数值，将温度
														 检测范围调整到-20至85℃节约ROM空间。同时修复在热
														 敏电阻(NTC)短路或阻值异常偏低时系统报告的温度数
														 值不正确的问题。
														 
				2025年12月20日 Initial Release
**	
*****************************************************************************/
/****************************************************************************/
/*	include files
*****************************************************************************/
#include <stdbool.h>

/****************************************************************************/
/*	Local pre-processor symbols/macros('#define')
****************************************************************************/
#define TemperatureReportOffset 0	//温度反馈的偏移值（如果你发现温度不准，可以在这里对温度监测系统进行TRIM）

/****************************************************************************/
/*	Local constant definitions('static const')
****************************************************************************/
code unsigned long NTCTableTop[48]={
880644, 826107, 775326, 728019,   //-15 到 -12 摄氏度
683926, 642808,604444, 568632,    //-11 到 -8 摄氏度
535187, 503937,474724, 447402,    //-7 到 -4 摄氏度  
421837, 397905,375491, 354489, 		//-3 到 0 摄氏度   
334803, 316340,299018, 282759,   	//1 到 4 摄氏度
267491, 253148,239667, 226993,   	//5 到 8 摄氏度
215072, 203854,193293, 183348,   	//9 到 12 摄氏度
173979, 165148,156822, 148970,   	//13 到 16 摄氏度
141560, 134566,127961, 121722,   	//17 到 20 摄氏度
115827, 110254,104985, 100000,    //21 到 25 摄氏度
95282,90816,86588, 82582,   			//26 到 30 摄氏度
78786,75188,71776, 68540   				//31 到 34 摄氏度
};

code unsigned int NTCTableBottom[52]={
65469, 62555, 59788, 57160,   //35 到 38 摄氏度
54663, 52290, 50035, 47890,   //39 到 42 摄氏度
45850, 43909, 42061, 40303,   //43 到 46 摄氏度
38628, 37032, 35512, 34063,   //47 到 50 摄氏度
32681, 31364, 30107, 28908,   //51 到 54 摄氏度
27764, 26671, 25628, 24631,   //55 到 58 摄氏度
23679, 22769, 21899, 21068,   //59 到 62 摄氏度
20272, 19511, 18783, 18086,   //63 到 66 摄氏度
17419, 16780, 16169, 15582,   //67 到 70 摄氏度
15021, 14482, 13966, 13471,   //71 到 74 摄氏度
12997, 12541, 12105, 11685,   //75 到 78 摄氏度
11283, 10896, 10525, 10168,   //79 到 82 摄氏度
9826, 9497, 9180, 8876  			//83 到 86 摄氏度
};

//NTC温度换算函数
//传入参数：NTC阻值(Ω),温度是否有效的bool指针输出
//返回参数：温度值(℃)
int	CalcNTCTemp(bool *IsNTCOK,unsigned long NTCRes){
unsigned char i;
volatile unsigned long NTCTableValue;
//电阻值大于查找表阻值上限，温度异常
if(NTCRes>(unsigned long)880644)
  {
  *IsNTCOK=false;
  return -15;
  }
//电阻值小于查找表阻值的阻值下限，温度异常
if(NTCRes<(unsigned long)8876)
  {
  *IsNTCOK=false;
  return 86;
  }
//温度正常，开始查表
*IsNTCOK=true;
if(NTCRes>(unsigned long)65469)for(i=0;i<48;i++)if(NTCTableTop[i]<=NTCRes)return (int)TemperatureReportOffset+(-15+i);
for(i=0;i<52;i++)
  {
  NTCTableValue=(unsigned long)NTCTableBottom[i];
  NTCTableValue&=0xFFFF;
  if(NTCTableValue<=NTCRes)return (int)TemperatureReportOffset+(35+i);
  }
//数值查找失败，返回错误值
*IsNTCOK=false;
return 0;
}
/*************************  End Of File  ***********************/
