/****************************************************************************/
/** \file SH367303.c
/** \Author redstoner_35
/** \Project Xtern Ripper Laser Edition 
/** \Description 这个文件为中层驱动文件，负责实现BMS相关寄存器的采集并输出结果

**	History: Initial Release
**	
*****************************************************************************/
/****************************************************************************/
/*	include files
*****************************************************************************/
#include "SH_REG.h"
#include "delay.h"
#include "LEDMgmt.h"
#include "GPIO.h"
#include "FastOp.h"
#include "PinDefs.h"
#include "cms8s6990.h"
#include "Chgmgmt.h"
#include "SideKey.h"
#include "BALDRV.h"
#include "WDCTL.h"

/****************************************************************************/
/*	Local pre-processor symbols/macros('#define')
****************************************************************************/
#define BalSampleInterval 5 //开启均衡后进行一致性采样的时间间隔(秒)

/****************************************************************************/
/*	Local Special Register definitions('sfr' and 'sbit')
****************************************************************************/
sbit VINOK=VINOKIOP^VINOKIOx;  //输入OK(低有效)
sbit CHGDIS=CHGOFFIOP^CHGOFFIOx;  //关闭充电输出
sbit CHGACT=BATACTIOP^BATACTIOx;  //IP2366按钮

/****************************************************************************/
/* variable definitions('extern')
****************************************************************************/

/****************************************************************************/
/*	Global variable definitions(declared in header file with 'extern')
****************************************************************************/

unsigned char CommonSysFSMTIM;  //电压显示计时器
SysMgmtStoreDef SysState;
xdata BattVshowFSMDef VshowFSMState; //电池电压显示所需的计时器和状态机转移

/****************************************************************************/
/*	Local variable  definitions('static')
****************************************************************************/

static xdata unsigned char BattErrShowTimer; //电池异常显示计时
static xdata unsigned char BattShowTimer; //电池电量显示计时
static xdata int VbattSample; //取样的电池电压
static xdata BattStatusDef BattStateSample;  //电池状态
static xdata unsigned char BalTimingMgmtTIM;  //均衡时序管理
static xdata unsigned int ManualBalTIM;  //手动均衡计时
static bit IsReportingTemperature=0; //报告温度
static bit IsTempNegative=0;         //温度是否为负
static xdata ACTCommandDef ACTCmd;   //命令缓存
static xdata unsigned char ACTTimer; //ACT计时器
static xdata unsigned int SleepTimer;  //睡眠计时器
static xdata unsigned char INFOSYSONTimer; //在系统待机状态下每5秒闪一次提示用户电池包管理器正在运行的计时器
static xdata unsigned char ShowConsTimer; //显示一致性计时器
static xdata unsigned char BMSCommTIM;  //BMS通信超时计时器
static xdata BALPowerDef PowerCfg;        //均衡的出力管理设置
static xdata unsigned char QueryUpdateTIM;   //避免频繁轮询导致芯片死机
static xdata unsigned char BattLVTIM;        //电池低电倒计时模块 
static xdata ConsDisplayFSMDef ConsFSMState; //一致性显示状态机状态
static bit IsConsNeedUpdate;  //一致性数据需要更新了

/****************************************************************************/
/*	Local constant definitions('static const')
****************************************************************************/	
static code LEDStateDef VShowIndexCode[]=
	{
	//内部使用的先导显示表
	LED_Red,
	LED_Amber,
	LED_Green,  //正常过渡是红黄绿
	LED_Amber,
	LED_Red  //高精度模式是反过来，绿红黄
	};

/****************************************************************************/
/*	Function implementation - Local
****************************************************************************/

static bit QueryIfAnyCellOverCharge(void)
	{
	unsigned char i;
	//循环遍历四节电芯的电压
	for(i=0;i<4;i++)if(BattState.CellVoltage[i]>4.20)return 1;
	//没电池超过4.2，返回0
	return 0;
	}	
	
//根据传入参数设置一致性状态和驱动频率
static void SetConsState(void)
	{
	//执行电压数据转移	
	SH36_TransferBattState();
	//管理频率状态变更
	switch(PowerCfg)
		{
		case BalPower_Max:
			//电压差大于100mV降低死区限制电流
			if(BattState.Vdiff>100)PowerCfg=BalPower_High;
		  break;  
		case BalPower_High:
			//电压差大于180mV继续降低死区更进一步限制电流
			if(BattState.Vdiff>200)PowerCfg=BalPower_MHigh;
		  //电压差小于75mV返回到高出力
		  if(BattState.Vdiff<75&&BattState.Vmin>2.9)PowerCfg=BalPower_Max;
		  break;
		case BalPower_MHigh:
			//电压差大于350mV继续降低死区更进一步限制电流
			if(BattState.Vdiff>350)PowerCfg=BalPower_Mid;
		  //电压差小于160mV返回到高出力
		  if(BattState.Vdiff<160&&BattState.Vmin>2.8)PowerCfg=BalPower_High;	
      break;		
		case BalPower_Mid:
			//电压差大于600mV继续降低死区更进一步限制电流
			if(BattState.Vdiff>600)PowerCfg=BalPower_Low;
		  //电压差小于270mV返回到高出力
		  if(BattState.Vdiff<270&&BattState.Vmin>2.7)PowerCfg=BalPower_MHigh;	
      break;			
		case BalPower_Low:	//最小出力使用最大死区
			if(BattState.Vdiff<400&&BattState.Vmin>2.6)PowerCfg=BalPower_Mid;
			break;  		
		}
	//管理一致性状态变更
	switch(SysState.ConsState)
		{
		case BattCons_OK:   //电池一致性良好(<60mV)
			if(BattState.Vdiff>55)SysState.ConsState=BattCons_Mid;
		  break;
		
		case BattCons_Mid:  //电池一致性一般(70到200mV)
			if(BattState.Vdiff>200)SysState.ConsState=BattCons_Bad;
		  if(BattState.Vdiff<40)SysState.ConsState=BattCons_OK; 
		  break;

    case BattCons_Bad:	 //电池一致性非常非常差(200mV以上)
	    if(BattState.Vdiff<150)SysState.ConsState=BattCons_Mid;
      break;
		}
	}

