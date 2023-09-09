#include "config.h"

// 驱动INMP441的I2S配置
const i2s_config_t i2s_INMP_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),//接收模式
    .sample_rate = SAMPLE_RATE,                 //采样率在头文件中宏定义
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,// 只能是24或32位
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,// leave L/R unconnected when using Left channel
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),// Philips standard | MSB first standard
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 64,  // 改成32count之后读取内存报错消失
    .dma_buf_len = 8,  // 1024 samples per buffer
    .use_apll = 1        // use APLL-CLK,frequency 16MHZ-128MHZ,it's for audio
};

// 驱动INMP441的I2S引脚
const i2s_pin_config_t i2s_INMP_pin_config = {
  .bck_io_num = MIC_INMP_I2S_BCK,
  .ws_io_num = MIC_INMP_I2S_WS,
  .data_out_num = MIC_INMP_I2S_DATA_OUT,
  .data_in_num = MIC_INMP_I2S_DATA_IN
};

