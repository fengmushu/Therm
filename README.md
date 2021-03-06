[ROM]
- PB8-i2c-CLK
- PB9-i2c-DAT
- 可能是用于保存传感器NTC查表的
- 合法数据才会有i2c访问记录

[SPK]
- PD7-蜂鸣器

[LCD]
- LCD[1-4] - COM0/1/2/3 - PA9/10/11/12
- LCD[5-15] - S19/18/17/16/15/14/13/12/11/10/9 - PA4/5/6/7 - PC4/5 - PB0/1/2/10/11
- LCD[16-24] - S8/7/6/5/4/3/2/1/0 - PB12/13/14/15 - PC6/7/8/9 - PA8

[P1-DEBUG]
- SW-CLK
- SW-DIO
- VCC
- GND
- RSTB

[KEY]
- KEY[1:Trigger - 5] - PD6 - PC10/11/12 - PD2


[Anlog]
- PA15 - AV+ 3V
       - AV- -> GND
- PA0 -> 1-1.2v ViR
- PA1 -> NTC+800mV
- PA2 -> NTC-2000mV

- VIN+ 960mV
- VIN- 1000mV
- Yellow-NTC--2.0V
- Green-VIN--1000mV
- Black-NTC+-800mV
- Red-VIN+-960mV

[LED]
- LED-Light - PC13


Vra = Vcc - VntcL


- ('Vi(Tt)', array([-2.420154e+06,  5.435100e+04,  4.950000e+02]))
- ('Vi(Te)', array([ 1.589544e+06, -5.435100e+04, -4.950000e+02]))
- ('Ra(Te)', array([ 2.84148615e+05, -9.89025670e+03,  1.02357699e+02]))

- ViTt = At + Bt * Tt + Ct * Tt^2;
- ViTe = Ae + Be * te + Ce * te^2;

- 29-/+0.5 步长0.1
- fAPM, fCaBase, fHumFix;

#### MUST READ !!!
1. 标定部分, 需要预留Flash最后2个Block
2. 

原理:  
1. 传感器里面有两个单元: 光敏电池(辐射热堆) 和 温敏电阻(测环境温度); 
2. 光敏电池的电压 Vir 与 辐射源的温度 Tt 存在一张离散数据表 Vir-Tt;
3. 温敏电阻阻值 Ra 与 环境温度 Te 存在一张离散的数据表 Ra-Te;
4. 校准先后分为两步:
1. 标准温度下, 标定黑体
              * 操作: 黑体和环境都是35°和42°的稳定环境下, 测出对应的标定温度: 如 35.3, 41.8 分别保存到设备里面H,L
       2. 恒定黑体下, 标定环温
              * 操作: 改变环境温度, 测出相对于固定黑体温度的值, 计算NTC-变温校准系数



#### 历史数据存储
1. 物体模式和人体模式, 各自分别保存32组数据;
2. 显示时: 根据当前所选模式, 各自读取 1) 保存的历史数据, 直接显示;
3. 历史数据存储位置为: I2C ROM的前半部分;
4. 历史数据存取方式,用环形栈结构;
5. 物/人切换后, 历史数据下标复位到栈顶;

#### 校准数据保存
1. Flash 的 0xFC00 块存放产测和校准数据(最大块容量 512Byte);
2. I2C ROM 除了保存历史数据之外, 最后64Byte也保存一份校准数据;
3. Flash的校准数据与I2C的, 写入时保持同步;
4. Flash的校准数据与I2C的, 加载时只判断有无;
5. Flash的校准数据和I2C有差异时: 以I2C保存的为准(处理传感器差异, 不需要升级软件, 修改I2C即可);
6. 校准数据需要MagicNumber区别, 防止软件版本变更数据结构;

7. 校准数据里面需要包含传感器的拟合参数 --- 有待商榷;



Rx * 100K / (Rx+100K) * I + 100K * I = 2.5

Vx = Rx * 100K / (Rx + 100K) * I

I = (2.5 - Vx) / 100K

Vx = Rx * ( 2.5 - Vx) / ( Rx + 100K )

Vx * Rx + Vx * 100K = (2.5 - Vx) * Rx

Vx * 100K = (2.5 - 2 * Vx) * Rx

Rx = Vx * 100K /  (2.5 - 2 * Vx)





1. R45 2k
2. R41,R43,R46 1k
3. R44 680k