//根据传入参数更新电池状态	
static void SetBattState(BattStatusDef *OrgState,float Value)
	{
	switch(*OrgState)
		{
		case Battery_Plenty: 
				if(Value<3.65)*OrgState=Battery_Mid; //电池电压小于指定阈值，回到电量中等状态
			  break;
		 //电池电量较为充足
		 case Battery_Mid:
				if(Value>3.90)*OrgState=Battery_Plenty; //电池电压3.9V回升到指定阈值
				if(Value<3.30)*OrgState=Battery_Low; //电池电压低于3.3则切换到电量低的状态
				break;
		 //电池电量不足
		 case Battery_Low:
		    if(Value>3.50)*OrgState=Battery_Mid; //电池电压高于3.5，切换到电量中等的状态
			  if(Value<3.00)*OrgState=Battery_VeryLow; //电池电压低于3.0，报告严重不足
		    break;
		 //电池电量严重不足
		 case Battery_VeryLow:
			  if(Value>3.20)*OrgState=Battery_Low; //电池电压回升到3.2，跳转到电量不足阶段
		    break;
	
	 }
	}
static void VShowFSMPrepare(void)	//准备电压显示状态机的模块
	{
	VshowFSMState=BattVdis_PrepareDis;	
	if(!VINOK||SysState.BalState!=BAL_Disable)CommonSysFSMTIM=8; //系统处于充电或者手动均衡状态，指示灯熄灭等一会
	LEDMode=LED_OFF;	
	}
//控制LED侧按产生闪烁指示电池电压的处理
static void VshowGenerateSideStrobe(LEDStateDef Color,BattVshowFSMDef NextStep)
	{
	//传入的是负数，符号位=1，通过快闪一次表示是0
	if(IsNegative8(CommonSysFSMTIM))
		{
		MakeFastStrobe(Color);
		CommonSysFSMTIM=0; 
		}
	//正常指示
	LEDMode=(CommonSysFSMTIM%4)&0x7E?Color:LED_OFF; //制造红色闪烁指示对应位的电压
	//显示结束
	if(!CommonSysFSMTIM) 
		{
		LEDMode=LED_OFF;
		CommonSysFSMTIM=10;
		VshowFSMState=NextStep; //等待一会
		}
	}
//电压显示状态机根据对应的电压位数计算出闪烁定时器的配置值
static void VshowFSMGenTIMValue(int Vsample,BattVshowFSMDef NextStep)
	{
	if(!CommonSysFSMTIM)	//时间到允许配置
		{	
		if(!Vsample)CommonSysFSMTIM=0x80; //0x80=瞬间闪一下
		else CommonSysFSMTIM=(4*Vsample)-1; //配置显示的时长
		VshowFSMState=NextStep; //执行下一步显示
		}
	}

//电池采样显示电压
static LEDStateDef VshowEnter_ShowIndex(void)
	{
	char Index;
	//执行先导显示处理
	if(CommonSysFSMTIM>9)
		{
		Index=((CommonSysFSMTIM-8)>>1)-1;
		if(IsReportingTemperature&&IsTempNegative)Index+=2;//温度播报时温度为正数，使用常规显示模式
		if(!IsReportingTemperature&&VbattSample>999)Index+=2; //电压播报时传入电压大于10V,使用常规显示模式
		return VShowIndexCode[Index];
		}
	return LED_OFF; //红黄绿闪烁之后(如果是高精度显示模式则为绿红黄)等待
	}

/****************************************************************************/
/*	Function implementation - Logical Handler
****************************************************************************/
	
