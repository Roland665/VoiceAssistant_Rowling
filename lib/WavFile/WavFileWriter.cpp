#include "esp_log.h"
#include "WAVFileWriter.h"
#include "base64.h"

static const char *TAG = "Wav_Write";

WavFileWriter::WavFileWriter(const char *wavPath, int sample_rate)
{
  m_header.sample_rate = sample_rate;
  m_wavPath = wavPath;
}

/**
 * @brief    开始向wav文件写入音频数据，并率先写入文件头
 * @param    void
 * @retval   void
 */
void WavFileWriter::start()
{
  ESP_LOGI(TAG, "Start writing wav file", m_file_size);
  // write out the header - we'll fill in some of the blanks later
  // fwrite(&m_header, sizeof(wav_header_t), 1, m_fp);
  // m_file_size = sizeof(wav_header_t);

  m_file = SD.open(m_wavPath, FILE_WRITE);
  if(!m_file){
    Serial.println("Failed to open file for writing");
    return;
  }
  //写入 wav文件头
  m_file.write((uint8_t*)&m_header, sizeof(wav_header_t));
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
  for(int i = 0; i < count; i++){
    m_file.write(samples, sample_size);
  }
  // fwrite(samples, sizeof(int16_t), count, m_fp);
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
  // now fill in the header with the correct information and write it again
  m_header.subchunk2_size = m_file_size - sizeof(wav_header_t);
  m_header.chunk_size = m_file_size - 8;
  m_file.seek(0, SeekSet);
  m_file.write((uint8_t*)&m_header, sizeof(wav_header_t));
  // fseek(m_fp, 0, SEEK_SET);
  // fwrite(&m_header, sizeof(wav_header_t), 1, m_fp);
  m_file.close();
  // m_file = SD.open(m_wavPath, FILE_READ);
  // ESP_LOGI(TAG, "  FILE: %s", m_file.name());
  // ESP_LOGI(TAG, "  SIZE: %d", m_file.size());
  // String a64 = m_file.readString();
  // ESP_LOGI(TAG, "ReadString successfully");
  // m_file.close();
}
