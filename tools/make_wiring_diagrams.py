# -*- coding: utf-8 -*-
"""生成 docs/assets 下的接线示意图（供电链路、信号接线总图）。

用法: python tools/make_wiring_diagrams.py [输出目录]
依赖: Pillow, 微软雅黑字体 (C:/Windows/Fonts/msyh.ttc)
"""
import math
import os
import sys

from PIL import Image, ImageDraw, ImageFont

OUT_DIR = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "docs", "assets")
MSYH = "C:/Windows/Fonts/msyh.ttc"
MSYHBD = "C:/Windows/Fonts/msyhbd.ttc"

RED = (198, 32, 32)
ORG = (214, 126, 12)
BLU = (32, 88, 190)
GRN = (26, 128, 78)
PUR = (140, 70, 180)
GRY = (88, 88, 88)
INK = (38, 38, 38)
SUB = (96, 96, 96)

_fc = {}


def F(size, bold=False):
    key = (size, bold)
    if key not in _fc:
        _fc[key] = ImageFont.truetype(MSYHBD if bold else MSYH, size)
    return _fc[key]


def box(d, x, y, w, h, title, lines=(), fill=(255, 255, 255), outline=GRY,
        ts=30, bs=21, radius=18, lh=None):
    d.rounded_rectangle([x, y, x + w, y + h], radius=radius, fill=fill,
                        outline=outline, width=3)
    cx = x + w / 2
    d.text((cx, y + 16 + ts * 0.55), title, font=F(ts, True), fill=outline, anchor="mm")
    step = lh if lh else bs + 12
    top = y + 16 + ts * 1.25
    for i, ln in enumerate(lines):
        d.text((cx, top + i * step + bs * 0.6), ln, font=F(bs), fill=INK, anchor="mm")


def arrow(d, p1, p2, color, width=6, head=18):
    d.line([p1, p2], fill=color, width=width)
    ang = math.atan2(p2[1] - p1[1], p2[0] - p1[0])
    for s in (-1, 1):
        a = ang + s * math.radians(24)
        d.line([p2, (p2[0] - head * math.cos(a), p2[1] - head * math.sin(a))],
               fill=color, width=width)


def label(d, x, y, text, color, size=20):
    f = F(size, True)
    bb = d.textbbox((x, y), text, font=f, anchor="mm")
    d.rounded_rectangle([bb[0] - 9, bb[1] - 5, bb[2] + 9, bb[3] + 5],
                        radius=9, fill=(255, 255, 255))
    d.text((x, y), text, font=f, fill=color, anchor="mm")


def title_block(d, w, main, sub):
    d.text((w / 2, 52), main, font=F(46, True), fill=INK, anchor="mm")
    d.text((w / 2, 104), sub, font=F(23), fill=SUB, anchor="mm")