//电池详细电压显示的状态机处理
void BatVshowFSM(void)
	{
	//电量显示状态机
	switch(VshowFSMState)
		{
		case BattVdis_PrepareDis: //准备显示
			if(CommonSysFSMTIM)break;
	    CommonSysFSMTIM=15; //延迟1.75秒
			VshowFSMState=BattVdis_DelayBeforeDisplay; //显示头部
		  break;
		//延迟并显示开头
		case BattVdis_DelayBeforeDisplay: 
			//头部显示结束后开始正式显示电压
			LEDMode=VshowEnter_ShowIndex();
		  if(CommonSysFSMTIM)break;
			//电池电压为大于10V的数，进行四舍五入处理保留小数点后一位的结果
		  if(VbattSample>999)
			   {
				 /********************************************************
				 这里四舍五入的原理是电池电压会被采样为整数，1LSB=0.01V。例如
				 电池电压为12.59V采样之后就会变成1259。那么此时我们需要对小数
				 点后两位进行四舍五入判断，得到一位小数的结果。由于整数结果的
				 个位实际上等于浮点的电池电压中的小数点后两位，因此我们只需要
				 通过和10求余数就可以取出小数点后结果的2位，然后如果结果大于4
				 则进行进位，令小数点后一位+1就实现了四舍五入了。对整个采样结
				 果除以10之后就会自动去掉小数点后两位的值保留1位小数。
				 *********************************************************/					 
				 if((VbattSample%10)>4)VbattSample+=10;
				 VbattSample/=10;
				 }
			//配置计时器显示第一组电压
			VshowFSMGenTIMValue(VbattSample/100,BattVdis_Show10V);
		  break;
    //显示十位
		case BattVdis_Show10V:
			VshowGenerateSideStrobe(LED_Red,BattVdis_Gap10to1V); //调用处理函数生成红色侧部闪烁
		  break;
		//十位和个位之间的间隔
		case BattVdis_Gap10to1V:
			VbattSample%=100;
			VshowFSMGenTIMValue(VbattSample/10,BattVdis_Show1V); //配置计时器开始显示下一组	
			break;	
		//显示个位
		case BattVdis_Show1V:
		  VshowGenerateSideStrobe(LED_Amber,BattVdis_Gap1to0_1V); //调用处理函数生成黄色侧部闪烁
		  break;
		//个位和十分位之间的间隔		
		case BattVdis_Gap1to0_1V:	
			//温度播报结束之后直接进入等待阶段
			if(IsReportingTemperature)
				{
				CommonSysFSMTIM=10;  
				VshowFSMState=BattVdis_WaitShowTempState; 
				}
			else VshowFSMGenTIMValue(VbattSample%10,BattVdis_Show0_1V);
			break;
		//显示小数点后一位(0.1V)
		case BattVdis_Show0_1V:
		  VshowGenerateSideStrobe(LED_Green,BattVdis_WaitShowChargeLvl); //调用处理函数生成绿色侧部闪烁
			break;
		//等待一段时间后显示当前温度水平
		case BattVdis_WaitShowTempState: 
			if(CommonSysFSMTIM)break;
			VshowFSMState=BattVdis_ShowTempState;
		  CommonSysFSMTIM=31;
			break;
	 
		//等待当前温度水平显示结束
		case BattVdis_ShowTempState:
			if(CommonSysFSMTIM<25&&CommonSysFSMTIM&0xF8)
				{
				if(BattState.CellTemp<42)LEDMode=LED_Green;
				else if(BattState.CellTemp<48)LEDMode=LED_Amber;
				else LEDMode=LED_Red;
				}
			//显示结束，LED熄灭一段时间
			else LEDMode=LED_OFF;
			//等待温度状态显示时间到，到了之后跳转到等待用户松开按键的处理
			if(!CommonSysFSMTIM)VshowFSMState=BattVdis_ShowChargeLvl;
			break;		  
		//等待一段时间后显示当前电量
		case BattVdis_WaitShowChargeLvl:
			if(CommonSysFSMTIM)break;
			CommonSysFSMTIM=22; 
			VshowFSMState=BattVdis_ShowChargeLvl; //等待电量显示状态结束
      break;
	  //等待总体电量显示结束
		case BattVdis_ShowChargeLvl:
			IsTempNegative=0;
			IsReportingTemperature=0;  									//clear掉温度显示标志位
			VbattSample=0;                              //电压显示每次结束后，clear掉电压缓存数据
		  if(CommonSysFSMTIM>4)switch(BattStateSample)	//显示电量
				{
				case Battery_Plenty:LEDMode=LED_Green;break; //电池电量充足绿色常亮
				case Battery_Mid:LEDMode=LED_Amber;break; //电池电量中等黄色常亮
				case Battery_Low:LEDMode=LED_Red;break;//电池电量不足
				case Battery_VeryLow:LEDMode=LED_RedBlink;break; //电池电量严重不足红色慢闪
				}
			else if(CommonSysFSMTIM)LEDMode=LED_OFF;  //显示结束，LED熄灭一会
			else if(!getSideKeyNClickAndHoldEvent())VshowFSMState=BattVdis_Waiting; //用户仍然按下按键，等待用户松开,松开后回到等待阶段
      break;
		}
	}	

void TriggerTShowDisplay(bool IsBattDisplay)	//启动系统温度显示
	{
	char tempbuf;
	if(!BattState.IsNTCOK||VshowFSMState!=BattVdis_Waiting)return; //非等待显示状态禁止操作
	VShowFSMPrepare();
	//进行温度取样
	if(IsBattDisplay)tempbuf=BattState.CellTemp;	
	else tempbuf=BattState.BMSTemp;
	//转换温度数据
  VbattSample=((int)tempbuf)*10;	
	if(IsNegative8(tempbuf))
    {
	  IsTempNegative=1; 
		VbattSample*=-1;  //对负数进行归一处理并标记温度值为负
		}
	//发送指令执行温度转换	
	IsReportingTemperature=1;
	}

//启动电池电压显示
void TriggerVshowDisplay(float Voltage,BattStatusDef State)	
	{
	if(VshowFSMState!=BattVdis_Waiting)return; //非等待显示状态禁止操作
	VShowFSMPrepare();
	//进行电压取样(缩放为LSB=0.01V)
	BattStateSample=State;
	VbattSample=(int)(Voltage*100); 		
	}		

//触发总体电池播报
void TriggerTotalVBatDisplay(void)
	{
	unsigned char i;
	float buf;
	//非等待显示状态禁止操作
	if(VshowFSMState!=BattVdis_Waiting)return; 
	VShowFSMPrepare();
	//进行电压取样(缩放为LSB=0.01V)
	BattStateSample=SysState.TotalBattState;
	buf=0;	
	for(i=0;i<4;i++)buf+=BattState.CellVoltage[i];
	buf*=(float)100;                               //累加四节电池电压
	VbattSample=(int)buf; 		
	}	


