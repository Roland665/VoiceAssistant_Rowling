#include "WavFile.h"
#include "sd_card.h"
#include "driver/i2s.h" // esp32 库

/***************************************
 * 注意：本类
*/
class WavFileReader
{
private:
  sd_card *m_sdcard;
  uint16_t m_bitDepth; // 音频位深
  uint32_t m_samlpeRate; // 采样率
  uint16_t m_channelsNum; // 通道数
  size_t m_bufferLength; // 自定缓冲区的长度
  wav_header_t *m_wav_header = NULL;
  wav_header_with_list_t *m_wav_header_with_list = NULL;
public:

  WavFileReader(sd_card *_sdcard);
  rowl_err_t check_format(const char *path);
  uint8_t *get_Audiobuffer(size_t length);
  size_t get_subchunk2_size(void);
  rowl_err_t set_esp_i2s_config(i2s_config_t *i2s_config);
  rowl_err_t start(const char *path);
  rowl_err_t read(void *buf);
  rowl_err_t read(void *buf, size_t number);
  rowl_err_t read(void *buf, size_t number, size_t *count);
  rowl_err_t read(void *buf, size_t data_size, size_t number, size_t *count);
  rowl_err_t readAll(void *buf, size_t data_size);
  rowl_err_t stop(void);
  ~WavFileReader();
};
