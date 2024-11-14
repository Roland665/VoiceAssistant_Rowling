#include "WavFileReader.h"
static const char *TAG = "Wav_Reader";

WavFileReader::WavFileReader(sd_card *_sdcard) : m_sdcard(_sdcard)
{
}



/**
 * @brief    检查文件是否为wave文件
 * @param    path : 文件名，可以以“/”开头，也可以单单只写文件名称
 * @retval   0-是wave文件，1-文件名称不符，2-文件不存在
 */
rowl_err_t WavFileReader::check_format(const char *path)
{
  ESP_LOGI(TAG, "Checking wave file:%s", path);

  // 检查文件头
  if (m_sdcard->start(path, "r"))
    return 2;
  m_wav_header = (wav_header_t *)malloc(sizeof(wav_header_t));
  m_sdcard->read(m_wav_header, sizeof(wav_header_t), 1);
  m_sdcard->stop();
  if (m_wav_header->chunk_ID[0] != 'R' || m_wav_header->chunk_ID[1] != 'I' || m_wav_header->chunk_ID[2] != 'F' || m_wav_header->chunk_ID[3] != 'F')
  {
    // 不符RIFF规范
    free(m_wav_header);
    m_wav_header = NULL;
    ESP_LOGE(TAG, "不符RIFF规范");
    return 1;
  }
  if (m_wav_header->format[0] != 'W' || m_wav_header->format[1] != 'A' || m_wav_header->format[2] != 'V' || m_wav_header->format[3] != 'E')
  {
    // 不符WAVE规范
    free(m_wav_header);
    m_wav_header = NULL;
    ESP_LOGE(TAG, "不符WAVE规范");
    return 1;
  }
  if (m_wav_header->subchunk1_ID[0] != 'f' || m_wav_header->subchunk1_ID[1] != 'm' || m_wav_header->subchunk1_ID[2] != 't' || m_wav_header->subchunk1_ID[3] != ' ')
  {
    // 不符fmt规范
    free(m_wav_header);
    m_wav_header = NULL;
    ESP_LOGE(TAG, "不符fmt规范");
    return 1;
  }
  if (m_wav_header->subchunk2_ID[0] == 'd' && m_wav_header->subchunk2_ID[1] == 'a' && m_wav_header->subchunk2_ID[2] == 't' && m_wav_header->subchunk2_ID[3] == 'a')
  {
    // 无LIST块，有data块
    m_bitDepth = m_wav_header->bits_per_sample/8;
    m_samlpeRate = m_wav_header->sample_rate;
    m_channelsNum = m_wav_header->num_channels;
    ESP_LOGI(TAG, "Checked wave file sucessfully, bits_per_sample is %d, sample_rate is %d", m_wav_header->bits_per_sample, m_wav_header->sample_rate);
  }
  else if((m_wav_header->subchunk2_ID[0] == 'L' && m_wav_header->subchunk2_ID[1] == 'I' && m_wav_header->subchunk2_ID[2] == 'S' && m_wav_header->subchunk2_ID[3] == 'T')){
    // 有LIST块，重新获取Wave头
    free(m_wav_header);
    m_wav_header = NULL;
    m_wav_header_with_list = (wav_header_with_list_t *)malloc(sizeof(wav_header_with_list_t));
    m_sdcard->start(path, "r");
    m_sdcard->read(m_wav_header_with_list, sizeof(wav_header_t), 1);
    m_wav_header_with_list->subchunk2_data = (char *)malloc(sizeof(char) * m_wav_header_with_list->subchunk2_size);
    // 读取List块内数据
    m_sdcard->read(m_wav_header_with_list->subchunk2_data, sizeof(char), m_wav_header_with_list->subchunk2_size);
    // 读取wave头内剩余数据(8 Byte)
    m_sdcard->read(m_wav_header_with_list->subchunk3_ID, sizeof(uint8_t), 4);
    m_sdcard->read(&m_wav_header_with_list->subchunk3_size, sizeof(int), 1);
    m_sdcard->stop();
    if(m_wav_header_with_list->subchunk3_ID[0] != 'd' || m_wav_header_with_list->subchunk3_ID[1] != 'a' || m_wav_header_with_list->subchunk3_ID[2] != 't' || m_wav_header_with_list->subchunk3_ID[3] != 'a')
    {
      // 没有data块
      free(m_wav_header_with_list->subchunk2_data);
      free(m_wav_header_with_list);
      m_wav_header_with_list = NULL;
      ESP_LOGE(TAG, "没有data块 subchunk3_ID is %c %c %c %c", m_wav_header_with_list->subchunk3_ID[0], m_wav_header_with_list->subchunk3_ID[1], m_wav_header_with_list->subchunk3_ID[2], m_wav_header_with_list->subchunk3_ID[3]);
      return 1;
    }
    m_bitDepth = m_wav_header_with_list->bits_per_sample/8;
    m_samlpeRate = m_wav_header_with_list->sample_rate;
    m_channelsNum = m_wav_header_with_list->num_channels;
    ESP_LOGI(TAG, "Checked wave with list file sucessfully, bits_per_sample is %d, sample_rate is %d", m_wav_header_with_list->bits_per_sample, m_wav_header_with_list->sample_rate);

  }
  else{
    // 无List块，也无data块
    free(m_wav_header_with_list);
    m_wav_header_with_list = NULL;
    ESP_LOGE(TAG, "无List块，也无data块");
    return 1;
  }

  return 0;
}



/**
 * @brief    读取音频数据前请执行此方法，即打开指定wave文件
 * @param    path : 文件名，可以以“/”开头，也可以单单只写文件名称
 * @retval   0-成功打开wave文件，1-文件内格式错误，2-文件不存在
 */