/****************************************************************************/
/*	Function implementation - Consistency Display Manager
****************************************************************************/	
void ConsistencyDisplayFSM(void)
	{
	unsigned char i,MinBatt,MaxBatt;
	float buf,vdiff,Avg;	
	BattStatusDef State;
	//状态机处理
	switch(ConsFSMState)
		{
		case ConsDisplay_Waiting:break; //等待一致性显示启动阶段	
			
		case ConsDisplay_GetherBadBattery:   //获取一致性最差的电池
			
     		//等待系统刷新一致性数据
		    if(IsConsNeedUpdate)break;
			  //对电池电压进行求平均作为中心基准
		    buf=0;
			  for(i=0;i<4;i++)buf+=ConsBuf.BatteryVoltTemp[i];
		    Avg=buf/(float)4;
		    //遍历四节电池记录电压最低和最高的
		    buf=-10;
		    vdiff=10;
		    MinBatt=0;
		    MaxBatt=0;
				for(i=0;i<4;i++)
						{
						//记录最低电压的电池
						if(vdiff>ConsBuf.BatteryVoltTemp[i])
							{
							MinBatt=i;
							vdiff=ConsBuf.BatteryVoltTemp[i];
							}
						//记录电压最高的电池
						if(buf<ConsBuf.BatteryVoltTemp[i])
							{
							MaxBatt=i;
							buf=ConsBuf.BatteryVoltTemp[i];
							}
						}
			  //判断电压最低的电池和最高的电池距离平均电压基准线的电压决定显示哪节
				vdiff=Avg-ConsBuf.BatteryVoltTemp[MinBatt];
				buf=ConsBuf.BatteryVoltTemp[MaxBatt]-Avg;
				if(vdiff<0)vdiff*=-1;
        if(buf<0)buf*=-1;
        //设置显示定时器
        if(vdiff>buf)ShowConsTimer=1+MinBatt;		
        else ShowConsTimer=1+MaxBatt;        //显示压差最大的一节电池所在的位置
				//遍历完毕，准备好显示计时器的实际值并跳转到准备数值显示的状态
				if(SysState.ConsState==BattCons_OK)ShowConsTimer=0;
				else ShowConsTimer=(4*ShowConsTimer)-1;										//如果说系统一致性很好那就没必要播报电池异常的节数
				ConsFSMState=ConsDisplay_PrePareValueReport;
				break;
		case ConsDisplay_PrePareValueReport: //准备数值显示
			  //根据压差数值设置状态指示
				if(ConsBuf.VDiff<40)State=Battery_Plenty;			 //小于40mV 一致性很好
		    else if(ConsBuf.VDiff<150)State=Battery_Mid;	 //60-150mV 一致性中等
		    else if(ConsBuf.VDiff<300)State=Battery_Low;   //150-300mV 一致性很差
		    else State=Battery_VeryLow;                      //300mV以上，一致性极差
		    //启动电压显示
		    if(ConsBuf.VDiff<1000)vdiff=ConsBuf.VDiff/100;  //小于1V的压差，缩放到9.99格式播报
		    else if(ConsBuf.VDiff<4300)vdiff=ConsBuf.VDiff/1000;  //大于1V但是小于4.3V的压差使用1000缩放到V
		    else
					{
					//压差大于4.3V肯定是哪里出了问题，红色闪三次报警并且终止显示
					LEDMode=LED_RedBlinkThird;
					CommonSysFSMTIM=12;
					ConsFSMState=ConsDisplay_DelayAgain;
					break;                                 //直接退出switch不触发播报
					}
		    TriggerVshowDisplay(vdiff,State);   //发送指令给电压显示状态机，载入电压采样并启动显示
		    //跳转到等待电压显示结束的阶段
		    ConsFSMState=ConsDisplay_WaitReportFinish;
				break;
		case ConsDisplay_WaitReportFinish:   //等待数值播报完毕
		    //当前数值播报未结束，继续等待
		    if(VshowFSMState!=BattVdis_Waiting)break;
		    //数值播报结束后等待1.5秒(电压显示状态机结束后会等0.5秒所以这里1秒就行)然后才播报电池一致性最差的电池所在的节数
				if(ShowConsTimer)CommonSysFSMTIM=8;
		    LEDMode=LED_OFF;                        //如果是一致性很好的状态，则电池等待期间LED始终熄灭
		    ConsFSMState=ConsDisplay_DelayALittle;
				break;
		
		case ConsDisplay_DelayALittle:       //延时一小会显示一致性最差的电池
			
		    //延时结束，显示一致性最差的电池所在的节数
				if(!CommonSysFSMTIM)ConsFSMState=ConsDisplay_ShowBatteryCount;
				break;
		
		case ConsDisplay_ShowBatteryCount:   //显示电池数目

				//制造黄色色闪烁指示一致性最差的电池是哪节
				if(ShowConsTimer)LEDMode=(ShowConsTimer%4)&0x7E?LED_Amber:LED_OFF;
        //显示结束，额外延时一小会(1.5秒)再返回待机状态
		    else 
					{
					ConsFSMState=ConsDisplay_DelayAgain;
					CommonSysFSMTIM=12;
					LEDMode=LED_OFF; 													//延时1.5秒并且强制关闭LED
					}
				break;  

    case ConsDisplay_DelayAgain:
				//延时结束后返回待机状态
				if(!CommonSysFSMTIM)ConsFSMState=ConsDisplay_Waiting; 
		    break;
 		
		}
	}	

