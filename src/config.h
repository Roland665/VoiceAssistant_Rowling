#pragma once

#include <driver/i2s.h>
#include <hal/gpio_hal.h>
/*************/
//解释，本项目所有带 INMP 关键字的变量/常量都表示与INMP441麦克风模块相绑定

// 整个系统的音频宏配置
#define SAMPLE_RATE 16000 // 采样率

// INMP441麦克风占用I2S通道
#define MIC_INMP_I2SPORT      I2S_NUM_0
// 驱动INMP441的I2S引脚占用
#define MIC_INMP_I2S_BCK      GPIO_NUM_1
#define MIC_INMP_I2S_WS       GPIO_NUM_2
#define MIC_INMP_I2S_DATA_OUT GPIO_NUM_NC//扬声器才要
#define MIC_INMP_I2S_DATA_IN  GPIO_NUM_3

extern const i2s_config_t i2s_INMP_config;
extern const i2s_pin_config_t i2s_INMP_pin_config;

// OLED引脚占用
#define OLED_SCL GPIO_NUM_4
#define OLED_SDA GPIO_NUM_5

// 录音的启停按键
#define Record_Key GPIO_NUM_12

// 旋转编码器外部中断
#define EC11A  GPIO_NUM_14
#define EC11B  GPIO_NUM_13
// 测试用LED相关配置
#define LEDA       GPIO_NUM_10 // IO占用
#define LEDA_OFF   digitalWrite(LEDA , LOW) // 关灯
#define LEDA_ON    digitalWrite(LEDA , HIGH) // 开灯
#define LEDA_PWM   digitalWrite(LEDA , !digitalRead(LEDA )) // 灯闪烁
#define LEDB       GPIO_NUM_11 // IO占用
#define LEDB_OFF   digitalWrite(LEDB , LOW) // 关灯
#define LEDB_ON    digitalWrite(LEDB , HIGH) // 开灯
#define LEDB_PWM   digitalWrite(LEDB , !digitalRead(LEDB )) // 灯闪烁

// MicroSD card SPI IO占用
#define SD_SPI_CS     GPIO_NUM_15
#define SD_SPI_MISO   GPIO_NUM_16
#define SD_SPI_MOSI   GPIO_NUM_17
#define SD_SPI_SCLK   GPIO_NUM_18