def build_power():
    W, H = 1800, 1090
    img = Image.new("RGB", (W, H), (255, 255, 255))
    d = ImageDraw.Draw(img)
    title_block(d, W, "CRTC2026 小车供电链路",
                "每台机器人只允许 1 个供电模块（额定电压 ≤ 7.4V）· 电池正极串入无线继电器，"
                "遥控可在 10m 内切断底盘电源")

    box(d, 60, 180, 300, 136, "18650 电池盒 ×2 节",
        ["2S = 7.4V 标称，8.4V 满电", "选动力型电芯（≥10A 放电）"],
        fill=(255, 236, 236), outline=RED, ts=27, bs=20)
    arrow(d, (368, 248), (424, 248), RED, width=9)
    label(d, 396, 214, "7.4V", RED, 19)

    box(d, 430, 170, 390, 156, "无线继电器接收器",
        ["DC5-24V：− / + 接电池（触点之前）", "COM + NC 串进 7.4V 母线正极", "按遥控 = 断电，再按 = 恢复"],
        fill=(255, 246, 230), outline=ORG, ts=27, bs=20)
    arrow(d, (828, 248), (874, 248), RED, width=9)

    box(d, 880, 180, 320, 136, "7.4V 主电源母线",
        ["焊接汇流排 / 面包板两条", "正极 + 负极（GND）"],
        fill=(242, 242, 244), outline=GRY, ts=27, bs=20)

    d.line([(1040, 316), (1040, 392)], fill=RED, width=9)
    d.line([(330, 392), (1480, 392)], fill=RED, width=9)
    label(d, 1040, 356, "7.4V 母线", RED, 19)
    for x in (330, 900, 1480):
        arrow(d, (x, 392), (x, 428), RED if x == 330 else ORG, width=7)

    box(d, 120, 428, 420, 120, "DRV8833 ×2 电机驱动",
        ["VM ← 7.4V（粗线，带电容）", "信号 / STBY ← 3.3V"],
        fill=(236, 242, 255), outline=BLU, ts=25, bs=20)
    arrow(d, (330, 556), (330, 604), RED, width=7)
    label(d, 330, 580, "7.4V", RED, 18)
    box(d, 120, 604, 420, 128, "TT 马达 ×4（麦轮）",
        ["每片 DRV8833 驱动 2 个马达", "堵转电流大，线要焊死"],
        fill=(236, 242, 255), outline=BLU, ts=25, bs=20)

    box(d, 690, 428, 420, 120, "DC-DC 降压 #1 → 5V",
        ["IN+ / IN− ← 7.4V 母线", "OUT+ / OUT− → 5V 逻辑供电"],
        fill=(255, 246, 230), outline=ORG, ts=25, bs=20)
    arrow(d, (900, 556), (900, 604), ORG, width=7)
    label(d, 900, 580, "5V", ORG, 18)
    box(d, 690, 604, 420, 128, "STM32 + PS2 接收器",
        ["STM32 由 5V 引脚供电", "PS2 接收器用板载 3.3V"],
        fill=(232, 246, 238), outline=GRN, ts=25, bs=20)

    box(d, 1260, 428, 440, 120, "DC-DC 降压 #2 → 5V",
        ["IN+ / IN− ← 7.4V 母线", "OUT+ / OUT− → 5V 舵机专用"],
        fill=(255, 246, 230), outline=ORG, ts=25, bs=20)
    arrow(d, (1480, 556), (1480, 604), ORG, width=7)
    label(d, 1480, 580, "5V", ORG, 18)
    box(d, 1260, 604, 440, 128, "舵机 ×6 独立 5V",
        ["MG995 ×2 + SG90 ×4", "不可从 STM32 板取电"],
        fill=(250, 240, 255), outline=PUR, ts=25, bs=20)

    box(d, 60, 790, 1680, 150, "接线要点",
        ["共地：所有模块 GND 必须汇到电池负极，单点星形接地（大电流回路与信号线分开走线）",
         "滤波：DRV8833 的 VM-GND 之间并联 470µF/16V 电解电容 + 0.1µF 陶瓷电容，防止电机启动把单片机拉复位",
         "保护：电池正极串 2A 保险丝；电源线与电机线用粗线并焊死 · 降压模块 3A(max)/1.6~1.8A(正常)，6 个舵机不要同时堵转"],
        fill=(248, 248, 250), outline=GRY, ts=27, bs=21, lh=34)

    img.save(os.path.join(OUT_DIR, "wiring_power.png"))


