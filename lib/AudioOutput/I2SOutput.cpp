#include "I2SOutput.h"
#include <esp32-hal-log.h>
static const char* TAG = "I2SOutput";

I2SOutput::I2SOutput(const i2s_port_t i2sPort, const i2s_pin_config_t &i2s_pin_config):m_i2sPort(i2sPort), m_i2s_pin_config(i2s_pin_config)
{

}


// 开始
rowl_err_t I2SOutput:: start(const i2s_config_t *i2s_config){
  ESP_ERROR_CHECK(i2s_driver_install(m_i2sPort, i2s_config, 0, NULL));
  ESP_ERROR_CHECK(i2s_set_pin(m_i2sPort, &m_i2s_pin_config));
  return 0;
}


// 输出音频
rowl_err_t I2SOutput:: play(void *buf, size_t number){
  size_t bytes_write;
  i2s_write(m_i2sPort, buf, number, &bytes_write, portMAX_DELAY);
  if(bytes_write != number)
    ESP_LOGW(TAG, "I2S data did not all writed, just %d/%d", bytes_write, number);
  return 0;
}


// 停止
rowl_err_t I2SOutput:: stop(void){
  ESP_ERROR_CHECK(i2s_set_pin(m_i2sPort, &m_i2s_pin_unconfig));
  ESP_ERROR_CHECK(i2s_driver_uninstall(m_i2sPort));
  return 0;
}



I2SOutput::~I2SOutput()
{
}
