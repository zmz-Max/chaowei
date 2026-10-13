/* ==========================================================================
 * CRTC2026 超威战队 — PS2 手柄遥控底盘测试（STM32F103C8T6 + HAL + CLion）
 *
 * 前置：CubeMX 已按 firmware/README.md 配好 TIM2（四路 PWM）、PB12~PB15
 *       （方向）、SPI1（PA5/PA6/PA7）、PA4（片选，GPIO 输出，初始 High）。
 *
 * 用法：把下面 3 段粘进 CubeMX 生成的 main.c 对应 USER CODE 区。
 *       工程里需要把 ps2.c 加进 CMakeLists.txt 的源文件列表。
 * ========================================================================== */


/* ---------- ① 粘到 /* USER CODE BEGIN PV */ --------------------------------- */

static PS2_t ps2;

/* 单个电机：duty -100 ~ +100，负数反转 */
static void Motor_Set(uint32_t ch, uint16_t dir_pin, int duty)
{
    const uint32_t arr1 = 50;                 /* ARR(49) + 1 */

    if (duty >  100) duty =  100;
    if (duty < -100) duty = -100;

    if (duty >= 0) {
        HAL_GPIO_WritePin(GPIOB, dir_pin, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(&htim2, ch, arr1 * (uint32_t)duty / 100);
    } else {
        HAL_GPIO_WritePin(GPIOB, dir_pin, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(&htim2, ch, arr1 * (uint32_t)(100 + duty) / 100);
    }
}

/* 底盘：四个轮子分别给速度，左前/左后/右前/右后 */
static void Car_Set(int fl, int rl, int fr, int rr)
{
    Motor_Set(TIM_CHANNEL_1, GPIO_PIN_12, fl);
    Motor_Set(TIM_CHANNEL_2, GPIO_PIN_13, rl);
    Motor_Set(TIM_CHANNEL_3, GPIO_PIN_14, fr);
    Motor_Set(TIM_CHANNEL_4, GPIO_PIN_15, rr);
}


/* ---------- ② 粘到 /* USER CODE BEGIN 2 */（MX_xxx_Init() 之后） ------------ */

HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
Car_Set(0, 0, 0, 0);

if (PS2_Init(&hspi1, GPIOA, GPIO_PIN_4)) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);   /* PC13 LED 常亮 = 通信成功 */
} else {
    /* 通信失败：LED 快闪，程序停在这里方便你排查接线 */
    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(100);
    }
}


/* ---------- ③ 粘到 /* USER CODE BEGIN 3 */（while (1) 循环里） --------------- */

#define DUTY_FWD   60      /* 行进速度，可改 40~80 */
#define DUTY_TURN  55      /* 转向速度 */

PS2_Read(&ps2);

if (!ps2.connected) {
    /* 通信断了：立刻停车，LED 熄灭 */
    Car_Set(0, 0, 0, 0);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
} else {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

    if (ps2.btns & PSB_UP) {                 /* 十字键上：前进 */
        Car_Set( DUTY_FWD,  DUTY_FWD,  DUTY_FWD,  DUTY_FWD);
    } else if (ps2.btns & PSB_DOWN) {        /* 十字键下：后退 */
        Car_Set(-DUTY_FWD, -DUTY_FWD, -DUTY_FWD, -DUTY_FWD);
    } else if (ps2.btns & PSB_LEFT) {        /* 十字键左：原地左转 */
        Car_Set(-DUTY_TURN, -DUTY_TURN,  DUTY_TURN,  DUTY_TURN);
    } else if (ps2.btns & PSB_RIGHT) {       /* 十字键右：原地右转 */
        Car_Set( DUTY_TURN,  DUTY_TURN, -DUTY_TURN, -DUTY_TURN);
    } else if (ps2.btns & PSB_CROSS) {       /* × 键：横移（左前后退 + 右前前进） */
        Car_Set(-DUTY_FWD,  DUTY_FWD,  DUTY_FWD, -DUTY_FWD);
    } else {
        Car_Set(0, 0, 0, 0);                 /* 松手即停（滑行） */
    }
}

HAL_Delay(10);   /* 轮询周期约 10ms，跟手且不占用太多时间 */

/* 注意：横移那一组麦轮方向与代码里的假设有关，
   如果横移时车是斜着走或者原地打转，把对应两个电机取反即可。 */
