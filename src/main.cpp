/*************************/
/*
@version V2.0
@author Roland
@brief	较V1：成功接入百度ERNIE-Bot大模型
*/
#include <Arduino.h>
#include <driver/gpio.h>
#include <U8g2lib.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/i2s.h"
#include "driver/gpio.h"

#include "ERNIE-Bot.h"
#include "config.h"
#include "WavFileWriter.h"
#include "WavFileReader.h"
#include "sd_card.h"
#include "I2SOutput.h"
#include "I2SINMPSampler.h"
#include "API_online.h"




static const char* TAG = "Roland";

// static char *wifi_ssid = "苹果手机 15 Pro max ultra";
// static char *wifi_password = "roland66";
// static char *wifi_ssid = "苹果手机15 pro max ultra";
// static char *wifi_password = "roland66";
static char *wifi_ssid = "HUAWEI Nova 11 SE";
static char *wifi_password = "roland66";
bool recordFlag = false;//是否开始录音标志位，true-开始
bool LEDT_flag = false;
hw_timer_t*   Timer0 = NULL; // 预先定义一个指针来存放定时器的位置
#define EC11_CD_TIME 4 // 4ms消抖
String sis_payload;// 语音识别后的文本
static uint8_t BLE_set_flag = 0; // wifi设置步骤记录，0-未开始设置wifi，1-正在设置wifi账号，2-正在设置wifi密码，3-正在添加语音识别热词, 4-正在减少语音识别热词
API_online sis_api("cn-north-4", "9512b326c85747cbade191a38a51691c"); // 设置云服务器结点、项目ID
ERNIE_API bot_api;
String chat_content;
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* clock=*/OLED_SCL, /* data=*/OLED_SDA);
sd_card *rowl_sd; // 挂载的TF卡对象
// OLED显示内容消息队列, 具体内容是uint8数据类型指令
#define OLED_SHOWING_LEN 1// 1字节指令
#define OLED_SHOWING_SIZE sizeof(uint8_t)
QueueHandle_t OLED_showing_queue = NULL;
/* UI指令内容
0-显示待机内容
1-显示录音中UI
2-显示识别中UI
3-显示语音识别结果
4-组件准备中
5-没网了
6-语音识别的录音时长按时间过短
7-网络质量不佳，重新操作
8-内存不足，malloc失败
9-语音识别内容为空文本-你说话了O.o?
10-刚启动时显示尝试连接的wifi名
11-开始设置WIFI，请输入wifi名
12-请输入wifi密码
*/
// Wifi账密内容消息队列, 具体内容是char字符串地址
#define WIFI_DATA_LEN_MAX 1
#define WIFI_DATA_SIZE sizeof(char *)
QueueHandle_t Wifi_data_queue = NULL;

/***任务句柄***/
TaskHandle_t OLED_Task_Handle;


#define EXAMPLE_MAX_CHAR_SIZE    64
static esp_err_t s_example_write_file(const char *path, char *data)
{
    ESP_LOGI(TAG, "Opening file %s", path);
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fprintf(f, data);
    fclose(f);
    ESP_LOGI(TAG, "File written");

    return ESP_OK;
}

static esp_err_t s_example_read_file(const char *path)
{
    ESP_LOGI(TAG, "Reading file %s", path);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }
    char line[EXAMPLE_MAX_CHAR_SIZE];
    fgets(line, sizeof(line), f);
    fclose(f);

    // strip newline
    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);

    return ESP_OK;
}


//定时器0 中断函数，1ms触发一次
void IRAM_ATTR Timer0_Handler(){
  static uint16_t millisecond = 0;
  millisecond++;
  if(millisecond == 1000){
    //每秒执行以下内容
    millisecond = 0;
  }
}

//LED_Task 任务函数
void LED_Task(void *parameter){
  uint8_t count = 0;
  uint8_t inver_flag = 1;
  while(1){
    LEDB_OFF;
    LEDA_ON;
    vTaskDelay(4*count+50);
    LEDB_ON;
    LEDA_OFF;
    vTaskDelay(4*count+50);
    if(inver_flag){
      count++;
      if(count == 50)
        inver_flag = 0;
    }
    else{
      count--;
      if(count == 0)
        inver_flag = 1;
    }
  }
}