/****************************************************************************/
/*	Function implementation - Main LED and Timer Logic
****************************************************************************/
void ChargeMgmt_MainLEDHandler(void)
	{
	//为了避免干扰只工作一次的频闪指示，不执行控制 	
	if(IsOneTimeStrobe())return; 
	//当前电池电量显示开启，执行对应的逻辑
	if(VshowFSMState!=BattVdis_Waiting)BatVshowFSM();  //电池电压显示启动，执行状态机
	 //电池一致性显示启动，执行状态机
	else if(ConsFSMState!=ConsDisplay_Waiting)ConsistencyDisplayFSM();   
	//电池包因为温度异常或者其他异常的原因被禁充
	else if(SysState.ChargeState!=Charge_Enabled&&!VINOK)switch(SysState.ChargeState)
		{
		case Charge_DisableUserCommand:	
				if(BattErrShowTimer)break;
				BattErrShowTimer=40;
				LEDMode=LED_GreenBlinkThird; //每隔5秒绿色闪三下
		    break;		
		case Charge_DisableUnderTemp:
		case Charge_DisableOverTemp:
				if(BattErrShowTimer)break;
				BattErrShowTimer=40;
				LEDMode=LED_AmberBlinkThird; //每隔5秒闪三下
		    break;
		
		case Charge_DisableBMSFault: //触发BMS异常
				LEDMode=LED_RedBlink_Fast;
				break;
		
		case Charge_DisableNTCFault: //触发NTC异常
			  LEDMode=LED_AmberBlinkFast;
				break;

		case Charge_DisableOverChg:  //触发过充保护
			  if(BattErrShowTimer)break;
				BattErrShowTimer=40;		
				LEDMode=LED_RedBlinkThird;	//每隔5秒黄色闪三下
		}		
	//当前系统处于均衡状态，持续闪烁显示系统的一致性
	else if(SysState.BalState!=BAL_Disable)switch(SysState.ConsState)
		{
		case BattCons_OK:LEDMode=LED_GreenBlink;break;   //电池一致性良好(<60mV)
		case BattCons_Mid:LEDMode=LED_AmberBlink;break;  //电池一致性一般(70到200mV)
		case BattCons_Bad:LEDMode=LED_RedBlink;break;   //电池一致性非常非常差(200mV以上)
		}
	else if(BattShowTimer||!VINOK)switch(SysState.ConsState)
		{
		case BattCons_OK:LEDMode=LED_Green;break;   //电池一致性良好(<60mV)
		case BattCons_Mid:LEDMode=LED_Amber;break;  //电池一致性一般(70到200mV)
		case BattCons_Bad:LEDMode=LED_Red;break;   //电池一致性非常非常差(200mV以上)
		}		
	//电池棒处于待机阶段，且没有按键按下的动静，故LED设置为关闭
	else LEDMode=LED_OFF; 	
	}
	
//充电指示定时器
void ChargeMgmt_TIMHandler(void)
	{
	//电池显示计时（这里是因为开机如果一致性非常差，会生成报警）
	if(ConsFSMState==ConsDisplay_Waiting&&BattShowTimer)BattShowTimer--;
	//通用状态机处理
	if(CommonSysFSMTIM)CommonSysFSMTIM--;
	//电池异常显示计时
	if(BattErrShowTimer)BattErrShowTimer--;
	//手动均衡计时器
	if(ManualBalTIM)ManualBalTIM--;
	//均衡时序管理计时器
  if(BalTimingMgmtTIM)
		{
		BalTimingMgmtTIM--;		
		if(BalTimingMgmtTIM==1)IsConsNeedUpdate=1; //系统已经停止均衡足够的时间，标志允许均衡采样
		}
	//激活命令时序处理
	if(ACTCmd==ACT_Command_None)ACTTimer=0;
	else if(ACTTimer<88)ACTTimer++;
	//通信超时计时器处理			
	if(BMSCommTIM<8)BMSCommTIM++;
	//自动休眠计时
	if(BattLVTIM)BattLVTIM--;
  if(SleepTimer)SleepTimer--;		
	//电池一致性播报的计数器
  if(ConsFSMState==ConsDisplay_ShowBatteryCount&&ShowConsTimer)ShowConsTimer--;		
	}

/****************************************************************************/
/*	Function implementation - ACT Command Logic
****************************************************************************/
void SubmitACTCommand(ACTCommandDef Cmd)
	{
	//当前已经有指令再执行或者是非法命令，禁止系统操作
	if(ACTCmd!=ACT_Command_None||Cmd==ACT_Command_None)return;
	//复位命令计时器后提交指令
	ACTTimer=0;
	ACTCmd=Cmd;
	}
	
//激活操作逻辑函数
void ACTCommandLogicHandler(void)
	{
	switch(ACTCmd)
	
		{
		case ACT_Command_None:   //没指令
			 //没有指令，关闭ACT输出
			 CHGACT=0;
		   break;
		case ACT_Command_Start:  //单击唤醒2366
			 //单击指令，唤醒2366
		   if(ACTTimer<5)CHGACT=1;
		   else ACTCmd=ACT_Command_None;
		   break;   
	  case ACT_Command_Stop:   //产生双击强制关闭2366
			 if(ACTTimer<4)CHGACT=1;
		   else if(ACTTimer<8)CHGACT=0;
		   else if(ACTTimer<12)CHGACT=1;
		   else ACTCmd=ACT_Command_None;   //产生双击命令
		   break;
		case ACT_Command_Active:
		   if(ACTTimer<40)CHGACT=1;
		   else ACTCmd=ACT_Command_None;  //长按5秒激活电池并唤醒SOC
			 break;		
		
		case ACT_Command_Reset:  //长按11秒，强制复位SOC
		   if(ACTTimer<88)CHGACT=1;
		   else ACTCmd=ACT_Command_None;
			 break;
		}
	
	}
/****************************************************************************/
/*	Function implementation - Power_OFF Logic
****************************************************************************/	
void PowerOffLogicHandler(bit IsLowPower)
	{
	unsigned char i=0;
	bit LEDWait;
	//停止看门狗	
	StopWDT();
	//启动LED指示闪烁三次表示进入关机
	LEDMode=IsLowPower?LED_RedBlink:LED_AmberBlinkThird;	
	while(LEDMode!=LED_OFF)if(SysHFBitFlag)
		{
		//执行函数处理	
		LEDWait=LEDWait?0:1;
		if(LEDWait)
			{
		  if(IsLowPower)i++;
			if(i==20)LEDMode=LED_OFF;   //在低电量关机模式，延时20mS后关闭
			LEDControlHandler();
			}
		SysHFBitFlag=0;
		}	
	//循环等待GPIO松开至少50mS表示用户放开按键
	i=0;
	do
		{
		//等待用户放开按键
		if(GetSideKeyRawGPIOState()&&VINOK)i++;
		else i=0;
		delay_ms(5);
		}
	while(i<10);
	//关闭侧按和心跳中断以及均衡模块
	BAL_SetBalState(0,PowerCfg);	
	DisableSysHBTIM();
	SideKey_SetIntOFF();		
		
	//发送PD指令到BMS，关闭系统
	SH36_SendSleepCommand();
	}
	
