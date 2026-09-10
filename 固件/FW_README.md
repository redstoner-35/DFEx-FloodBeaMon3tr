powered by 35's Embedded Systems Inc. An Open-Source High Power Flood FlashLight Solution    

## 固件编译注意事项

### 固件编译环境要求

该驱动的固件源代码使用了**Keil C51 V9.61a版本**的高级编译优化特性来实现空间压缩。使用其他的keil C51版本会导致编译出来的固件因为优化不足，超出芯片容量大小而编译失败！如果您编译无法通过则可以使用[预编译固件文件夹](/%E9%A2%84%E7%BC%96%E8%AF%91%E5%9B%BA%E4%BB%B6/)内作者所编译好的二进制固件文件并提供给芯片卖家让其烧录好芯片。

### MCU的CONFIG配置

由于中微单片机内部有一组硬编码于Flash指定区域，用于设置单片机的硬件首选项（例如看门狗，调试模式，低电压保护点等）的配置Word。为了确保编译出来的固件可以在硬件内正常运行，您需要在工程配置(Options for Target '*****'->Debug选项卡->右上角的Settings按钮)的Config设置菜单，按照下表正确填写MCU的配置。错误的CONFIG设置会引起固件工作异常！

+ Power                      :  Inter 5V（适用于驱动固件） / Inter 3.3V（适用于其余的固件）        
+ WDT                        :  SOFTWARECONTROL     
+ PROTECT                    :  ENABLE              
+ FLASH_DATA_PROTECT         :  ENABLE              
+ LVR                        :  3.5V（适用于驱动固件） / 2.0V（适用于其余固件）                
+ DEBUG                      :  Disable             
+ OSC                        :  HSI                 
+ OSC_PRESCALE               :  Fosc/1              
+ HSI_FS                     :  Fhsi/1              
+ EXT_RESET                  :  DISABLE             
+ EXT_RESETSEL               :  P00                 
+ WAKEUP_WAITTIME            :  500us               
+ CPU_WAITCLOCK              :  2T                  
+ WRITE_PROTECT[0-2K]        :  DISABLE             
+ WRITE_PROTECT[2-4K]        :  DISABLE             
+ WRITE_PROTECT[4-6K]        :  DISABLE             
+ WRITE_PROTECT[6-8K]        :  DISABLE             
+ WRITE_PROTECT[8-10K]       :  DISABLE             
+ WRITE_PROTECT[10-12K]      :  DISABLE             
+ WRITE_PROTECT[12-14K]      :  DISABLE             
+ WRITE_PROTECT[14-16K]      :  DISABLE             
+ BOOT                       :  BOOT_DIS      

### 烧录器在哪买？

本项目使用到了中微半导体的CMS8S系列MCU。配套的在线烧录/调试器是中微的**CMS ICE8 PRO**。您可在中微半导体的[淘宝店](https://item.taobao.com/item.htm?abbucket=5&id=679620957040&loginBonus=1&mi_id=0000OfZj4mB4W8ntz7cuVQ23ysQDaMcod53sO6cLY24fOm8&ns=1&priceTId=214783b117872947342754623e1200&skuId=5045123690895&spm=a21n57.sem.item.16.4fe43903socVPV&utparam=%7B%22aplus_abtest%22%3A%229bba9efce8e1272f374986c080ba9508%22%7D&xxc=taobaoSearch)购买。并且安装好CMS 51的Keil支持补丁。相关补丁可以在[中微半导体官网](https://www.mcu.com.cn/)下载。安装好环境并编译通过后，您可以直接在keil中使用CMS ICE8 PRO烧录器直接将固件烧录到单片机内。由于时间有限，作者恕不提供关于keil C51环境、烧录器驱动等基础环境的搭建教程。烦请自行google或百度查找相关教程。如您实在无法成功搭建环境，请看本文件的下一个章节。

## 如果我没办法自己编译和在线烧录怎么办？

如果您所使用的IDE环境不被CMS8S单片机所支持导致无法编译或下载，作者也提供了解决方案。在工程文件夹中提供了预先编译好的固件。您可以在购买芯片时，向芯片卖家提供hex文件让其代替您完成固件烧录并将烧录完毕的芯片邮寄给您。但是有一点需要强调，偷懒让芯片卖家为您代客烧录的烧录费用极为惊人，要做好心理准备。通常四款程序，四种样品各5-10PCS的烧录费用可达300至500人民币。如果您需要让商家代客烧录，请提供如下的Option Byte和烧录流程烧录信息给卖家：

+ Power                      :  Inter 5V（适用于驱动固件） / Inter 3.3V（适用于其余的固件）        
+ WDT                        :  SOFTWARECONTROL     
+ PROTECT                    :  ENABLE              
+ FLASH_DATA_PROTECT         :  ENABLE              
+ LVR                        :  3.5V（适用于驱动固件） / 2.0V（适用于其余固件）                
+ DEBUG                      :  Disable             
+ OSC                        :  HSI                 
+ OSC_PRESCALE               :  Fosc/1              
+ HSI_FS                     :  Fhsi/1              
+ EXT_RESET                  :  DISABLE             
+ EXT_RESETSEL               :  P00                 
+ WAKEUP_WAITTIME            :  500us               
+ CPU_WAITCLOCK              :  2T                  
+ WRITE_PROTECT[0-2K]        :  DISABLE             
+ WRITE_PROTECT[2-4K]        :  DISABLE             
+ WRITE_PROTECT[4-6K]        :  DISABLE             
+ WRITE_PROTECT[6-8K]        :  DISABLE             
+ WRITE_PROTECT[8-10K]       :  DISABLE             
+ WRITE_PROTECT[10-12K]      :  DISABLE             
+ WRITE_PROTECT[12-14K]      :  DISABLE             
+ WRITE_PROTECT[14-16K]      :  DISABLE             
+ BOOT                       :  BOOT_DIS 

烧录流程（针对其余的三个固件）如下图：

![烧录流程A](/img/Burn_A.JPG)

烧录流程（针对驱动的固件）如下图：

![烧录流程B](/img/Burn_B.JPG)

烧录驱动的流程绝不应用于烧录其余两个固件，否则会导致风扇手柄运行异常！

## 为什么禁赛版本风扇把手有2个固件？

禁赛版本的风扇把手存在2款硬件PCB。这两者互不兼容。对于使用2.2uH电感进行焊接，支持单锂51W输出的单锂暴力版本，请刷写单锂暴力模式专用固件。如果你贴装了4.7uH电感在牺牲单锂输出能力的条件下解锁单双锂支持，则刷写单双锂通用版本固件。这两个固件的操作逻辑基本一致，除了单锂暴力版本固件始终开启电池配置锁并屏蔽了双锂模式的一切功能，禁止安装两节电池开机外没有其余区别。 **需要注意的是，固件一定不能刷错！** 如果单锂暴力版本硬件刷写双锂版本固件会导致单锂模式下风扇输出功率异常，且上双锂强制开机会因为峰值电流过大瞬间烧毁IC！双锂版本硬件刷写单锂暴力版本固件会导致单锂下因为输出功率未限制而引起电感过热，装两节电池后驱动直接报错无法开机。

----------------------------------------------------------------------------------------------------------------------------------
© redstoner_35 @ 35's Embedded Systems Inc.  2026