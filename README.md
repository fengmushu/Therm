[ROM]
- PB8 - i2c CLK
- PB9 - i2c DAT
- 加速度传感器
- 外接 I2C-ROM (预留设计)

[SPK]
- PD07 - 蜂鸣器

[SPI-LCD]
- PA12 - MOSI
- PA15 - CS
- PB03 - CLK
- PA11 - MISO
- PC10 - D/C#S
- PC11 - RESET

[J1-SWIO-DEBUG]
- PA14 - SW-CLK
- PA13 - SW-DIO
- VCC
- GND
- RSTB - PIN-7

[J2-FW-DEBUG]
- PA09 - UART-FW-TX
- PA10 - UART-FW-RX
- PD00 - UART-DBG-TX

[KEY]
- PD04 KEY - TRIGGER

[Anlog]
- PB07 - SYSTEM-POWER

[PWM-COUNTER]
- 

[LED]
- LED-Light - PC13


#### 系统的原理：
1. 时分采样（微小的时间间隔两个脉冲，可视为同一时刻）红光和红外光
2. 采样频率100hz左右
3. 根据采样数据，还原电流的交流和直流分量
4. R值=(ACir/DCir)/（ACred/DCred）交流分量代表微动脉舒张（高痒血蛋白），直流分量代表环境背景（骨头，组织）
5. 带入二次函数求解（系数a,b,c是经验值，或者校准时算出）

#### 校准与数据保存
1. Flash 末尾块保存校准/配置信息(最大块容量 512Byte);
2. 校准数据需要Magic-Number, 防止软件版本变更数据结构;
3. 校准数据里面需要包含传感器前端的参数/硬件支持类型;
4. 采样电路的类型, 软件能做到自动识别(基于配置电阻/器件接口电平?);
5. 工程模式下: 启动/Dashboard可以打出相关硬件调试信息;
