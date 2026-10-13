/* ==========================================================================
 * CRTC2026 超威战队 — 四路电机接线测试（STM32F103C8T6 + DRV8833 ×2，HAL 库）
 *
 * 用法：把下面 4 段代码分别粘进 STM32CubeMX 生成的 main.c 中对应标记处，
 *       编译烧录即可。不需要改动其它文件。
 *
 * 硬件前提：TIM2 的 CH1~CH4 已在 CubeMX 中配置为 PWM（PSC=71 / ARR=49），
 *           PB12~PB15 已配置为推挽输出；两片驱动板的 STBY 都接了 3.3V。
 * ========================================================================== */


/* ---------- ① 粘到 /* USER CODE BEGIN PV */ --------------------------------- */

typedef struct {
    uint32_t      ch;      /* TIM 通道 */
    GPIO_TypeDef *port;    /* 方向脚所在端口 */
    uint16_t      pin;     /* 方向脚 */
} Motor_t;

static Motor_t motors[4] = {
    { TIM_CHANNEL_1, GPIOB, GPIO_PIN_12 },   /* M1 左前：驱动板 #1 A 通道 */
    { TIM_CHANNEL_2, GPIOB, GPIO_PIN_13 },   /* M2 左后：驱动板 #1 B 通道 */
    { TIM_CHANNEL_3, GPIOB, GPIO_PIN_14 },   /* M3 右前：驱动板 #2 A 通道 */
    { TIM_CHANNEL_4, GPIOB, GPIO_PIN_15 },   /* M4 右后：驱动板 #2 B 通道 */
};


/* ---------- ② 粘到 /* USER CODE BEGIN 0 */ ---------------------------------- */

/**
 * @brief  设置单个电机
 * @param  ch        TIM 通道（TIM_CHANNEL_1 ~ 4）
 * @param  dir_port  方向脚端口（GPIOB）
 * @param  dir_pin   方向脚（GPIO_PIN_12 ~ 15）
 * @param  duty      -100 ~ +100，负数反转，0 停转（滑行）
 *
 * 说明：驱动板一个通道只有“一个调速脚 + 一个方向脚”。
 *       正转：方向脚 = 0，调速脚输出占空比 d，转速正比于 d。
 *       反转：方向脚 = 1，此时电机两端电压为 (d - 1)，
 *             所以要让占空比写成 (100 - |duty|)，转速才正比于 |duty|。
 *       两根同时为 1 是刹车，本函数不用，需要急停时直接写 GPIO 即可。
 */
static void Motor_Set(uint32_t ch, GPIO_TypeDef *dir_port, uint16_t dir_pin, int duty)
{
    const uint32_t arr1 = 50;              /* ARR(49) + 1 */

    if (duty >  100) duty =  100;
    if (duty < -100) duty = -100;

    if (duty >= 0) {
        HAL_GPIO_WritePin(dir_port, dir_pin, GPIO_PIN_RESET);   /* IN2 = 0 */
        __HAL_TIM_SET_COMPARE(&htim2, ch, arr1 * (uint32_t)duty / 100);
    } else {
        HAL_GPIO_WritePin(dir_port, dir_pin, GPIO_PIN_SET);     /* IN2 = 1 */
        __HAL_TIM_SET_COMPARE(&htim2, ch, arr1 * (uint32_t)(100 + duty) / 100);
    }
}


/* ---------- ③ 粘到 /* USER CODE BEGIN 2 */（所有 MX_xxx_Init() 之后） -------- */

HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);


/* ---------- ④ 粘到 /* USER CODE BEGIN 3 */（while (1) 循环体里面） ---------- */

/* 第一轮：逐个电机 正转 1s → 停 0.5s → 反转 1s → 停 0.5s */
for (int i = 0; i < 4; i++) {
    Motor_Set(motors[i].ch, motors[i].port, motors[i].pin,  60);   HAL_Delay(1000);
    Motor_Set(motors[i].ch, motors[i].port, motors[i].pin,   0);   HAL_Delay(500);
    Motor_Set(motors[i].ch, motors[i].port, motors[i].pin, -60);   HAL_Delay(1000);
    Motor_Set(motors[i].ch, motors[i].port, motors[i].pin,   0);   HAL_Delay(500);
}

/* 第二轮：四个一起正转 2s，再一起停 2s */
for (int i = 0; i < 4; i++) {
    Motor_Set(motors[i].ch, motors[i].port, motors[i].pin, 60);
}
HAL_Delay(2000);
for (int i = 0; i < 4; i++) {
    Motor_Set(motors[i].ch, motors[i].port, motors[i].pin, 0);
}
HAL_Delay(2000);

/* 程序会一直循环上面两轮，方便观察；调试完把这段换成底盘控制逻辑 */
