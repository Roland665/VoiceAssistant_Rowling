#pragma once

#pragma pack(push, 1) //采用1字节对齐

typedef struct _wav_header
{
  // RIFF Header
  char chunk_ID[4];           // Contains "RIFF"
  int  chunk_size = 0;        // Size of the wav portion of the file, which follows the first 8 bytes. File size - 8
  char format[4];             // Contains "WAVE"

  // Format Header
  char subchunk1_ID[4];       // Contains "fmt " (includes trailing space)
  int subchunk1_size = 16;    // Should be 16 for PCM
  short audio_format = 1;     // Should be 1 for PCM. 3 for IEEE Float
  short num_channels = 1;     // Number of sound channels
  int sample_rate = 32000;    // Sample rate
  int byte_rate = 32000 * 1 * 2;      // Number of bytes per second. sample_rate * num_channels * Bytes Per Sample
  short block_align = 1*2;      // num_channels * Bytes Per Sample
  short bits_per_sample = 16; // Number of bits per sample

  // Data
  char subchunk2_ID[4];       // Contains "data"
  int subchunk2_size = 0;     // Number of bytes in data. Number of samples * num_channels * sample byte size
  _wav_header()
  {
    chunk_ID[0] = 'R';
    chunk_ID[1] = 'I';
    chunk_ID[2] = 'F';
    chunk_ID[3] = 'F';
    format[0] = 'W';
    format[1] = 'A';
    format[2] = 'V';
    format[3] = 'E';
    subchunk1_ID[0] = 'f';
    subchunk1_ID[1] = 'm';
    subchunk1_ID[2] = 't';
    subchunk1_ID[3] = ' ';
    subchunk2_ID[0] = 'd';
    subchunk2_ID[1] = 'a';
    subchunk2_ID[2] = 't';
    subchunk2_ID[3] = 'a';
  }
}wav_header_t;

// 带有LIST的wave文件头
typedef struct _wav_header_with_list
{
  // RIFF Header
  char chunk_ID[4];           // Contains "RIFF"
  int  chunk_size = 0;        // Size of the wav portion of the file, which follows the first 8 bytes. File size - 8
  char format[4];             // Contains "WAVE"

  // Format Header
  char subchunk1_ID[4];       // Contains "fmt " (includes trailing space)
  int subchunk1_size = 16;    // Should be 16 for PCM
  short audio_format = 1;     // Should be 1 for PCM. 3 for IEEE Float
  short num_channels = 1;     // Number of sound channels
  int sample_rate = 32000;    // Sample rate
  int byte_rate = 32000 * 1 * 2;      // Number of bytes per second. sample_rate * num_channels * Bytes Per Sample
  short block_align = 1*2;      // num_channels * Bytes Per Sample
  short bits_per_sample = 16; // Number of bits per sample

  // List
  char subchunk2_ID[4]; // Contains "LIST"
  unsigned int subchunk2_size; // The length of subchunk2_data
  char *subchunk2_data; // The data of List

  // Data
  char subchunk3_ID[4];       // Contains "data"
  int subchunk3_size = 0;     // Number of bytes in data. Number of samples * num_channels * sample byte size
  _wav_header_with_list()
  {
    chunk_ID[0] = 'R';
    chunk_ID[1] = 'I';
    chunk_ID[2] = 'F';
    chunk_ID[3] = 'F';
    format[0] = 'W';
    format[1] = 'A';
    format[2] = 'V';
    format[3] = 'E';
    subchunk1_ID[0] = 'f';
    subchunk1_ID[1] = 'm';
    subchunk1_ID[2] = 't';
    subchunk1_ID[3] = ' ';
    subchunk2_ID[0] = 'L';
    subchunk2_ID[1] = 'I';
    subchunk2_ID[2] = 'S';
    subchunk2_ID[3] = 'T';
    subchunk3_ID[0] = 'd';
    subchunk3_ID[1] = 'a';
    subchunk3_ID[2] = 't';
    subchunk3_ID[3] = 'a';
  }
}wav_header_with_list_t;

#pragma pack(pop)//恢复对齐状态
