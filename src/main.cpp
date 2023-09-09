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
#include <driver/gpio.h>
#include <U8g2lib.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>

bool recordFlag = false;//是否开始录音标志位，true-开始
static const char* TAG = "Roland";
bool LEDT_flag = false;
int8_t EC11_dir = 0; // EC11旋转编码器旋转方向，顺时针为1，静止为0，逆时针为-1
int16_t EC11_count = 0; // EC11旋转脉冲数，正数为顺时针旋转数
bool EC11A_flag = false; //是否检测到EC11 A端的脉冲标志位
bool EC11B_flag = false; //是否检测到EC11 B端的脉冲标志位
hw_timer_t*   Timer0 = NULL; // 预先定义一个指针来存放定时器的位置
#define EC11_CD_TIME 4 // 4ms消抖
String sis_payload;// 语音识别后的文本
static uint8_t BLE_set_flag = 0; // wifi设置步骤记录，0-未开始设置wifi，1-正在设置wifi账号，2-正在设置wifi密码，3-正在添加语音识别热词, 4-正在减少语音识别热词
API_online sis_api("cn-north-4", "9512b326c85747cbade191a38a51691c"); // 设置云服务器结点、项目ID
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* clock=*/OLED_SCL, /* data=*/OLED_SDA);
// OLED显示内容消息队列, 具体内容是uint8数据类型指令
#define OLED_SHOWING_LEN 1// 1字节指令
#define OLED_SHOWING_SIZE sizeof(uint8_t)
QueueHandle_t OLED_showing_queue = NULL;
/* 指令内容
0-显示待机内容
1-显示录音中UI
2-显示识别中UI
3-显示语音识别结果
4-组件准备中
8-未联网
*/
// Wifi账密内容消息队列, 具体内容是char字符串地址
#define WIFI_DATA_LEN_MAX 1
#define WIFI_DATA_SIZE sizeof(char *)
QueueHandle_t Wifi_data_queue = NULL;

/***任务句柄***/
TaskHandle_t OLED_Task_Handle;

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
  while(1){
    LEDA_ON;
    vTaskDelay(100);
    LEDA_OFF;
    vTaskDelay(1000);
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
        if(yTemp > 0 || xTemp < 16-1){
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
          vTaskDelay(300);
          u8g2.clearBuffer(); // clear the u8g2 buffer
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
    }
    else if (OLED_showing_state == 2)
    {
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      u8g2.setCursor(40, 25); // 设置坐标
      u8g2.print("识别中~~");
    }
    else if (OLED_showing_state == 3)
    {
      u8g2.setCursor(15,10); // 设置坐标
      u8g2.print("==识别内容==");
      static uint8_t k = 0;
      static uint8_t x_add = 0;
      static uint8_t y_add = 0;
      for(k = 0,x_add = 0,y_add = 0; k < sis_payload.length(); k++){
        u8g2.setCursor(3+12*x_add,30+y_add*12); // 设置坐标
        if(sis_payload[k]>127){
          //是中文
          u8g2.setFont(u8g2_font_wqy12_t_gb2312);
          u8g2.print(sis_payload[k++]);
          u8g2.print(sis_payload[k++]);
          u8g2.print(sis_payload[k]);
        }
        else{
          u8g2.setFont(u8g2_font_samim_16_t_all);
          u8g2.print(sis_payload[k]);
        }
        x_add++;
        if(x_add == 10){
          y_add++;
          x_add = 0;
        }
      }
      // u8g2.setCursor(3,50); // 设置坐标
      // u8g2.print(sis_payload);
    }
    else if(OLED_showing_state == 4)
    {
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      u8g2.setCursor(25, 30); // 设置坐标
      u8g2.print("组件准备中~~");
    }
    u8g2.sendBuffer(); // 同步屏幕
  }
}