//每5秒启动一次显示一致性状态，以提示用户管理器正在运行的定时器处理
void INFOTimerHandler(void)
	{
	//电池正在显示，或者输入插入，复位计时器
	if(!VINOK||BattShowTimer)INFOSYSONTimer=80;
	//当前均衡正在运行，指示灯一直亮不需要显示	
	else if(SysState.BalState!=BAL_Disable)INFOSYSONTimer=80;	
	//正常倒计时
	else if(INFOSYSONTimer)INFOSYSONTimer--;
	else
		{
		//时间到，短暂点亮提示
		INFOSYSONTimer=80;
		BattShowTimer=3;	
		}
	}

/****************************************************************************/
/*	Function implementation - Main Key Logic
****************************************************************************/	
void ChargeMgmtKeyLogic(void)
	{	
	//如果系统有电池电量过低，为了避免饿死电池，触发低电保护	
	if(!BattLVTIM)PowerOffLogicHandler(1);
	//长按强制关闭均衡系统	
	if(ManualBalTIM!=0&&getSideKeyLongPressEvent())	
			{
			ManualBalTIM=0;  //清除倒计时
			SysState.BalState=BAL_Disable;
			}
	//判断系统是否能进入睡眠
	if(ACTCmd!=ACT_Command_None||SysState.BalState!=BAL_Disable)SleepTimer=480;	
	else if(!VINOK||VshowFSMState!=BattVdis_Waiting)SleepTimer=480;               //输入有电、正在播报电量等、均衡执行过程中或者是有激活指令都不允许睡觉
	else if(!SleepTimer||getSideKeyLongPressEvent())PowerOffLogicHandler(0);       //系统允许进入睡眠，倒计时结束或者是用户手动长按，进入睡眠
	//没按键事件发生
	if(!IsKeyEventOccurred())return;
  SleepTimer=480;                   //按键按下重置计时器
	//处理短按事件
	switch(getSideKeyShortPressCount())
		{
		case 1:
			//单击在系统休眠的情况下查看一致性并唤醒2366
		  if(SysState.ChargeState==Charge_DisableUserCommand)
				{
				SysState.ChargeState=Charge_Enabled; //当前系统处于禁充模式，单击退出禁充模式
				break;
				}
		  if(ACTCmd!=ACT_Command_None)break;
      if(!VINOK||SysState.BalState!=BAL_Disable)break;  //当前有指令正在执行
			BattShowTimer=18;			
			SubmitACTCommand(ACT_Command_Start);
			break;
		
		case 2:
			//双击查看整体电池电量
		  TriggerTotalVBatDisplay();
		  break;
		case 3:
			//三击查看电池当前温度
		  TriggerTShowDisplay(true);
		  break;
    
		case 4:				
		  //四击查看电池当前的一致性
		  if(ConsFSMState!=ConsDisplay_Waiting)break;
			if(VshowFSMState!=BattVdis_Waiting)break;	    //系统已经在显示了不允许打断
			//发送指令初始化一致性显示                     
		  ConsFSMState=ConsDisplay_GetherBadBattery;
		  break;
		
		case 5:
			//五击允许用户开启手动均衡
			if(!VINOK||BattState.Vdiff<20)LEDMode=LED_RedBlinkThird; //当前处于充电或者压差足够低，闪三下提示无法开启
		  else if(SysState.BalState==BAL_Disable)                  //均衡处于关闭状态，四击强制关闭
				{
				ManualBalTIM=480*60;  //倒计时一个小时
				SysState.BalState=BAL_EnableManual;   //打开手动均衡
				}
			else if(SysState.ConsState==BattCons_OK)  //均衡已经完成，四击关闭均衡
				{
				ManualBalTIM=0;  //清除倒计时
				SysState.BalState=BAL_Disable;
				}
			break;
    
		case 6:
			//六击发送指令查看BMS板温度
			TriggerTShowDisplay(false);			
		  break;
		
		case 7:
			//七击发送指令执行电池激活
			LEDMode=LED_GreenBlinkThird;         //绿色闪三次提示已发出命令
		  SubmitACTCommand(ACT_Command_Active);
		  break;				
				
		case 8:
			//八击发送指令强制reset SoC
		  LEDMode=LED_GreenBlinkThird;         //绿色闪三次提示已发出命令
		  SubmitACTCommand(ACT_Command_Reset);
		  break;
		//其余次数，啥也不干
		default: break;
		}
	//处理N击+长按事件(查询单体电压和电量水平)
	switch(getSideKeyNClickAndHoldEvent())
		{
		case 1:TriggerVshowDisplay(BattState.CellVoltage[0],SysState.BattState[0]);break; //电池1
		case 2:TriggerVshowDisplay(BattState.CellVoltage[1],SysState.BattState[1]);break; //电池2
		case 3:TriggerVshowDisplay(BattState.CellVoltage[2],SysState.BattState[2]);break; //电池3
		case 4:TriggerVshowDisplay(BattState.CellVoltage[3],SysState.BattState[3]);break; //电池4
		
		case 5:
			  //用户手动五击+长按，在压差比较小的情况下结束自动均衡
			  if(SysState.BalState!=BAL_EnableAuto)break;  //系统未开启自动均衡，不响应操作
		    if(ConsBuf.VDiff>160)LEDMode=LED_RedBlinkThird; //提示用户压差过大无法关闭
		    else SysState.BalState=BAL_Disable;   //直接关闭均衡
		    break;
			
    case 6:
			 //用户手动6击+长按，进入禁充模式
       if(SysState.ChargeState==Charge_Enabled)SysState.ChargeState=Charge_DisableUserCommand; //进入禁充模式
		   break;
		//其余次数，啥也不干
		default: break;
		}
	
	//按键事件处理完毕，清除短按事件
	ClearShortPressEvent();
	}