// Key_Task 任务函数
// void Key_Task(void *parameter){
//   pinMode(Record_Key, INPUT);
//   gpio_set_pull_mode(Record_Key, GPIO_PULLUP_ONLY); // 设置上拉
//   while(1){
//     if(digitalRead(Record_Key) == 0){
//       vTaskDelay(10);
//       recordFlag = true;
//       ESP_LOGI(TAG, "Record begin~");
//       while(digitalRead(Record_Key) == 0);
//       recordFlag = false;
//       ESP_LOGI(TAG, "Record stop~");
//     }
//     vTaskDelay(10);
//   }
// }

//OLED_Task 任务函数
void OLED_Task(void *parameter){
  uint8_t OLED_showing_state = 4;// OLED待机标志位，置0时显示待机动画
  u8g2.begin();
  u8g2.enableUTF8Print(); // 支持显示中文
  xQueueSend(OLED_showing_queue, &OLED_showing_state, 0); // 这里需要先向队列发送一条初始的消息，不然OLED开机没反应
  while(1){
    // TODO: 可以加一个temp，记录上一次接收的消息，如果没变化，那就不执行下面的sendbuf了
    xQueueReceive(OLED_showing_queue, &OLED_showing_state, portMAX_DELAY);
    u8g2.clearBuffer();
    if(OLED_showing_state == 0){
      if((WiFi.status() == WL_CONNECTED)){
        //显示wifi图标
        u8g2.setFont(u8g2_font_siji_t_6x10);
        u8g2.drawGlyph(115, 9, 57882);
      }
      // 待机动画
      static short i,j;
      static short xTemp = 0;
      static short yTemp = 8-1;
      static uint8_t widTemp = 8;
      static uint8_t highTemp = 8;
      u8g2.setDrawColor(1);// 设置有色
        i = xTemp;
        j = 8-1;
        while(j >= yTemp){
            if(j > yTemp)
                i = 16-1;
            else
                i = xTemp;
            while(i >= 0){
              u8g2.drawBox(i*widTemp, j*highTemp, widTemp, highTemp); // 绘制方块
              i--;
            }
            j--;
        }
        if(yTemp > 1 || xTemp < 16-1){
            if(xTemp < 16-1)
              xTemp++;
            else{
              xTemp = 0;
              yTemp--;
            }
        }
        else{
          xTemp = 0;
          yTemp = 8-1;
          u8g2.sendBuffer(); // 同步屏幕
          vTaskDelay(100);
          u8g2.clearBuffer(); // clear the u8g2 buffer
          if((WiFi.status() == WL_CONNECTED)){
            //显示wifi图标
            u8g2.setFont(u8g2_font_siji_t_6x10);
            u8g2.drawGlyph(115, 9, 57882);
          }

          u8g2.sendBuffer(); // 同步屏幕
          vTaskDelay(100);
        }
        // xQueuePeek(OLED_showing_queue, &OLED_showing_state, 0);
      xQueueSend(OLED_showing_queue, &OLED_showing_state, 0);
    }
    else if (OLED_showing_state == 1)
    {
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      u8g2.setCursor(40, 10); // 设置坐标
      u8g2.print("录音中~~");
      u8g2.setCursor(30, 30); // 设置坐标
      u8g2.print("松手录音结束……");
      u8g2.setFont(u8g2_font_siji_t_6x10);
      u8g2.drawGlyphX2(50, 60, 57420);
    }
    else if (OLED_showing_state == 2)
    {
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      u8g2.setCursor(40, 25); // 设置坐标
      u8g2.print("识别中~~");
    }
    else if (OLED_showing_state == 3)
    {
      static uint32_t k = 0;
      static uint8_t x_add = 0;
      static uint16_t y_add = 0;
      static uint32_t y_err = 0; // 控制所有字体上移
      static bool temp_flag = false;
      y_err = 0;
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      do{
        u8g2.clearBuffer();
        for(k = 0,x_add = 0,y_add = 0; k < chat_content.length()-1; k++){
          if(10-y_err>0){
            u8g2.setCursor(0,10-y_err); // 设置坐标
            u8g2.print("========Rowling=======");
          }
          if(25-y_err+y_add*12>0){
            u8g2.setCursor(3+6*x_add,25-y_err+y_add*12); // 设置坐标
            if(chat_content[k]>127){
              // 是中文
              u8g2.print(chat_content[k++]);
              u8g2.print(chat_content[k++]);
              u8g2.print(chat_content[k]);
              x_add+=2;
              temp_flag = true; // 允许刷新屏幕
            }
            else{
              // u8g2.setFont(u8g2_font_samim_16_t_all);
              u8g2.print(chat_content[k]);
              temp_flag = true; // 允许刷新屏幕
              x_add++;
            }
            if(x_add >= 20){
              y_add++;
              x_add = 0;
            }
          }
          else if(chat_content[k]>127)
            k+=2;
        }
        if(temp_flag){
          temp_flag = false;
          u8g2.sendBuffer(); // 同步屏幕
          vTaskDelay(2/portTICK_PERIOD_MS);
        }
        y_err++;
      }while(25-y_err+y_add*12 >= 63);
    }
    else if(OLED_showing_state == 4)
    {
      if((WiFi.status() == WL_CONNECTED)){
        //显示wifi图标
        u8g2.setFont(u8g2_font_siji_t_6x10);
        u8g2.drawGlyph(115, 9, 57882);
      }
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      u8g2.setCursor(25, 30); // 设置坐标
      u8g2.print("组件准备中~~");
    }
    else if(OLED_showing_state == 5)
    {
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      u8g2.setCursor(30, 20); // 设置坐标
      u8g2.print("没网了？");
      u8g2.setCursor(5, 32); // 设置坐标
      u8g2.print("请打开wifi:");
      u8g2.print(wifi_ssid);
      u8g2.setCursor(3, 44); // 设置坐标
      u8g2.print("或BLE设置新wifi");
    }
    else if(OLED_showing_state == 6)
    {
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      u8g2.setCursor(30, 40); // 设置坐标
      u8g2.print("请重新操作！");
    }
    else if(OLED_showing_state == 7)
    {
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      u8g2.setCursor(20, 25); // 设置坐标
      u8g2.print("网络质量不佳qwq");
      u8g2.setCursor(25, 45); // 设置坐标
      u8g2.print("请重新操作QAQ");
    }
    else if(OLED_showing_state == 8)
    {
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      u8g2.setCursor(50, 22); // 设置坐标
      u8g2.print("O.o");
      u8g2.setCursor(40, 35); // 设置坐标
      u8g2.print("内存不足！");
      u8g2.setCursor(10, 55); // 设置坐标
      u8g2.print("请联系作者debug");
    }
    else if(OLED_showing_state == 9)
    {
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      u8g2.setCursor(25, 40); // 设置坐标
      u8g2.print("你说话了O.o？");
    }
    else if(OLED_showing_state == 10)
    {
      static uint8_t m;
      static uint8_t x_add;
      static uint8_t y_add;
      static String str;
      str = "SSID：";
      str += wifi_ssid;
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      u8g2.setCursor(10, 12); // 设置坐标
      u8g2.print("正在尝试链接Wifi~~");
      for(m = 0,x_add = 0,y_add = 0; m < str.length(); m++){
        u8g2.setCursor(0+6*x_add,26+y_add*12); // 设置坐标
        if(str[m]>127){
          // 是中文
          u8g2.print(str[m++]);
          u8g2.print(str[m++]);
          u8g2.print(str[m]);
          x_add+=2;
        }
        else{
          u8g2.print(str[m]);
          x_add++;
        }
        if(x_add >= 20){
          y_add++;
          x_add = 0;
        }
      }
      u8g2.setCursor(0, 60); // 设置坐标
      u8g2.print("可通过BLEapp指定wifi~");
    }
    else if(OLED_showing_state == 11){
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      u8g2.setCursor(20, 30); // 设置坐标
      u8g2.print("请输入wifi账号~");
    }
    else if(OLED_showing_state == 12){
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      u8g2.setCursor(20, 30); // 设置坐标
      u8g2.print("请输入wifi密码~");
    }
    u8g2.sendBuffer(); // 同步屏幕
  }
}

