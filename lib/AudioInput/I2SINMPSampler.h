#pragma once

#include "I2SSampler.h"

typedef int16_t i2s_INMP_sample_t;
/**
 * I2S Class for both the ADC or I2S-standard sampler
 **/
class I2SINMPSampler : public I2SSampler
{
protected:
  i2s_pin_config_t m_i2s_pin_config;

public:
  I2SINMPSampler(i2s_port_t i2sPort, const i2s_config_t &i2s_config, const i2s_pin_config_t &i2s_pin_config);
  void configureI2S();
  void unConfigureI2S();
  int read(i2s_INMP_sample_t *samples, int16_t count);
  int sample_rate()
  {
    return m_i2s_config.sample_rate;
  }
};
