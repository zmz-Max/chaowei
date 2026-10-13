# 固件说明

本目录存放机器人嵌入式程序。目前只有电机接线测试用的小程序，正式工程（PS2 手柄 + 麦轮运动学）后续加入。

## 编号约定

四路电机与主控通道的对应关系（也是本文档后面所有代码的约定）：

| 编号 | 驱动板 | 驱动板通道 | 调速脚 | 方向脚 | 物理位置 |
| --- | --- | --- | --- | --- | --- |
| M1 | #1 | A 通道 | PA0（TIM2_CH1） | PB12 | 左前 |
| M2 | #1 | B 通道 | PA1（TIM2_CH2） | PB13 | 左后 |
| M3 | #2 | A 通道 | PA2（TIM2_CH3） | PB14 | 右前 |
| M4 | #2 | B 通道 | PA3（TIM2_CH4） | PB15 | 右后 |

## 两个版本的测试程序

按自己使用的环境挑一个：

- `test_motor/main_test.c`：STM32CubeMX + Keil / STM32CubeIDE（HAL 库）
- `test_motor/test_motor.ino`：Arduino IDE（STM32duino 支持包），用 ST-Link 或串口下载

两段程序做的事情完全一样：**逐个电机 正转 1 秒 → 停 0.5 秒 → 反转 1 秒 → 停 0.5 秒，四个都过一遍之后四轮同时正转 2 秒，然后循环。**

## CubeMX 配置要点（HAL 版需要）

| 项目 | 设置 |
| --- | --- |
| RCC | HSE 使用外部晶振 |
| SYS | Debug = Serial Wire（保留下载口，务必设置） |
| PA0~PA3 | TIM2 的 CH1~CH4，模式 PWM Generation CH1~CH4 |
| TIM2 | Prescaler = 71、Counter Period = 49、Pulse = 0、PWM Mode 1 |
| PB12~PB15 | GPIO_Output，推挽、无上下拉、初始电平 Low、速度 Low |
| 时钟树 | SYSCLK = 72MHz |

电机的 PWM 频率为 72MHz ÷ 72 ÷ 50 = **20kHz**（超出人耳听觉范围，避免电机啸叫）。

## 测试步骤

1. 先只接一片驱动板、只接一个电机（`AIN1`←PA0、`AIN2`←PB12、`STBY`←3.3V、`VM`←7.4V 母线、`GND`←母线负极）。
2. 上电前用万用表确认 `VM` 对 `GND` 有 7.4V 左右。
3. 烧入测试程序，观察电机是否能正转、停、反转。
   - 正转时转向相反 → 把该电机两根输出线对调，或者反过来理解方向脚的电平。
   - 完全不动 → 先查 `STBY` 是否接了 3.3V，再查 `VM` 是否真的有电。
4. 一个电机通过后，依次接上其余三路，最后接第二片驱动板。
5. 四路都通过后，把实测结果填到下表（这张表后面写底盘运动学要用）。

## 实测记录（待填）

| 编号 | 正转时轮子转向 | 是否正常 | 备注 |
| --- | --- | --- | --- |
| M1（左前） | 待填 | 待填 | |
| M2（左后） | 待填 | 待填 | |
| M3（右前） | 待填 | 待填 | |
| M4（右后） | 待填 | 待填 | |

---

# PS2 手柄遥控（`ps2_driver/`）

## 文件说明

- `ps2_driver/ps2.h`、`ps2_driver/ps2.c`：手柄驱动，加入工程即可（CLion 下记得把 `ps2.c` 加进 `CMakeLists.txt` 的源文件列表）
- `ps2_driver/main_ps2_test.c`：粘进 `main.c` 三段 `USER CODE` 区的测试代码

## 接线（接收板丝印 → 主控）

| 接收板 | STM32 |
| --- | --- |
| `CLK` | PA5（SPI1_SCK） |
| `CS` | PA4（普通 GPIO，软件片选） |
| `CMD` | PA7（SPI1_MOSI） |
| `DAT` | PA6（SPI1_MISO） |
| `VCC` | 3.3V |
| `GND` | GND |

## CubeMX 里要补的配置

| 项目 | 设置 |
| --- | --- |
| SPI1 | Mode = Full-Duplex Master（2Lines Full Duplex） |
| SPI1 | Hardware NSS Signal = Disable（片选自己做） |
| SPI1 | Data Size = 8 Bits、First Bit = **LSB First** |
| SPI1 | Prescaler = **256**（约 281kHz） |
| SPI1 | Clock Polarity = **High**、Clock Phase = **2 Edge** |
| SPI1 | CRC Calculation = Disabled |
| PA4 | GPIO_Output，初始电平 **High**（片选空闲为高） |

## 测试里程碑

1. **第一关：通信**。上电后 PC13 的 LED **常亮** = 手柄通信成功；如果 LED 快闪，说明没收到应答，按下面的排错清单查。
2. **第二关：遥控**。十字键 ↑ 前进、↓ 后退、← 原地左转、→ 原地右转，× 键横移，松手即停。
3. 两个都通了，中期考核的“底盘运动（单一 / 全部）”两个指标就能拍视频交材料了。

## 排错清单（LED 不常亮时按顺序查）

1. `VCC` 是不是接了 3.3V（接 5V 可能烧接收板）。
2. `CMD` / `DAT` 是否接反——这两根是交叉关系，最容易错。
3. 主控和接收板是否**共地**。
4. CubeMX 里 `First Bit` 是不是 LSB First、`Clock Polarity` 是不是 High、`Clock Phase` 是不是 2 Edge，这三个配错一定会读不到数据。
5. 接收板与手柄是否配对（多数出厂已配对，必要时按接收板上的配对键重新配对）。
6. 实在读不到，可以在 `DAT` 与 3.3V 之间并一个 10k 上拉电阻（部分接收板的 MISO 是开漏输出）。

## 摇杆读不到（值一直是 128 或 0）

说明接收板还停在数字模式。先按一下手柄上的 **ANALOG** 键（指示灯亮起），很多无线手柄需要手动切模拟模式；若仍不行，再检查 `PS2_Init()` 里那 4 条配置命令是否被执行（它们负责把接收板切到模拟模式）。
