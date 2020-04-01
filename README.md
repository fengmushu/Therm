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


('Vi(Tt)', array([-2.420154e+06,  5.435100e+04,  4.950000e+02]))
('Vi(Te)', array([ 1.589544e+06, -5.435100e+04, -4.950000e+02]))
('Ra(Te)', array([ 2.84148615e+05, -9.89025670e+03,  1.02357699e+02]))

ViTt = At + Bt * Tt + Ct * Tt^2;
ViTe = Ae + Be * te + Ce * te^2;

原理:  
1. 传感器里面有两个单元: 光敏电池(辐射热堆) 和 温敏电阻(测环境温度); 
2. 光敏电池的电压 Vir 与 辐射源的温度 Tt 存在一张离散数据表 Vir-Tt;
3. 温敏电阻阻值 Ra 与 环境温度 Te 存在一张离散的数据表 Ra-Te;
4. 校准先后分为两步:
1. 标准温度下, 标定黑体
              * 操作: 黑体和环境都是35°和42°的稳定环境下, 测出对应的标定温度: 如 35.3, 41.8 分别保存到设备里面H,L
       2. 恒定黑体下, 标定环温
              * 操作: 改变环境温度, 测出相对于固定黑体温度的值, 计算NTC-变温校准系数