rowl_err_t WavFileReader::start(const char *path)
{
  // 先判断文件内容是否符合wave文件要求，并获取到 wave 文件的相关数据
  switch (check_format(path))
  {
  case 1:
    ESP_LOGE(TAG, "Error wave file format!");
    return 1;
  case 2:
    ESP_LOGE(TAG, "Cannot found %s", path);
    return 2;
  }
  ESP_LOGI(TAG, "Start reading wave file in %s", path);

  // 打开文件
  m_sdcard->start(path, "r");
  // 文件指针指向data正文第一个字节
  if(m_wav_header)
    m_sdcard->seek(sizeof(wav_header_t), SEEK_SET);
  else
    m_sdcard->seek(sizeof(wav_header_with_list_t) + m_wav_header_with_list->subchunk2_size - sizeof(char *), SEEK_SET);

  return 0;
}



/**
 * @brief    读取wave文件数据，读出数据的单数据大小等于音频位深，数据长度等于缓冲区长度，调用此方法前必须执行 start 方法并且第二个传参不能是 "w"、"a"
 * @param    buf       : 读出数据存放缓冲
 * @retval   0-成功读取， 1-未读出指定数据量
 */
rowl_err_t WavFileReader::read(void *buf)
{
  return m_sdcard->read(buf, 1, m_bufferLength);
}



/**
 * @brief    读取wave文件数据，读出数据的单数据大小等于音频位深，调用此方法前必须执行 start 方法并且第二个传参不能是 "w"、"a"
 * @param    buf       : 读出数据存放缓冲
 * @param    number    : 目标读出数据量(单位音频位深)
 * @retval   0-成功读取， 1-未读出指定数据量
 */
rowl_err_t WavFileReader::read(void *buf, size_t number)
{
  return m_sdcard->read(buf, m_bitDepth, number);
}



/**
 * @brief    读取wave文件数据，读出数据的单数据大小等于音频位深，调用此方法前必须执行 start 方法并且第二个传参不能是 "w"、"a"
 * @param    buf       : 读出数据存放缓冲
 * @param    number    : 目标读出数据量(单位音频位深)
 * @param    count     : 实际读出数据量(单位Byte)
 * @retval   0-成功读取， 1-未读出指定数据量
 */
rowl_err_t WavFileReader::read(void *buf, size_t number, size_t *count)
{
  size_t data_size;
  if(m_wav_header)
    data_size = m_wav_header->bits_per_sample/8;
  else
    data_size = m_wav_header_with_list->bits_per_sample/8;
  return m_sdcard->read(buf, sizeof(uint8_t), number*data_size, count);
}



/**
 * @brief    读取wave文件所有数据，调用此方法前必须执行 start 方法并且第二个传参不能是 "w"、"a"
 * @param    buf       : 读出数据存放缓冲
 * @param    data_size : 读出数据的单数据大小(单位byte)
 * @param    number    : 目标读出数据量
 * @param    count     : 实际读出数据量
 * @retval   0-成功读取， 1-未读出指定数据量
 */
rowl_err_t WavFileReader::read(void *buf, size_t data_size, size_t number, size_t *count)
{
  return m_sdcard->read(buf, data_size, number, count);
}



/**
 * @brief    读取wave文件所有正文数据，调用此方法前必须执行 start 方法并且第二个传参不能是 "w"、"a"
 * @param    buf       : 读出数据存放缓冲
 * @param    data_size : 读出数据的单数据大小(单位byte)
 * @retval   0-成功读取， 1-未读出指定数据量
 */
rowl_err_t WavFileReader::readAll(void *buf, size_t data_size)
{
  size_t count;
  return m_sdcard->read(buf, data_size, m_wav_header->subchunk2_size, &count);
}



/**
 * @brief    结束本次wave文件读取
 * @param    void
 * @retval   0-成功结束
 */
rowl_err_t WavFileReader::stop(void)
{
  if(m_wav_header){
    free(m_wav_header);
    m_wav_header = NULL;
  }
  else{
    free(m_wav_header_with_list->subchunk2_data);
    free(m_wav_header_with_list);
    m_wav_header_with_list = NULL;
  }
  return m_sdcard->stop();
}



/**
 * @brief    自动设置 esp32 的 I2S 驱动配置
 * @param    void
 * @retval   0-成功结束
 */
rowl_err_t WavFileReader::set_esp_i2s_config(i2s_config_t *i2s_config)
{
  i2s_config->mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),// 发送模式
  i2s_config->sample_rate = m_samlpeRate;
  i2s_config->bits_per_sample = (i2s_bits_per_sample_t)(m_bitDepth*8);
  if(m_channelsNum == 1)
    // 如果是单声道
    i2s_config->channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  else if(m_channelsNum == 2)
    // 如果是双声道
    i2s_config->channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  else
    // 多声道
    i2s_config->channel_format = I2S_CHANNEL_FMT_MULTIPLE;
  return 0;
}



/**
 * @brief    获取wave音频缓冲区，缓冲区是uint8_t类型，长度为位深*传参大小，调用此方法前必须执行 start 方法
 * @param    length : 缓冲区长度，单位是位深
 * @retval   缓冲区指针
 */
uint8_t *WavFileReader::get_Audiobuffer(size_t length)
{
  m_bufferLength = m_bitDepth * length;
  uint8_t *buffer = (uint8_t *)malloc(m_bufferLength);
  return buffer;
}



/**
 * @brief    获取wave文件音频数据大小，调用此方法前必须执行 start 方法
 * @param    void
 * @retval   wave文件音频数据大小(单位byte)
 */
size_t WavFileReader::get_subchunk2_size(void)
{
  return m_wav_header->subchunk2_size;
}



WavFileReader::~WavFileReader()
{
}