def build_signal():
    W, H = 1800, 1210
    img = Image.new("RGB", (W, H), (255, 255, 255))
    d = ImageDraw.Draw(img)
    title_block(d, W, "CRTC2026 信号接线总图 · STM32F103C8T6 引脚分配",
                "所有信号按 3.3V 电平处理 · 下载调试用 ST-Link V2（SWD：PA13 / PA14）")

    box(d, 40, 140, 570, 232, "留空 / 保留引脚",
        ["PA9 / PA10：USART1 串口调试（预留）",
         "PA13 / PA14：SWD 下载调试，接 ST-Link V2",
         "PA11 / PA12：USB，本方案不用",
         "PB2：BOOT1，不要接 · PC13：板载 LED"],
        fill=(246, 246, 248), outline=GRY, ts=25, bs=20, lh=42)

    box(d, 660, 140, 560, 170, "PS2 无线手柄接收器",
        ["SPI1 接口（直插排针转接板）", "VCC 接 3.3V，不要接 5V"],
        fill=(255, 244, 232), outline=ORG, ts=27, bs=21)
    arrow(d, (940, 310), (940, 468), BLU, width=6)
    d.text((962, 360), "PA5 → SCK ｜ PA6 → DATA(MISO)", font=F(21), fill=BLU)
    d.text((962, 396), "PA7 → CMD(MOSI) ｜ PA4 → ATT(CS) ｜ 3.3V / GND",
           font=F(21), fill=BLU)

    box(d, 620, 468, 560, 400, "STM32F103C8T6 核心板",
        ["TIM2 四路 PWM：PA0 / PA1 / PA2 / PA3",
         "方向 GPIO：PB12 / PB13 / PB14 / PB15",
         "SPI1：PA4(CS) PA5(SCK) PA6(MISO) PA7(MOSI)",
         "舵机 PWM：PB0 / PB1（TIM3_CH3 / CH4）",
         "舵机 PWM：PB6 / PB7 / PB8 / PB9（TIM4_CH1~4）",
         "5V ← 降压 #1 ｜ 3.3V 供 PS2 与驱动板 STBY"],
        fill=(232, 246, 238), outline=GRN, ts=28, bs=21, lh=52)

    box(d, 40, 520, 520, 200, "DRV8833 #1（左侧两轮）",
        ["AIN1 ← PA0（PWM）  AIN2 ← PB12（方向）",
         "BIN1 ← PA1（PWM）  BIN2 ← PB13（方向）",
         "AO1/AO2 → 左前马达  BO1/BO2 → 左后马达",
         "STBY ← 3.3V（必须为高）｜ VM ← 7.4V ｜ GND"],
        fill=(236, 242, 255), outline=BLU, ts=26, bs=20, lh=40)
    arrow(d, (560, 620), (620, 620), BLU, width=6)

    box(d, 1240, 520, 520, 200, "DRV8833 #2（右侧两轮）",
        ["AIN1 ← PA2（PWM）  AIN2 ← PB14（方向）",
         "BIN1 ← PA3（PWM）  BIN2 ← PB15（方向）",
         "AO1/AO2 → 右前马达  BO1/BO2 → 右后马达",
         "STBY ← 3.3V（必须为高）｜ VM ← 7.4V ｜ GND"],
        fill=(236, 242, 255), outline=BLU, ts=26, bs=20, lh=40)
    arrow(d, (1180, 620), (1240, 620), BLU, width=6)

    box(d, 40, 780, 560, 180, "两个最容易错的地方",
        ["麦轮辊子：左前与右后同向，右前与左后同向",
         "杜邦线会松：电源与电机线焊死，插头点热熔胶"],
        fill=(255, 246, 230), outline=ORG, ts=25, bs=20, lh=40)

    box(d, 1240, 780, 560, 180, "上电检查顺序",
        ["① 万用表测正负极是否短路",
         "② 只接 ST-Link，先确认能下载程序",
         "③ 上电池：先测母线 8.4V → 5V 输出",
         "④ 最后接电机、舵机"],
        fill=(246, 246, 248), outline=GRY, ts=25, bs=20, lh=32)

    arrow(d, (900, 868), (900, 964), BLU, width=6)
    box(d, 310, 964, 900, 200, "舵机 ×6（50Hz PWM，0.5~2.5ms）",
        ["MG995 ×2 → PB0（TIM3_CH3）/ PB1（TIM3_CH4）",
         "SG90 ×4 → PB6 / PB7 / PB8 / PB9（TIM4_CH1~CH4）",
         "舵机 V+ 走降压 #2 的 5V，GND 与 STM32 共地，信号线直连"],
        fill=(250, 240, 255), outline=PUR, ts=27, bs=21, lh=42)

    img.save(os.path.join(OUT_DIR, "wiring_signal.png"))


if __name__ == "__main__":
    os.makedirs(OUT_DIR, exist_ok=True)
    build_power()
    build_signal()
    print("saved to", OUT_DIR)
