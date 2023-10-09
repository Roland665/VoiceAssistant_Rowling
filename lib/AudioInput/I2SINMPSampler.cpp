#include "Arduino.h"
#include "I2SINMPSampler.h"
#include "esp_log.h"

static const char* TAG = "I2S_INMP";




I2SINMPSampler::I2SINMPSampler(const i2s_port_t i2sPort, const i2s_config_t &i2s_config, const i2s_pin_config_t &i2s_pin_config)
    : I2SSampler(i2sPort, i2s_config, i2s_pin_config){}

void I2SINMPSampler::configureI2S(){
  esp_err_t err;
  err = i2s_set_pin(m_i2sPort, &m_i2s_pin_config);
  ESP_LOGI(TAG, "m_i2s_pin_config: .bck_io_num=%d, .data_out_num=%d", m_i2s_pin_config.bck_io_num, m_i2s_pin_config.data_out_num);
  if (err != ESP_OK) {
    ESP_LOGI(TAG, "Failed setting pin: %d\n", err);
    while (true);
  }
}

void I2SINMPSampler::unConfigureI2S(){

}

int I2SINMPSampler::read(i2s_INMP_sample_t *samples, int16_t count){
  size_t bytes_read;
  i2s_read(m_i2sPort, samples, sizeof(i2s_INMP_sample_t)*count, &bytes_read, portMAX_DELAY);
  return bytes_read/sizeof(i2s_INMP_sample_t);
}
