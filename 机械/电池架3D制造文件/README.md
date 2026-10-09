## 电池架四周导电铜柱说明

### 连接底座和顶部结构之间的M3铜柱

连接底座的四个M3铜柱为标准件。您可以在淘宝购买[80mm长度单通内六角铜柱](https://detail.tmall.com/item.htm?ali_refid=a3_420434_1006%3A1105372134%3AH%3ASXCe3ANyoPXbmVAraJEbuw%3D%3D%3A52ef91f7ae6667e6a8b897091e42451f&ali_trackid=282_52ef91f7ae6667e6a8b897091e42451f&id=631585716430&mi_id=0000LIIWLdn_YMj7CjoMTPRGpdj7MncfX_wjoA0U2IgNLMA&mm_sceneid=1_0_36426808_0_0&skuId=4942906158357&spm=a21n57.1.hoverItem.2&utparam=%7B%22aplus_abtest%22%3A%22541951b373c38511efc53b5c457439df%22%7D&xxc=ad_ztc)并且找普车师傅给你在内螺纹的端面位置车一刀车成79mm。

### 连接底座和顶部结构的中心铜柱

在底座和顶部结构之间还有一个M5的实心铜柱需要CNC车削加工。由于结构太过简单因此作者恕不提供具体的3D图纸。以下是该铜柱的要求：

+ 工件外形：实心棒料直径8mm长度79mm，材质为铬锆铜（紫铜也行但是比较废刀）表面需要精车到Ra1.6粗糙度。所有外形尺寸走负公差（允许尺寸偏差为+0mm/-0.05mm）。
+ 两端加工要求：该棒料两端钻M5*0.8内螺纹孔，有效螺纹深度11mm，内螺纹孔轴心必须和棒料外圆同轴。同轴度偏差不得超过0.05mm。

该铜柱加工没有什么技术难度，找普车师傅即可完成加工。可以自行在淘宝购买铬锆铜棒材然后给普车师傅塞点钱（塞几包烟也行）然后等他加工。自己买料的话可以买直径12mm，长度零切150mm的棒材预留足够的调直和精车余量。

## 电池架顶部按键的说明

该电池架顶部有一个用户唤醒BMS的按键，这个按键为铝材质的标准件。这个铝件可以在[淘宝](https://item.taobao.com/item.htm?id=743639180452&mi_id=0000bJEdwgYlsuBC63aEtzL8MJmbJK9RdosOENyA0DSUEzQ&spm=tbpc.boughtlist.suborder_itemtitle.1.37f12e8dYeHfiQ&skuId=5299651522961)上买到，基本不需要自己制造。如果实在买不到的话可以自行在装配体内导出按钮的3D图纸安排CNC加工（很贵很贵）。

## 电池架使用的垫片说明

该电池架总成一共使用了两种垫片，该垫片必须按照要求打样并覆盖在保护板和电池架底座之间，以及充电板和顶部散热块之间，以免发生短路！！垫片的2D图纸可以在垫片相关的文件夹内找到。对于需要的厚度和材质类型，请参考文件名。每个电池架总成需要2种、各一个垫片。作者使用的垫片的定制供应商如下：

+ 1.0厚度EPDM垫片：[鑫胜源塑胶制品](https://item.taobao.com/item.htm?id=645674577873&mi_id=0000H5fhIrOVZRHUFkXrl5Bx98Uus1YrJflWJngloZMsvCQ&spm=tbpc.boughtlist.suborder_itemtitle.1.3f432e8dWc63eN)
+ 0.2厚度青稞纸垫片：[深圳市爵达士绝缘材料有限公司](https://shop525160103.taobao.com/category.htm?spm=pc_detail.30350276.shop_block.dshopinfo.2e8d7dd6djC4Je&topIds=711970532459)

联系好愿意打样的供应商之后，将2D图档和需要的厚度要求提交给供应商即可。打样建议报多一点数量，比如说十几二十个因为太少了他们可能会因为嫌麻烦而懒得给你做。

## 保护板限位垫圈

在制造期间作者发现EPDM无法承受螺丝的扭力会导致保护板被拉变形而引起导电异常以及断线，因此在保护板和铝制电池架底座之间需要增加一个内径3mm，外径5mm厚度1mm的黄铜垫圈作为间隔避免PCB被拉变形。该垫圈可以在[淘宝](https://item.taobao.com/item.htm?id=1064116642584&mi_id=0000SXtCrwponJuqGbjYSnj59FvjHYjhziyVn3uBLncazgM&spm=tbpc.boughtlist.suborder_itemtitle.1.3f432e8dWc63eN)买到。垫圈需要在保护板制造的时候直接焊接到保护板对应位置，不能直接依靠机械锁紧力固定。对于具体的焊接位置，您可以查看工程文件中的[电池架装配总成](%E7%94%B5%E6%B1%A0%E6%9E%B6%E8%A3%85%E9%85%8D%E8%A7%86%E5%9B%BE.SLDASM)文件的装配体查看位置。

## 3D文件说明

需要外部CNC加工的五金料已经单独为您生成2D图纸和3D图纸，您只需要将图纸提交给CNC供应商即可。需要注意的是，[均衡板的导电铜柱](/3D%E5%9B%BE/%E5%9D%87%E8%A1%A1%E6%9D%BF%E6%94%AF%E6%92%91%E6%9F%B1.STEP)每个电池架需要4PCS因此订购时务必按照需要的数量订购。

----------------------------------------------------------------------------------------------------------------------------------
© redstoner_35 @ 35's Embedded Systems Inc.  2026