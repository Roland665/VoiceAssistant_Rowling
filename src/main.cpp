/*************************/
/*
@version V1.0
@author Roland
@brief	实现了录音至wav并上传至华为SIS API实现语音识别
*/
#include <Arduino.h>
#include "config.h"
#include "WavFileWriter.h"
#include "SD_Card.h"
#include <esp_task_wdt.h>
#include "I2SINMPSampler.h"
#include <WiFi.h>
#include "API_online.h"
#include "myBase64.h"
bool recordFlag = false;//是否开始录音标志位，true-开始
static const char* TAG = "Roland";
bool LEDT_flag = false;

//LED_Task 任务函数
void LED_Task(void *parameter){
  pinMode(LEDA,OUTPUT);
  pinMode(LEDB,OUTPUT);
  while(1){
    LEDA_ON;
    vTaskDelay(100);
    LEDA_OFF;
    vTaskDelay(1000);
  }
}

// Key_Task 任务函数
void Key_Task(void *parameter){
  pinMode(Record_Key, INPUT);
  gpio_set_pull_mode(Record_Key, GPIO_PULLUP_ONLY); // 设置上拉
  while(1){
    if(digitalRead(Record_Key) == 0){
      vTaskDelay(10);
      while(digitalRead(Record_Key) == 0);
      recordFlag = true;
      ESP_LOGI(TAG, "Record begin~");
      vTaskDelay(5000);
      recordFlag = false;
      ESP_LOGI(TAG, "Record stop~");
    }
    vTaskDelay(10);
  }
}

//Get_MIC_To_SD_Task 任务函数
void Get_MIC_To_SD_Task(void * parameter){
  String sis_payload;
  const char* wavPath = "/voice.wav";//录音文件路径
  //挂载MicroSD Card
  SPIClass SD_SPI_Config;
  SD_SPI_Config.begin(SD_SPI_SCLK, SD_SPI_MISO, SD_SPI_MOSI, SD_SPI_CS);
  while(!SD.begin(SD_SPI_CS, SD_SPI_Config)){
    Serial.println("Card Mount Failed");
    delay(1000);
  }
  //检测SD卡型号
  uint8_t cardType = SD.cardType();
  while(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    vTaskDelay(1000);
    cardType = SD.cardType();
  }
  Serial.print("SD Card Type: ");
  switch (cardType)
  {
  case CARD_MMC:
    Serial.println("MMC");
    break;
  case CARD_SDHC:
    Serial.println("SDHC");
    break;
  case CARD_SD:
    Serial.println("SDSC");
    break;
  default:
    Serial.println("UNKNOWN");
    break;
  }
  //检测SD卡大小
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  ESP_LOGI(TAG, "SD Card Size: %lluMB\r\n", cardSize);

  //实例化一个wav文件写手 对象
  WavFileWriter mic_WavFileWriter(wavPath, SAMPLE_RATE);

  // 连接wifi并获取sis_api token
  API_online sis_api("苹果手机 15 Pro max ultra", "roland66", "https://iam.cn-north-4.myhuaweicloud.com/v3/auth/tokens", "https://sis-ext.cn-north-4.myhuaweicloud.com/v1/9512b326c85747cbade191a38a51691c/asr/short-audio");
  sis_api.start();

  // 实例化一个INMP441的采样器 对象
  I2SINMPSampler mic_I2SINMPSampler(MIC_INMP_I2SPORT, i2s_INMP_config, i2s_INMP_pin_config);

  // 开空间存储采样数据
  i2s_INMP_sample_t *samples_read = (i2s_INMP_sample_t *)malloc(sizeof(i2s_INMP_sample_t) * i2s_INMP_config.dma_buf_len);
  uint8_t *samples_write = (uint8_t*)malloc(i2s_INMP_config.dma_buf_len * sizeof(i2s_INMP_sample_t));
  int bytes_read = 0xffff;//单次实际采样数

  // test
  // mic_I2SINMPSampler.start();
  while(1){
    if(recordFlag == true){
      Serial.println("hello");
      // 启用i2s模块获取INMP441音频数据
      mic_I2SINMPSampler.start();
      vTaskDelay(1000);//延时1s等待i2s模块稳定
      // 开始向目标文件写入wav头片段
      mic_WavFileWriter.start();
      while(recordFlag){
        // 采样录音
        bytes_read = mic_I2SINMPSampler.read(samples_read, i2s_INMP_config.dma_buf_len);
        mic_WavFileWriter.write((uint8_t *)samples_read, sizeof(i2s_INMP_sample_t), bytes_read);
      }
      // 结束I2S采样
      mic_I2SINMPSampler.stop();
      // 结束wav文件的写入
      mic_WavFileWriter.finish();

      //对wav文件进行base64编码
      File myfile = SD.open(wavPath,FILE_READ);
      char *base64Data = FiletoBase64(myfile);
      myfile.close();
      // Serial.println(base64Data);
      // 将wav文件的base64编码post到SIS，接收返回数据
      sis_payload = sis_api.sis(base64Data);
      Serial.println(sis_payload);
    }
    else
      vTaskDelay(1); // 不加这一行这个任务似乎进入while(1)就不再被执行
  }
}

void setup() {
  Serial.begin(921600);
  // pinMode(LEDA ,OUTPUT);
  // LEDA_ON;

  Serial.print("Free PSRAM after allocation: ");
  Serial.println(heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
  //启动各任务
  disableCore0WDT();
  // disableCore1WDT(); //默认就没开启
  xTaskCreate(LED_Task, "LED_Task", 1024*2, NULL, 1, NULL);
  xTaskCreate(Key_Task, "Key_Task", 1024*2, NULL, 1, NULL);
  xTaskCreate(Get_MIC_To_SD_Task, "Get_MIC_To_SD_Task", 1024*8, NULL, 1, NULL);
}

void loop(){
}