//Get_MIC_To_SD_Task 任务函数
void Get_MIC_To_SD_Task(void * parameter){
  uint8_t queue_temp; //
  int bytes_read = 0xffff;//单次实际采样数
  int16_t err;

  const char* wavPath = "voice.wav";//录音文件路径
  //挂载MicroSD Card
  rowl_sd = new sd_card(SD_MOUNT_POINT, sd_spi_bus_config, SD_SPI_CS); // 实例化TF卡读写对象

  //实例化一个wav文件写手 对象
  WavFileWriter mic_WavFileWriter(rowl_sd, wavPath, SAMPLE_RATE, sizeof(i2s_INMP_sample_t));

  // 实例化一个INMP441的采样器 对象
  I2SINMPSampler mic_I2SINMPSampler(MIC_INMP_I2SPORT, i2s_INMP_config, i2s_INMP_pin_config);

  // 开空间存储采样数据
  i2s_INMP_sample_t *samples_read = (i2s_INMP_sample_t *)malloc(sizeof(i2s_INMP_sample_t) * i2s_INMP_config.dma_buf_len);
  if(samples_read == NULL){
    ESP_LOGE(TAG, "Insufficient heap space, *samples_read* create failed");
    while(1);
  }
  uint8_t *samples_write = (uint8_t*)malloc(i2s_INMP_config.dma_buf_len * sizeof(i2s_INMP_sample_t));
  if(samples_write == NULL){
    ESP_LOGE(TAG, "Insufficient heap space, *samples_write* create failed");
    while(1);
  }

  // 连接wifi并获取sis_api token
  ESP_LOGI(TAG, "Default wifi: %s", wifi_ssid);
  // 先尝试连接默认wifi,等待蓝牙写入新的wifi账密
  WiFi.begin(wifi_ssid, wifi_password);
  do{
    switch (BLE_set_flag)
    {
      // case 0:
      case 1:
        queue_temp = 11;
        xQueueSend(OLED_showing_queue, &queue_temp, portMAX_DELAY); // 通知UI显示输入账号
        xQueueReceive(Wifi_data_queue, &wifi_ssid, portMAX_DELAY);
        ESP_LOGI(TAG, "Wifi账号设置完毕:%s，请输入Wifi密码~", wifi_ssid);
        break;
      case 2:
        queue_temp = 12;
        xQueueSend(OLED_showing_queue, &queue_temp, portMAX_DELAY); // 通知UI显示输入密码
        xQueueReceive(Wifi_data_queue, &wifi_password, portMAX_DELAY);
        ESP_LOGI(TAG, "Wifi密码设置完毕:%s", wifi_password);
        WiFi.begin(wifi_ssid, wifi_password);
        break;
      default:
        ESP_LOGI(TAG, "Waiting to link wifi: %s", wifi_ssid);
        vTaskDelay(500);
        queue_temp = 10;
        xQueueSend(OLED_showing_queue, &queue_temp, portMAX_DELAY); // 告诉OLED显示待联网UI
        break;
    }
  }while(WiFi.status() != WL_CONNECTED);
  ESP_LOGI(TAG, "Succeed in linking wifi: %s", wifi_ssid);

  // 进入待机模式
  queue_temp = 0;
  xQueueSend(OLED_showing_queue, &queue_temp, 0);

  sis_api.start(); // 获取token
  bot_api.start();
  sis_api.queryHotList(); // 查询云端并更新本地热词表
  // //test
  // bot_api.chat("你觉得编程可以作为兴趣爱好吗?", &chat_content);
  // Serial.println(chat_content);
  // sis_api.hAddData("你好");
  // sis_api.createHotList();


  while(1){
    if(digitalRead(Record_Key) == 0){
      recordFlag = true;
      vTaskDelay(10);
      ESP_LOGI(TAG, "Record begin~");
      queue_temp = 1;
      xQueueSend(OLED_showing_queue, &queue_temp, portMAX_DELAY); // 告诉OLED显示“录音中”UI
      // 启用i2s模块获取INMP441音频数据
      mic_I2SINMPSampler.start();
      // 开始向目标文件写入wav头片段
      mic_WavFileWriter.start();
      vTaskDelay(500);
      vTaskSuspend(OLED_Task_Handle); // 挂起用到IIC的任务，影响到spi写入SD卡了
      while(digitalRead(Record_Key) == 0){
        // 采样录音
        bytes_read = mic_I2SINMPSampler.read(samples_read, i2s_INMP_config.dma_buf_len);
        for(int i = 0; i < bytes_read/sizeof(i2s_INMP_sample_t); i++){
          samples_read[i]*=11;
          if(samples_read[i]>32767)
            samples_read[i] = 32767;
          else if(samples_read[i] < -32768)
            samples_read[i] = -32768;
        }
        mic_WavFileWriter.write((uint8_t *)samples_read, sizeof(i2s_INMP_sample_t), bytes_read);
      }
      recordFlag = false;
      ESP_LOGI(TAG, "Record stop~");
      vTaskResume(OLED_Task_Handle);
      // 结束I2S采样
      mic_I2SINMPSampler.stop();
      // 结束wav文件的写入
      mic_WavFileWriter.finish();
      queue_temp = 2;
      xQueueSend(OLED_showing_queue, &queue_temp, portMAX_DELAY); // 告诉OLED显示识别中UI
      //对wav文件进行base64编码
      char *base64_data = rowl_sd->enBase64(wavPath);
      // 将wav文件的base64编码POST到SIS，接收返回数据
sis_api_again:
      err = sis_api.sis(base64_data, &sis_payload);
      ESP_LOGD(TAG, "err=%d", err);
      switch(err)
      {
        case -2: // 与服务器的连接断开了
          goto sis_api_again; //重新来一次
          break;
        case 0: // 成功识别
          // 将语音转文本的数据传给模型
bot_api_again:
          err = bot_api.chat(sis_payload, &chat_content);
          ESP_LOGD(TAG, "err=%d", err);
          switch (err)
          {
            case -2: // 与服务器的连接断开了
              goto bot_api_again; //重新来一次
              break;
            case 0: // 成功聊天
              queue_temp = 3;
              xQueueSend(OLED_showing_queue, &queue_temp, portMAX_DELAY); // 告诉OLED显示语音识别结果UI
              break;
            case 1:// 没网了
              queue_temp = 5;
              xQueueSend(OLED_showing_queue, &queue_temp, portMAX_DELAY);
              break;
            case 2:// 网络连接不畅
              queue_temp = 7;
              xQueueSend(OLED_showing_queue, &queue_temp, portMAX_DELAY);
              break;
          }
          break;
        case 1: // 没网了
          queue_temp = 5;
          xQueueSend(OLED_showing_queue, &queue_temp, portMAX_DELAY);
          break;
        case 2: // 内存不足了
          queue_temp = 8;
          xQueueSend(OLED_showing_queue, &queue_temp, portMAX_DELAY);
          break;
        case 3: // API返回的文本为空
          queue_temp = 9;
          xQueueSend(OLED_showing_queue, &queue_temp, portMAX_DELAY);
          break;
        case 500: // 长按时间过短
          queue_temp = 6;
          xQueueSend(OLED_showing_queue, &queue_temp, portMAX_DELAY);
          break;
        default: // 返回的是httpcode错误状态码
          queue_temp = 7;
          xQueueSend(OLED_showing_queue, &queue_temp, portMAX_DELAY);
          break;
      }
      free(base64_data);
      base64_data = NULL;
      while(digitalRead(Record_Key) == 1); //保持显示识别结果,再次点按回到待机状态
      vTaskDelay(500); // 消抖
      queue_temp = 0;
      xQueueSend(OLED_showing_queue, &queue_temp, portMAX_DELAY); // 告诉OLED显示待机界面
      sis_payload.clear(); // 清空一下
    }
    else{
      vTaskDelay(1);
    }
  }
}

