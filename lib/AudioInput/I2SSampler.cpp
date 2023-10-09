#include "Arduino.h"
#include "I2SSampler.h"
#include "esp_log.h"

static const char* TAG = "I2S";

I2SSampler::I2SSampler(i2s_port_t i2sPort, const i2s_config_t &i2s_config, const i2s_pin_config_t &i2s_pin_config)
    : m_i2sPort(i2sPort), m_i2s_config(i2s_config), m_i2s_pin_config(i2s_pin_config){
    }

void I2SSampler::start()
{
  esp_err_t err;
  // Install and start I2S driver
  err = i2s_driver_install(m_i2sPort, &m_i2s_config, 0, NULL);
  if (err != ESP_OK) {
    ESP_LOGI(TAG, "Failed installing driver: %d\n", err);
    while (true);
  }
  ESP_LOGI(TAG, "Success installed i2s driver");
  // Set up the I2S configuration from the subclass
  configureI2S();
}

void I2SSampler::stop()
{
  // Clear any I2S configuration
  unConfigureI2S();
  // Stop the I2S driver
  i2s_driver_uninstall(m_i2sPort);
}