//Get_MIC_To_SD_Task 任务函数
void Get_MIC_To_SD_Task(void * parameter){
  uint8_t queue_pvItem;

  // 连接wifi并获取sis_api token
  static char *wifi_ssid = "Xiaomi_4c";
  static char *wifi_password = "l18005973661";
  ESP_LOGI(TAG, "Default wifi: %s", wifi_ssid);
  // 先尝试连接默认wifi,等待蓝牙写入新的wifi账密
  WiFi.begin(wifi_ssid, wifi_password);
  do{
    switch (BLE_set_flag)
    {
      // case 0:
      case 1:
        xQueueReceive(Wifi_data_queue, &wifi_ssid, portMAX_DELAY);
        ESP_LOGI(TAG, "Wifi账号设置完毕:%s，请输入Wifi密码~", wifi_ssid);
        break;
      case 2:
        xQueueReceive(Wifi_data_queue, &wifi_password, portMAX_DELAY);
        ESP_LOGI(TAG, "Wifi密码设置完毕:%s", wifi_password);
        WiFi.begin(wifi_ssid, wifi_password);
        break;
      default:
        ESP_LOGI(TAG, "Waiting to link wifi: %s", wifi_ssid);
        vTaskDelay(500);
        break;
    }
  }while(WiFi.status() != WL_CONNECTED);
  ESP_LOGI(TAG, "Succeed in linking wifi: %s", wifi_ssid);

  sis_api.start(); // 获取token
  sis_api.queryHotList(); // 查询云端并更新本地热词表
  // sis_api.hAddData("你好");
  // sis_api.createHotList();

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
  int bytes_read = 0xffff;//单次实际采样数

  // test
  // mic_I2SINMPSampler.start();
  queue_pvItem = 0;
  xQueueSend(OLED_showing_queue, &queue_pvItem, 0); // 进入待机模式
  while(1){
    if(digitalRead(Record_Key) == 0){
      recordFlag = true;
      vTaskDelay(10);
      ESP_LOGI(TAG, "Record begin~");
      queue_pvItem = 1;
      xQueueSend(OLED_showing_queue, &queue_pvItem, portMAX_DELAY); // 告诉OLED显示“录音中”UI
      // 启用i2s模块获取INMP441音频数据
      mic_I2SINMPSampler.start();
      // 开始向目标文件写入wav头片段
      mic_WavFileWriter.start();
      vTaskDelay(500);
      vTaskSuspend(OLED_Task_Handle); // 挂起用到IIC的任务，影响到spi写入SD卡了
      while(digitalRead(Record_Key) == 0){
        // 采样录音
        // Serial.println("demo");
        bytes_read = mic_I2SINMPSampler.read(samples_read, i2s_INMP_config.dma_buf_len);
        for(int i = 0; i < bytes_read/sizeof(i2s_INMP_sample_t); i++){
          samples_read[i]*=10;
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
      queue_pvItem = 2;
      xQueueSend(OLED_showing_queue, &queue_pvItem, portMAX_DELAY); // 告诉OLED显示识别中UI
      //对wav文件进行base64编码
      File myfile = SD.open(wavPath,FILE_READ);
      char *base64Data = FiletoBase64(myfile);
      myfile.close();
      // 将wav文件的base64编码POST到SIS，接收返回数据
      sis_payload = sis_api.sis(base64Data);
      Serial.println(sis_payload);
      queue_pvItem = 3;
      xQueueSend(OLED_showing_queue, &queue_pvItem, portMAX_DELAY); // 告诉OLED显示语音识别结果UI
      free(base64Data);
      base64Data = NULL;
      vTaskDelay(5000); //保持5秒显示识别结果
      queue_pvItem = 0;
      xQueueSend(OLED_showing_queue, &queue_pvItem, portMAX_DELAY); // 告诉OLED显示语音识别结果UI
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
  BLEDevice::init("Rowling-v1"); //启动蓝牙并命设备名
  BLEDevice::setMTU(29+3); //增大MTU至32Byte（默认23），正好是华为手机设备名称的最长值
  // Create the BLE Server
  bServer = BLEDevice::createServer(); // 实例化BLE服务端（从机）
  bServer->setCallbacks(new MyServerCallbacks());

  BLEService *bService = bServer->createService(SERVICE_UUID); // 实例化BLE服务

  bTxCharacteristic = bService->createCharacteristic(
										CHARACTERISTIC_UUID_TX, // UUID
										BLECharacteristic::PROPERTY_NOTIFY // 属性
									); // 实例化BLE特征
  bTxCharacteristic->addDescriptor(new BLE2902()); // 添加一个0x2902描述符对象

  bRxCharacteristic = bService->createCharacteristic(
											 CHARACTERISTIC_UUID_RX, // UUID
											BLECharacteristic::PROPERTY_WRITE // 属性
										); // 实例化BLE特征
  bRxCharacteristic->setCallbacks(new MyCallbacks()); //设置回调函数对象

  bService->start(); // 开始服务

  bServer->getAdvertising()->start(); // 开始广播，等待主机链接
  ESP_LOGI(TAG, "Waiting a client connection to notify...");
  while(1){
    if (deviceConnected) {
      // txValue = &i;
      // txLenth = 1;
      txValue = (uint8_t *)"This is Rowling\n";
      txLenth = 16;
      bTxCharacteristic->setValue(txValue, txLenth);// 缓冲区上弹
      bTxCharacteristic->notify();// 发布通知
      i++;
      vTaskDelay(500); // bluetooth stack will go into congestion, if too many packets are sent
    }

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
  Serial.begin(921600);
  pinMode(LEDA,OUTPUT);
  pinMode(LEDB,OUTPUT);
  pinMode(Record_Key, INPUT);
  gpio_set_pull_mode(Record_Key, GPIO_PULLUP_ONLY); // 设置上拉
  // 打印PSRAM大小
  Serial.print("Free PSRAM after allocation: ");
  Serial.println(heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
  // 创建消息队列
  OLED_showing_queue = xQueueCreate(OLED_SHOWING_LEN, OLED_SHOWING_SIZE);
  Wifi_data_queue = xQueueCreate(WIFI_DATA_LEN_MAX, WIFI_DATA_SIZE);
  // 创建各任务
  xTaskCreate(LED_Task, "LED_Task", 1024*2, NULL, 1, NULL);
  // xTaskCreate(Key_Task, "Key_Task", 1024*2, NULL, 1, NULL);
  xTaskCreate(OLED_Task, "OLED_Task", 1024*4, NULL, 1, &OLED_Task_Handle);
  xTaskCreate(Get_MIC_To_SD_Task, "Get_MIC_To_SD_Task", 1024*6, NULL, 1, NULL);
  // xTaskCreate(BLE_Handler_Task, "BLE_Handler_Task", 1024*10, NULL, 1, NULL);

  // 配置定时器0中断
  // Timer0 = timerBegin(0, 80, true); // 初始化定时器，80分频至1Mhz,
  // timerAttachInterrupt(Timer0, &Timer0_Handler, true); //注册中断，边沿触发
  // timerAlarmWrite(Timer0, 1000, true); // 编辑定时器更新中断细节
  // timerAlarmEnable(Timer0);  // 启动定时器
  // 关闭各核心看门狗
  disableCore0WDT();
  // disableCore1WDT(); //默认就没开启
}


void loop(){
}