// BLE
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
bool deviceConnected = false;
bool oldDeviceConnected = false;
// 重载onConnect和onDisconnect方法，对刚连接上和刚断开连接时的状态做动作
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* bServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* bServer) {
      deviceConnected = false;
    }
};
// 重新onWrite方法，接收一串数据时做动作
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      static char *wifi_ssid = NULL;
      static char *wifi_password = NULL;
      uint16_t i = 0;
      int8_t err;
      uint8_t queue_temp;
      std::string rxValue = pCharacteristic->getValue();
      if (rxValue.length() > 0 && WiFi.status() != WL_CONNECTED) {
        switch (BLE_set_flag)
        {
          case 0:
            if(rxValue.compare("WIFI") == 0 || rxValue.compare("Wifi") == 0 || rxValue.compare("wifi") == 0 || rxValue.compare("WiFi") == 0){
              Serial.println("请输入Wifi账号~");
              BLE_set_flag = 1;
            }
            else if(rxValue.compare("hot+") == 0 && WiFi.status() == WL_CONNECTED){
              Serial.println("请单个多次输入待增热词~");
              BLE_set_flag = 3;
            }
            else if(rxValue.compare("hot-") == 0 && WiFi.status() == WL_CONNECTED){
              Serial.println("请单个多次输入待减热词~");
              BLE_set_flag = 4;
            }
            else{
              Serial.printf("Received str: %s\n", rxValue.c_str());
            }
            break;
          case 1:
            free(wifi_ssid);
            wifi_ssid = (char *)malloc(sizeof(char)*(rxValue.length()+1));
            strcpy(wifi_ssid, rxValue.c_str());
            xQueueSend(Wifi_data_queue, &wifi_ssid, portMAX_DELAY);
            BLE_set_flag = 2;
            ESP_LOGI(TAG, "Wifi账号设置完毕:%s，请输入Wifi密码~", wifi_ssid);
            break;
          case 2:
            free(wifi_password);
            wifi_password = (char *)malloc(sizeof(char)*(rxValue.length()+1));
            strcpy(wifi_password, rxValue.c_str());
            xQueueSend(Wifi_data_queue, &wifi_password, portMAX_DELAY);
            BLE_set_flag = 0;
            ESP_LOGI(TAG, "Wifi密码设置完毕:%s", wifi_password);
            break;
          case 3:
          //添加热词
            if(rxValue.compare("over") == 0){
              BLE_set_flag = 0;
              err = sis_api.updateHotList();
              if(err != 0){
                ESP_LOGE(TAG, "Update Hot-List error, error state is %d", err);
              }
            }
            else{
              // hAddData(sis_api, rxValue.c_str());
            }
            break;
          case 4:
          //删除热词
            if(rxValue.compare("over") == 0){
              BLE_set_flag = 0;
              err = sis_api.updateHotList();
              if(err != 0){
                ESP_LOGE(TAG, "Update Hot-List error, error state is %d", err);
              }
            }
            else{
              // hReduceData(sis_api, rxValue.c_str());
            }
        default:
          break;
        }
      }
    }
};
// BLE_Handler_Task 任务函数
// 确保蓝牙功能正常稳定
void BLE_Handler_Task(void *parameter){
  uint8_t *txValue = NULL; // BLE tx 待发送数据指针
  uint32_t txLenth = 0; // BLE tx 待发送数据长度(byte)
  uint8_t i = 0;
  BLEServer *bServer = NULL; // 服务端，也就是本机
  BLECharacteristic * bTxCharacteristic; // 单服务的特征-发送通知
  BLECharacteristic * bRxCharacteristic; // 单服务的特征-接收
  // Create the BLE Device
  BLEDevice::init("Rowling-v2"); //启动蓝牙并命设备名
  BLEDevice::setMTU(29+3); //增大MTU至32Byte（默认23），正好是华为手机设备名称的最长值
  // Create the BLE Server
  bServer = BLEDevice::createServer(); // 实例化BLE服务端（从机）
  bServer->setCallbacks(new MyServerCallbacks());

  BLEService *bService = bServer->createService(SERVICE_UUID); // 实例化BLE服务

  // bTxCharacteristic = bService->createCharacteristic(
	// 									CHARACTERISTIC_UUID_TX, // UUID
	// 									BLECharacteristic::PROPERTY_NOTIFY // 属性
	// 								); // 实例化BLE特征
  // bTxCharacteristic->addDescriptor(new BLE2902()); // 添加一个0x2902描述符对象

  bRxCharacteristic = bService->createCharacteristic(
											 CHARACTERISTIC_UUID_RX, // UUID
											BLECharacteristic::PROPERTY_WRITE // 属性
										); // 实例化BLE特征
  bRxCharacteristic->setCallbacks(new MyCallbacks()); //设置回调函数对象

  bService->start(); // 开始服务

  bServer->getAdvertising()->start(); // 开始广播，等待主机链接
  ESP_LOGI(TAG, "Waiting a client connection to notify...");
  while(1){
    // if (deviceConnected) {
    //   // txValue = &i;
    //   // txLenth = 1;
    //   txValue = (uint8_t *)"This is Rowling\n";
    //   txLenth = 16;
    //   bTxCharacteristic->setValue(txValue, txLenth);// 缓冲区上弹
    //   bTxCharacteristic->notify();// 发布通知
    //   i++;
    //   vTaskDelay(500); // bluetooth stack will go into congestion, if too many packets are sent
    // }

    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
      vTaskDelay(500); // give the bluetooth stack the chance to get things ready
      bServer->startAdvertising(); // restart advertising
      ESP_LOGI(TAG, "restart advertising");
      oldDeviceConnected = deviceConnected;
    }

    // connecting
    if (deviceConnected && !oldDeviceConnected) {
      ESP_LOGI(TAG, "Getting a client connection to notify");
      // do stuff here on connecting
      oldDeviceConnected = deviceConnected;
    }
  }
}

