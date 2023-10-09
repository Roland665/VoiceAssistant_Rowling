#pragma once

#include <driver/i2s.h>

/**
 * Base Class for both the ADC or I2S-standard sampler
 **/
class I2SSampler
{
protected:
    i2s_port_t m_i2sPort = I2S_NUM_0; // The I2S`s port in esp32
    i2s_config_t m_i2s_config;        // This i2s configuration is set by the user. It must be configured before using the I2S module
    i2s_pin_config_t m_i2s_pin_config;
    virtual void configureI2S() = 0;
    virtual void unConfigureI2S() = 0;
    // virtual void processI2SData(void *samples, size_t count){
    //     // nothing to do for the default case
    // };

public:
    I2SSampler(i2s_port_t i2sPort, const i2s_config_t &i2sConfig, const i2s_pin_config_t &i2s_pin_config);
    void start();
    int read(); // Should be implemented in a subclass
    void stop();
    int sample_rate()
    {
        return m_i2s_config.sample_rate;
    }
};
