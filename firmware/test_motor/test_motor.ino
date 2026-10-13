/* ==========================================================================
 * CRTC2026 超威战队 — 四路电机接线测试（Arduino IDE + STM32duino）
 *
 * 用法：Arduino IDE 里选择 STM32F103C8（BluePill）开发板，
 *       下载方式选 ST-Link，直接上传本文件。
 *
 * 硬件前提：两片驱动板的 STBY 都接了 3.3V，VM 接 7.4V 母线，GND 共地。
 * ========================================================================== */

/* 编号约定：M1 左前、M2 左后、M3 右前、M4 右后 */
struct Motor {
  int pwmPin;   /* 调速脚（PWM） */
  int dirPin;   /* 方向脚（普通 GPIO） */
};

Motor motors[4] = {
  { PA0, PB12 },   /* M1 左前：驱动板 #1 A 通道 */
  { PA1, PB13 },   /* M2 左后：驱动板 #1 B 通道 */
  { PA2, PB14 },   /* M3 右前：驱动板 #2 A 通道 */
  { PA3, PB15 },   /* M4 右后：驱动板 #2 B 通道 */
};

/**
 * 设置单个电机
 * duty: -100 ~ +100，负数反转，0 停转
 *
 * 反转时方向脚拉高，电机两端电压变成 (d - 1)，
 * 所以占空比要写成 (255 - |duty| 对应的值)，转速才与 |duty| 成正比。
 */
void motorSet(const Motor &m, int duty) {
  duty = constrain(duty, -100, 100);
  if (duty >= 0) {
    digitalWrite(m.dirPin, LOW);
    analogWrite(m.pwmPin, duty * 255 / 100);
  } else {
    digitalWrite(m.dirPin, HIGH);
    analogWrite(m.pwmPin, 255 - (255 * (-duty) / 100));
  }
}

void setup() {
  for (int i = 0; i < 4; i++) {
    pinMode(motors[i].pwmPin, OUTPUT);
    pinMode(motors[i].dirPin, OUTPUT);
    digitalWrite(motors[i].dirPin, LOW);
    analogWrite(motors[i].pwmPin, 0);
  }
  delay(500);
}

void loop() {
  /* 第一轮：逐个电机 正转 1s → 停 0.5s → 反转 1s → 停 0.5s */
  for (int i = 0; i < 4; i++) {
    motorSet(motors[i],  60);  delay(1000);
    motorSet(motors[i],   0);  delay(500);
    motorSet(motors[i], -60);  delay(1000);
    motorSet(motors[i],   0);  delay(500);
  }

  /* 第二轮：四个一起正转 2s，再一起停 2s */
  for (int i = 0; i < 4; i++) motorSet(motors[i], 60);
  delay(2000);
  for (int i = 0; i < 4; i++) motorSet(motors[i], 0);
  delay(2000);
}