/****************************************************************************/
/*	Function implementation - Main Control Logic For Balance
****************************************************************************/	
void ChargeMgmt_BalHandler(void)
	{
	unsigned char i,j;
	float buf;
	//避免频繁轮询引起芯片异常	
	if(QueryUpdateTIM)
		{
		QueryUpdateTIM--;
		delay_ms(1);
		return;
		}
	//时间到开始尝试转换
	else QueryUpdateTIM=50;
		
	switch(SH36_ConvertSysState())	
		{
		case SH_I2CCommErr:SH36_I2C_Recovery();return;  //SH I2C错误，执行返回
		case SH_ConvertOK: 
			//转换成功，继续执行下面的内容
			BMSCommTIM=0;                            //转换成功完成，复位通信超时计时器
			break;     
		default: return;               //其余状态，退出
		}

	//刷新电池和温度结果(均衡模式下为了避免导线大电流压降，每均衡几秒后休息0.5秒再采样)
	if(SysState.BalState==BAL_Disable||(IsConsNeedUpdate&&SysState.BalState!=BAL_Disable))	
		{
		SetConsState(); //更新一致性状态
		IsConsNeedUpdate=0; //一致性更新完毕，允许均衡刷新
		buf=0;
		j=0;
		for(i=0;i<4;i++)
			{
			//轮流采样每节电池的状态并且进行累加方便计算电池整体状态
			SetBattState(&SysState.BattState[i],BattState.CellVoltage[i]);
			buf+=BattState.CellVoltage[i];
			if(BattState.CellVoltage[i]>2.50)j++;   //电池电压正常，标记正常的节数+1
			}
		//判断是否发生异常	
		if(j==i)BattLVTIM=160;   //四节电池电压都高于正常值，复位计时器阻止系统低电关机	
		//更新整体电池状况
		buf/=4;
		SetBattState(&SysState.TotalBattState,buf);	 //更新整体电池状态	
		}
	//判断电池是否存在严重低压(小于2V)
  i=0;		
	for(j=0;j<4;j++)if(BattState.CellVoltage[i]<2.00)i++;
	//转换完毕，进行均衡管理
	if(SysState.ChargeState==Charge_DisableBMSFault)SysState.BalState=BAL_Disable; //BMS采集器故障，禁止系统进行均衡立即关闭均衡
	else if(i)SysState.BalState=BAL_Disable;   //检测到单节电池电压低于2V，立即禁止均衡
	else switch(SysState.BalState)
		{
		case BAL_Disable: //关闭均衡
			//为了避免瞬间巨大浪涌，均衡从关闭状态切换到开启时，始终以最低出力运行，根据实时压差自动切换
			PowerCfg=BalPower_Low;
	    //电池一致性很差而且并非低电量，启动均衡
		  if(SysState.ConsState==BattCons_Bad&&SysState.TotalBattState!=Battery_VeryLow)SysState.BalState=BAL_EnableAuto;
		  break;
	  case BAL_EnableAuto:       //由于电池一致性太差，均衡被启动
			//电池电量过低，关闭均衡
			if(SysState.TotalBattState==Battery_VeryLow)SysState.BalState=BAL_Disable; 
	    //电池一致性修回来了，关闭
			if(SysState.ConsState==BattCons_OK)SysState.BalState=BAL_Disable;
		  break;
		case BAL_EnableManual:     //均衡自动启动
			//电池电量过低，关闭均衡
			if(SysState.TotalBattState==Battery_VeryLow)SysState.BalState=BAL_Disable; 
			//电池一致性合格或者时间到，自动关闭
		  if(!ManualBalTIM||(BattState.Vdiff<20&&SysState.ConsState==BattCons_OK))SysState.BalState=BAL_Disable;
		  break;
		}
	//设置均衡使能
  if(SysState.BalState==BAL_Disable||SysState.ChargeState==Charge_DisableOverTemp)BAL_SetBalState(0,BalPower_Low);  //均衡被关闭或者电池过温，手动关闭均衡	
	else 
		{
		//循环使能均衡，每几秒钟停一下采样电压再继续
		if(!BalTimingMgmtTIM)BalTimingMgmtTIM=4+(8*BalSampleInterval);  //均衡完成一次采集，继续下次采集
		if(BattState.CellTemp>50||BattState.BMSTemp>70)PowerCfg=BalPower_Low; //电池或者均衡板过热，使用低功率均衡	
		BAL_SetBalState(BalTimingMgmtTIM>4?1:0,PowerCfg);
		}			
	//进行禁充模块管理
	if(BMSCommTIM==8)SysState.ChargeState=Charge_DisableBMSFault;           //BMS通信超时触发保护禁充
	else if(!BattState.IsNTCOK)SysState.ChargeState=Charge_DisableNTCFault;	//NTC掉线，触发禁充
	else switch(SysState.ChargeState)
		{
		case Charge_DisableUserCommand:break;                    //用户手动执行六击+长按，执行禁止充电操作
		case Charge_DisableBMSFault:
       if(BMSCommTIM<8)SysState.ChargeState=Charge_Enabled;  //通信成功，复位到允许充电状态
			 break;
    case Charge_DisableNTCFault:
   			if(BattState.IsNTCOK)SysState.ChargeState=Charge_Enabled; //NTC复位完毕，复位到允许充电阶段
		    break;
		  
		case Charge_Enabled:   //正常开启充电
		   //过温度和低温保护
			 if(BattState.CellTemp>55||BattState.BMSTemp>90)SysState.ChargeState=Charge_DisableOverTemp;
		   if(BattState.CellTemp<0)SysState.ChargeState=Charge_DisableUnderTemp;
		   //电池一致性误差检测
		   if(QueryIfAnyCellOverCharge()&&SysState.ConsState==BattCons_Bad)SysState.ChargeState=Charge_DisableOverChg;  //检测到一致性奇差无比还发生单体过冲，强制禁止充电
		   break;
			
		case Charge_DisableUnderTemp:  //因为电池温度过低，充电被禁用
			 
				if(BattState.CellTemp>15)SysState.ChargeState=Charge_Enabled;  //电池回升到10度以上，充电使能
				break;
		
		case Charge_DisableOverTemp:			//因为电池温度过高，充电被禁用
				
				if(BattState.CellTemp<45&&BattState.BMSTemp<70)SysState.ChargeState=Charge_Enabled;  //电池下降到45度以下，充电使能
				break; 
		
		case Charge_DisableOverChg:     //因为电池一致性太差且单体过冲，充电被禁用
		
				if(!QueryIfAnyCellOverCharge()&&SysState.ConsState==BattCons_Mid)SysState.ChargeState=Charge_Enabled; //没发生单体过冲且一致性压下来了，使能充电
		    break;
		}
	//设置禁充模块对应的GPIO
	CHGDIS=(SysState.ChargeState==Charge_Enabled?0:1);
	}
	