void setup(){
  Serial.begin(115200);
  Serial.printf("CPU run in %d MHz\n", ESP.getCpuFreqMHz());
  // 打印PSRAM大小
  Serial.print("Free PSRAM after allocation: ");
  Serial.println(heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
  // 创建消息队列
  OLED_showing_queue = xQueueCreate(OLED_SHOWING_LEN, OLED_SHOWING_SIZE);
  Wifi_data_queue = xQueueCreate(WIFI_DATA_LEN_MAX, WIFI_DATA_SIZE);

  // 创建各任务
  pinMode(LEDA,OUTPUT);
  pinMode(LEDB,OUTPUT);
  xTaskCreate(LED_Task, "LED_Task", 1024*2, NULL, 1, NULL);

  xTaskCreate(OLED_Task, "OLED_Task", 1024*4, NULL, 1, &OLED_Task_Handle);

  pinMode(Record_Key, INPUT);
  gpio_set_pull_mode(Record_Key, GPIO_PULLUP_ONLY); // 设置上拉
  xTaskCreate(Get_MIC_To_SD_Task, "Get_MIC_To_SD_Task", 1024*6, NULL, 1, NULL);

  xTaskCreate(BLE_Handler_Task, "BLE_Handler_Task", 1024*10, NULL, 1, NULL);

  // 配置定时器0中断
  // Timer0 = timerBegin(0, 80, true); // 初始化定时器，80分频至1Mhz,
  // timerAttachInterrupt(Timer0, &Timer0_Handler, true); //注册中断，边沿触发
  // timerAlarmWrite(Timer0, 1000, true); // 编辑定时器更新中断细节
  // timerAlarmEnable(Timer0);  // 启动定时器

  // 关闭各核心看门狗
  disableCore0WDT();
  // disableCore1WDT(); //默认就没开启


  // 放音测试
  // const char *wavPath = "/daoxiang.wav";//录音文件路径
  // const char *wavPath = "/jiangnan-320.wav";//录音文件路径
  const char *wavPath = "/xuanni.wav";//录音文件路径
  size_t readed_len; // 每次读出的数据长度
  sd_card *play_sd = new sd_card(SD_MOUNT_POINT, sd_spi_bus_config, SD_SPI_CS);
  WavFileReader wav_reader(play_sd); // 实例化wave文件读者
  I2SOutput wav_player(PLAYER_I2SPORT, i2s_player_pin_config); // 实例化I2S的音频播放驱动

  wav_reader.start(wavPath); // 开始读取wave
  wav_reader.set_esp_i2s_config(&i2s_player_config);// 实时配置i2s
  wav_player.start(&i2s_player_config); // 开始播放wave
uint8_t new_buf[1024*256];
  uint8_t *voicebuf = wav_reader.get_Audiobuffer(i2s_player_config.dma_buf_len);
  uint32_t index = 0;
  do{
    ret = wav_reader.read(&new_buf[index], i2s_player_config.dma_buf_len,&readed_len); // 1024Byte
    if (ret != 0)
    {

    }

    // for(int i = 0; i < readed_len; i++)
    //   voicebuf[i]/=64; // 降音量
    Serial.println(readed_len);
    wav_player.play(voicebuf, readed_len);
  }while(readed_len);
  wav_reader.stop();
  wav_player.stop();
  free(voicebuf);
  voicebuf = NULL;
}


void loop(){

}
