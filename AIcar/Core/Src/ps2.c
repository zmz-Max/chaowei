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

/* 空转延时：-O0 编译下每轮约 8~12 个周期，200 轮约 25us 左右 */
static void ps2_delay_loops(uint32_t n)
{
    for (volatile uint32_t i = 0; i < n; i++) {
        __NOP();
    }
}

/* 一次完整通信：拉低片选 → 逐字节收发 → 拉高片选 → 间隔
 *
 * 时序要点（PS2 协议对时间敏感，太快接收板会来不及应答）：
 * 1. 片选拉低后要等一会儿再发第一个时钟；
 * 2. 字节之间也要留间隔，无线接收板才有时间准备下一字节；
 * 3. 两次通信之间至少间隔 1~2ms。
 */
static void ps2_transact(const uint8_t *tx, uint8_t *rx, uint8_t len)
{
    CS_LOW();
    ps2_delay_loops(200);

    for (uint8_t i = 0; i < len; i++) {
        rx[i] = ps2_xfer(tx[i]);
        ps2_delay_loops(100);
    }

    CS_HIGH();
    ps2_delay_loops(100);
    HAL_Delay(2);
}

/* 判断应答是否有效：收到合法 ID 字节，或收到固定应答字节 0x5A，都算通信正常 */
static uint8_t ps2_reply_valid(const uint8_t *rx)
{
    if (rx[1] == 0x41 || rx[1] == 0x73 || rx[1] == 0x79) {
        return 1;
    }
    if (rx[2] == 0x5A) {
        return 1;
    }
    return 0;
}

uint8_t PS2_Init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
    uint8_t rx[9];
    GPIO_InitTypeDef gpio = { 0 };

    s_hspi    = hspi;
    s_cs_port = cs_port;
    s_cs_pin  = cs_pin;

    /* 关键：F1 的 HAL_SPI_Init() 只配置外设，不会把 SPI 使能（SPE 仍为 0）。
       本驱动是直接读写寄存器的，必须自己打开 SPI，否则写进 DR 也不会产生
       时钟，读 RXNE 会一直等不到（表现为程序卡死、LED 不闪）。 */
    __HAL_SPI_ENABLE(hspi);

    /* 给 MISO 线（本设计中为 SPI1 的 PA6，接接收板 DAT）打开内部上拉。
       部分 PS2 接收板的 DAT 是开漏输出，没有上拉时永远读不到高电平，
       现象就是"发得出去、收不回来"，LED 一直闪。这一步不需要加电阻。 */
    gpio.Pin  = GPIO_PIN_6;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &gpio);

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
    /* ⑤ 反复尝试读取：无线接收板第一次应答可能较慢，给它几次机会 */
    s_prev_btns = 0;
    for (uint8_t attempt = 0; attempt < 5; attempt++) {
        const uint8_t tx[9] = { 0x01, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        ps2_transact(tx, rx, 9);
        if (ps2_reply_valid(rx)) {
            return 1;
        }
        HAL_Delay(100);
    }
    return 0;
}

void PS2_Read(PS2_t *ps2)
{
    const uint8_t tx[9] = { 0x01, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint8_t rx[9] = { 0 };

    ps2_transact(tx, rx, 9);

    ps2->type      = rx[1];
    ps2->connected = ps2_reply_valid(rx) ? 1 : 0;
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