/****************************************************************************/
/*	Function implementation - Hardware and Reg Init
****************************************************************************/

//初始化充电管理器
void ChargeMgmt_Init(void)
	{
  GPIOCfgDef CHGInitCfg;
	unsigned char i,j;
	unsigned char FaultBatterycount=0;
	float buf=0;
	bit IsBattFault=0;
	//设置结构体
	CHGInitCfg.Mode=GPIO_Out_PP;
  CHGInitCfg.Slew=GPIO_Fast_Slew;		
	CHGInitCfg.DRVCurrent=GPIO_High_Current; //推CHGDAC，不需要很高的上升斜率
	//初始化变量		
	PowerCfg=BalPower_Low;  //均衡默认重置到最低出力运行
	VshowFSMState=BattVdis_Waiting;
	BattLVTIM=160;      //倒计时十秒钟
	INFOSYSONTimer=80;  //定时器每5秒闪一次
	QueryUpdateTIM=50;  //每50mS获取一次
	SleepTimer=480;    //无操作一分钟自动休眠
	BattErrShowTimer=0;
	ACTTimer=0;
	ShowConsTimer=0;
	BMSCommTIM=0;      //通信超时计时器复位
	ManualBalTIM=0;
	BalTimingMgmtTIM=4+(8*BalSampleInterval); //均衡自动计时管理
  for(i=0;i<10;i++)SetConsState(); //根据首次遥测结果更新一致性状态
	for(i=0;i<4;i++)
		{
		//轮流采样每节电池的状态并且进行累加方便计算电池整体状态
		SysState.BattState[i]=Battery_VeryLow;                      //默认处于极低电量，然后慢慢往上走
		for(j=0;j<10;j++)SetBattState(&SysState.BattState[i],BattState.CellVoltage[i]);
		buf+=BattState.CellVoltage[i];
		if(BattState.CellVoltage[i]<2.20)
			{
			IsBattFault=1; //单节电池电压小于2.2，触发保护
			if(!FaultBatterycount)FaultBatterycount=1+i;  //记录损坏电池的数目
			}
		}
	buf/=4;
	//对电池总体电量进行采集
	if(buf<2.50)IsBattFault=1; 																		//检测到总体电量过低，触发保护
	SysState.TotalBattState=Battery_VeryLow;                      //默认处于极低电量，然后慢慢往上走
	for(i=0;i<10;i++)SetBattState(&SysState.TotalBattState,buf);
  SysState.BalState=BAL_Disable;		

	//配置输出GPIO
	CHGDIS=0;
  CHGACT=0;
	GPIO_ConfigGPIOMode(CHGOFFIOG,GPIOMask(CHGOFFIOx),&CHGInitCfg); 
	GPIO_ConfigGPIOMode(BATACTIOG,GPIOMask(BATACTIOx),&CHGInitCfg); 
	//配置输入GPIO
	CHGInitCfg.Mode=GPIO_IPU;
	GPIO_ConfigGPIOMode(VINOKIOG,GPIOMask(VINOKIOx),&CHGInitCfg);
	//对电池温度进行判断执行过温和低温保护
	if(BattState.CellTemp>55||BattState.BMSTemp>90)SysState.ChargeState=Charge_DisableOverTemp;
	else if(BattState.CellTemp<0)SysState.ChargeState=Charge_DisableUnderTemp;	
  else SysState.ChargeState=Charge_Enabled; 	
	if(SysState.ChargeState!=Charge_Enabled)CHGDIS=1;  //电池温度异常，立即触发过温保护模块
		
	//启动系统电量显示并激活2366
	ACTCmd=ACT_Command_Active;
  BattShowTimer=18;		
	//如果电池一致性非常糟糕，则发送指令初始化一致性显示作为警告提醒用户，电池存在异常
	if(SysState.ConsState==BattCons_Bad)ConsFSMState=ConsDisplay_GetherBadBattery;	
	else ConsFSMState=ConsDisplay_Waiting;																					//没问题的话电池显示和一致性显示状态机都配置为等待状态
	//检测电池总体电量和单节电量是否存在过低线性，若存在则红色慢闪后关机	
	if(!IsBattFault)return;      
  //检测到单节电池存在异常，报警
	if(FaultBatterycount)do
			{ 
			//先停掉看门狗
			StopWDT(); 
			//如果是单体电池过压，则标记故障电池所在位置后，延时3秒关机
			LEDMode=LED_Amber;   
			LEDControlHandler();
			delay_ms(300);
			LEDMode=LED_OFF;   
			LEDControlHandler();
      delay_ms(300);				
			}
	while(--FaultBatterycount);
	//执行低电关闭
	PowerOffLogicHandler(1);
	}

/*********************************  End Of File  ************************************/
