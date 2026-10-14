/**
 * @file    ps2.c
 * @brief   PS2 无线手柄接收板驱动实现
 */

#include "ps2.h"

static SPI_HandleTypeDef *s_hspi;
static GPIO_TypeDef      *s_cs_port;
static uint16_t           s_cs_pin;
static uint16_t           s_prev_btns;

#define CS_LOW()   HAL_GPIO_WritePin(s_cs_port, s_cs_pin, GPIO_PIN_RESET)
#define CS_HIGH()  HAL_GPIO_WritePin(s_cs_port, s_cs_pin, GPIO_PIN_SET)

/* 直接读写寄存器收发 1 字节。
   不用 HAL_SPI_TransmitReceive(…, 1)：F1 的 HAL 在单字节收发时存在
   读完 RXNE 前数据未就绪的问题，直接用寄存器最稳。 */
static uint8_t ps2_xfer(uint8_t tx)
{
    uint32_t guard = 100000;

    while (((s_hspi->Instance->SR & SPI_SR_TXE) == 0) && (--guard != 0)) { }
    *(volatile uint8_t *)&s_hspi->Instance->DR = tx;

    guard = 100000;
    while (((s_hspi->Instance->SR & SPI_SR_RXNE) == 0) && (--guard != 0)) { }

    return *(volatile uint8_t *)&s_hspi->Instance->DR;
}

/* 片选拉低后到第一个时钟之间需要一点延时（几微秒即可） */
static void ps2_short_delay(void)
{
    for (volatile uint32_t i = 0; i < 120; i++) {
        __NOP();
    }
}

/* 一次完整通信：拉低片选 → 收发 len 字节 → 拉高片选 → 间隔 */
static void ps2_transact(const uint8_t *tx, uint8_t *rx, uint8_t len)
{
    CS_LOW();
    ps2_short_delay();

    for (uint8_t i = 0; i < len; i++) {
        rx[i] = ps2_xfer(tx[i]);
    }

    CS_HIGH();
    HAL_Delay(1);   /* 两次轮询之间必须有间隔，太快接收板不响应 */
}

uint8_t PS2_Init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
    uint8_t rx[9];

    s_hspi    = hspi;
    s_cs_port = cs_port;
    s_cs_pin  = cs_pin;

    /* 关键：F1 的 HAL_SPI_Init() 只配置外设，不会把 SPI 使能（SPE 仍为 0）。
       本驱动是直接读写寄存器的，必须自己打开 SPI，否则写进 DR 也不会产生
       时钟，读 RXNE 会一直等不到（表现为程序卡死、LED 不闪）。 */
    __HAL_SPI_ENABLE(hspi);

    CS_HIGH();
    HAL_Delay(20);

    /* ① 进入配置模式 */
    {
        const uint8_t tx[9] = { 0x01, 0x43, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };
        ps2_transact(tx, rx, 9);
    }
    /* ② 打开模拟模式 */
    {
        const uint8_t tx[9] = { 0x01, 0x44, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };
        ps2_transact(tx, rx, 9);
    }
    /* ③ 设置模式：模拟、双摇杆 */
    {
        const uint8_t tx[9] = { 0x01, 0x4F, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00 };
        ps2_transact(tx, rx, 9);
    }
    /* ④ 退出配置模式 */
    {
        const uint8_t tx[9] = { 0x01, 0x43, 0x00, 0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A };
        ps2_transact(tx, rx, 9);
    }
    /* ⑤ 正式读一次，用第 3 个字节是否为 0x5A 判断通信是否正常 */
    {
        const uint8_t tx[9] = { 0x01, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        ps2_transact(tx, rx, 9);
    }

    s_prev_btns = 0;
    return (rx[2] == 0x5A) ? 1 : 0;
}

void PS2_Read(PS2_t *ps2)
{
    const uint8_t tx[9] = { 0x01, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint8_t rx[9] = { 0 };

    ps2_transact(tx, rx, 9);

    ps2->type      = rx[1];
    ps2->connected = (rx[2] == 0x5A) ? 1 : 0;
    ps2->analog    = (rx[1] == 0x73 || rx[1] == 0x79) ? 1 : 0;

    if (ps2->connected) {
        uint16_t now  = (uint16_t)(((uint16_t)rx[4] << 8) | rx[3]);
        ps2->btns_new = (uint16_t)(now & (uint16_t)~s_prev_btns);
        ps2->btns     = now;
        s_prev_btns   = now;

        if (ps2->analog) {
            ps2->rx = rx[5];
            ps2->ry = rx[6];
            ps2->lx = rx[7];
            ps2->ly = rx[8];
        }
    } else {
        /* 通信中断：清空状态，避免拿着旧数据继续开车 */
        ps2->btns     = 0;
        ps2->btns_new = 0;
        ps2->analog   = 0;
        s_prev_btns   = 0;
    }
}
