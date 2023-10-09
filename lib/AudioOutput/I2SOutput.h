#include "config.h"
#include "driver/i2s.h"
#include "driver/gpio.h"

typedef uint8_t rowl_err_t;

class I2SOutput
{
protected:
  i2s_port_t m_i2sPort = I2S_NUM_0; // The I2S`s port in esp32
  i2s_config_t m_i2s_config;        // This i2s configuration is set by the user. It must be configured before using the I2S module
  i2s_pin_config_t m_i2s_pin_config;
  size_t m_data_size; // one audio data in byte
  i2s_pin_config_t m_i2s_pin_unconfig = {
    .mck_io_num = -1,
    .bck_io_num = -1,
    .ws_io_num = -1,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
public:
  I2SOutput(const i2s_port_t i2sPort, const i2s_config_t &i2s_config, const i2s_pin_config_t &i2s_pin_config);
  rowl_err_t start(size_t data_size);
  rowl_err_t play(void *buf, size_t number);
  rowl_err_t stop(void);
  ~I2SOutput();
};

