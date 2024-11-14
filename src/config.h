#pragma once

#include <Arduino.h>
#include <driver/i2s.h>
#include <hal/gpio_hal.h>
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

/*************/
//解释，本项目所有带 INMP 关键字的变量/常量都表示与INMP441麦克风模块相绑定

typedef uint8_t rowl_err_t;
#define ROWL_OK 0

// 整个系统的音频宏配置
#define SAMPLE_RATE 32000 // 采样率

// INMP441麦克风占用I2S通道
#define MIC_INMP_I2SPORT      I2S_NUM_0
// 驱动INMP441的I2S引脚占用
#define MIC_INMP_I2S_BCK      GPIO_NUM_1
#define MIC_INMP_I2S_WS       GPIO_NUM_2
#define MIC_INMP_I2S_DATA_OUT GPIO_NUM_NC//扬声器才要
#define MIC_INMP_I2S_DATA_IN  GPIO_NUM_3

// I2S功放占用I2S通道
#define PLAYER_I2SPORT      I2S_NUM_1
// I2S功放的I2S引脚占用
#define PLAYER_I2S_BCK      GPIO_NUM_6
#define PLAYER_I2S_WS       GPIO_NUM_7
#define PLAYER_I2S_DATA_OUT GPIO_NUM_8
#define PLAYER_I2S_DATA_IN  GPIO_NUM_NC // MIC 才要
// 驱动INMP441的I2S配置
const i2s_config_t i2s_INMP_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),// 接收模式
    .sample_rate = SAMPLE_RATE,                         // 采样率
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,       // 样本位深
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,        // leave L/R unconnected when using Left channel
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 64,                                // DMA缓冲区数量
    .dma_buf_len = 8,                                   // DMA缓冲区长度
    .use_apll = 1                                       // use APLL-CLK,frequency 16MHZ-128MHZ,it's for audio
};

// 驱动INMP441的I2S引脚
const i2s_pin_config_t i2s_INMP_pin_config = {
  .bck_io_num = MIC_INMP_I2S_BCK,
  .ws_io_num = MIC_INMP_I2S_WS,
  .data_out_num = MIC_INMP_I2S_DATA_OUT,
  .data_in_num = MIC_INMP_I2S_DATA_IN
};

// 驱动I2S扬声器模块的配置
i2s_config_t i2s_player_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),// 发送模式
    .sample_rate = 0,                         // 采样率
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,       // 样本位深
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,        // leave L/R unconnected when using Left channel
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 16,                                 // DMA缓冲区数量
    .dma_buf_len = 1024,                                // DMA缓冲区长度
    // .use_apll = 1                                       // use APLL-CLK,frequency 16MHZ-128MHZ,it's for audio
};

// 驱动INMP441的I2S引脚
const i2s_pin_config_t i2s_player_pin_config = {
  .bck_io_num = PLAYER_I2S_BCK,
  .ws_io_num = PLAYER_I2S_WS,
  .data_out_num = PLAYER_I2S_DATA_OUT,
  .data_in_num = PLAYER_I2S_DATA_IN
};



// OLED引脚占用
#define OLED_SCL GPIO_NUM_4
#define OLED_SDA GPIO_NUM_5

// 录音的启停按键
#define Record_Key GPIO_NUM_12

// 测试用LED相关配置
#define LEDA       GPIO_NUM_10 // IO占用
#define LEDA_OFF   digitalWrite(LEDA , LOW) // 关灯
#define LEDA_ON    digitalWrite(LEDA , HIGH) // 开灯
#define LEDA_PWM   digitalWrite(LEDA , !digitalRead(LEDA )) // 灯闪烁
#define LEDB       GPIO_NUM_11 // IO占用
#define LEDB_OFF   digitalWrite(LEDB , LOW) // 关灯
#define LEDB_ON    digitalWrite(LEDB , HIGH) // 开灯
#define LEDB_PWM   digitalWrite(LEDB , !digitalRead(LEDB )) // 灯闪烁

// MicroSD card fat文件挂载点
#define SD_MOUNT_POINT "/sd"
// MicroSD card SPI IO占用
#define SD_SPI_CS     GPIO_NUM_15
#define SD_SPI_MISO   GPIO_NUM_16
#define SD_SPI_MOSI   GPIO_NUM_17
#define SD_SPI_SCLK   GPIO_NUM_18

// MicroSD card SPI 总线配置信息
spi_bus_config_t sd_spi_bus_config = {
    .mosi_io_num = SD_SPI_MOSI,
    .miso_io_num = SD_SPI_MISO,
    .sclk_io_num = SD_SPI_SCLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 4000,
};

