/**
 * @file    ps2.h
 * @brief   PS2 无线手柄接收板驱动（STM32F103 + HAL 库）
 *
 * 通信协议：同步串行（SPI Mode 3 等效），LSB first，8 位一帧。
 * 硬件连接：CLK → PA5(SPI1_SCK)、CS → PA4(GPIO)、CMD → PA7(SPI1_MOSI)、
 *           DAT → PA6(SPI1_MISO)、VCC → 3.3V、GND → GND
 */

#ifndef PS2_H
#define PS2_H

#include "main.h"

/* 按键位定义（低字节先收到，合成 16 位后与下面位对应） */
#define PSB_SELECT    0x0001
#define PSB_L3        0x0002
#define PSB_R3        0x0004
#define PSB_START     0x0008
#define PSB_UP        0x0010   /* 十字键上 */
#define PSB_RIGHT     0x0020
#define PSB_DOWN      0x0040
#define PSB_LEFT      0x0080
#define PSB_L2        0x0100
#define PSB_R2        0x0200
#define PSB_L1        0x0400
#define PSB_R1        0x0800
#define PSB_TRIANGLE  0x1000   /* △ */
#define PSB_CIRCLE    0x2000   /* ○ */
#define PSB_CROSS     0x4000   /* × */
#define PSB_SQUARE    0x8000   /* □ */

typedef struct {
    uint8_t  connected;   /* 1 = 本次通信正常 */
    uint8_t  type;        /* 0x41 数字 / 0x73 模拟 / 0x79 双震动 */
    uint8_t  analog;      /* 1 = 已进入模拟模式（能读摇杆） */
    uint16_t btns;        /* 当前按下的所有键（按位或） */
    uint16_t btns_new;    /* 本次新按下的键（用于“按一下触发一次”） */
    uint8_t  lx, ly;      /* 左摇杆 0~255，中位约 128 */
    uint8_t  rx, ry;      /* 右摇杆 0~255，中位约 128 */
} PS2_t;

/**
 * @brief  初始化手柄（含切模拟模式），并验证通信
 * @param  hspi    已初始化的 SPI 句柄（如 &hspi1）
 * @param  cs_port 片选脚端口（如 GPIOA）
 * @param  cs_pin  片选脚（如 GPIO_PIN_4）
 * @retval 1 通信正常，0 未收到应答
 */
uint8_t PS2_Init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);

/**
 * @brief  读一次手柄状态并填入 ps2
 * @note   建议每 10~20ms 调用一次，别放在中断里
 */
void PS2_Read(PS2_t *ps2);

#endif /* PS2_H */
