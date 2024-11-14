#include "esp_log.h"
#include "WAVFileWriter.h"
#include "base64.h"

static const char *TAG = "Wav_Write";

WavFileWriter::WavFileWriter(sd_card *_sdcard, const char *wavPath, int sample_rate, uint8_t bytes_per_sample)
{
  m_sdcard = _sdcard;
  m_header.sample_rate = sample_rate;
  m_header.bits_per_sample = bytes_per_sample*8;
  m_header.block_align = m_header.num_channels * bytes_per_sample;
  m_header.byte_rate = sample_rate * m_header.block_align;
  m_wavPath = wavPath;
}

/**
 * @brief    开始向wav文件写入音频数据，并率先写入文件头
 * @param    void
 * @retval   void
 */
void WavFileWriter::start(void)
{
  ESP_LOGI(TAG, "Start writing wav file");
  // 打开文件
  if(m_sdcard->start(m_wavPath, "w"))
    ESP_LOGE(TAG, "Failed to open file for writing");
  //写入 wav文件头
  m_sdcard->write(&m_header, sizeof(wav_header_t), 1);
  m_file_size = sizeof(wav_header_t);
}

/**
 * @brief    向wav文件写入音频数据
 * @param    samples    : 待写入的采样数据
 * @param    sample_size: 单采样数据的大小(一般用sizeof(数据类型)获取大小后传入)
 * @param    count      : 一次需写入的数量
 * @retval   void
 */
void WavFileWriter::write(const uint8_t *samples, size_t sample_size, int count)
{
  // write the samples and keep track of the file size so far
  m_sdcard->write(samples, sample_size, count);
  m_file_size += sample_size * count;
}


/**
 * @brief    结束向wav文件写入音频数据,并生成一串该wav文件base64编码后的字符串
 * @param    samples    : 待写入的采样数据
 * @param    sample_size: 单采样数据的大小(一般用sizeof(数据类型)获取大小后传入)
 * @param    count      : 一次需写入的数量
 * @retval   void
 */
void WavFileWriter::finish()
{
  ESP_LOGI(TAG, "Finishing wav file size: %d", m_file_size);
  // 往wav头填充新的信息
  m_header.subchunk2_size = m_file_size - sizeof(wav_header_t);
  m_header.chunk_size = m_file_size - 8;

  m_sdcard->seek(0, SEEK_SET); // 文件操作偏移量指向文件开头
  m_sdcard->write(&m_header, sizeof(wav_header_t), 1); // 写新的wav头
  m_sdcard->stop(); // 结束wav文件编写
}
