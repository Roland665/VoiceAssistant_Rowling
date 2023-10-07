#pragma once

#include <stdio.h>
#include "WavFile.h"
#include "SD_Card.h"

class WavFileWriter{

private:
  long m_file_size;      //Wave file size
  wav_header_t m_header;//Wave file header
  const char *m_wavPath;//Wave file path
  File m_file;

public:
  WavFileWriter(const char *wavPath, int sample_rate, uint8_t bytes_per_sample);
  void start();
  void write(const uint8_t *samples, size_t sample_size, int count);
  void finish();
};
