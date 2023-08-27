#include "myBase64.h"

static const char* TAG = "Base64";
static const char* to_base64_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


/**
  * @brief		将文件base64编码
  * @param		myfile: 文件指针
  * @retval		编码后字符串的头指针
  */

char* FiletoBase64(File myfile){
  // 获取文件大小并打印
  size_t file_size = myfile.size();
  ESP_LOGI(TAG, "File:%s  Size: %d", myfile.name(), file_size);

  // 计算文件大小与3byte的最大公倍数之差
  uint8_t size_err = 0;
  if(file_size % 3 != 0){
    size_err = 3 - file_size%3;
  }
  // ESP_LOGI(TAG, "size_err: %d\r\n", size_err);

  uint8_t fileDatas[3];//文件读取临时存放区
  // 开空间存放base64格式的数据
    Serial.print("Free heap after freeing: ");
    Serial.println(ESP.getFreeHeap());
    Serial.print("Free PSRAM after freeing: ");
    Serial.println(heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
  char *base64Data = (char*)malloc(sizeof(char) * (4 * (file_size+size_err)/3 + 1));
    Serial.print("Free heap after freeing: ");
    Serial.println(ESP.getFreeHeap());
    Serial.print("Free PSRAM after freeing: ");
    Serial.println(heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
  // char *base64Data = (char*)heap_caps_malloc(sizeof(char) * 4 * (file_size+size_err)/3 + 1, MALLOC_CAP_SPIRAM);
  if(base64Data == NULL){
    ESP_LOGE(TAG, "Insufficient heap space, *base64Data* create failed");
    while(1);
  }

  // 开始base64编码
  int dataIndex = 0;
  int i = 0;
  for(; i < file_size/3; i++){
    myfile.read(fileDatas, 3);
    base64Data[dataIndex++] = to_base64_table[(fileDatas[0] & 0xFC)>>2];// 取第一字节的前6bit
    base64Data[dataIndex++] = to_base64_table[((fileDatas[0] & 0x03)<<4) + ((fileDatas[1] & 0xF0)>>4)]; //取第一字节的后2bit和第二字节的前4bit
    base64Data[dataIndex++] = to_base64_table[((fileDatas[1] & 0x0F)<<2) + ((fileDatas[2] & 0xC0)>>6)]; //取第二字节的后4bit和第三字节的前2bit
    base64Data[dataIndex++] = to_base64_table[(fileDatas[2] & 0x3F)]; //取第三字节的后6bit
  }
  if(size_err == 2){
    myfile.read(fileDatas, 1);
    base64Data[dataIndex++] = to_base64_table[(fileDatas[0] & 0xFC)>>2]; // 取第一字节的前6bit
    base64Data[dataIndex++] = to_base64_table[(fileDatas[0] & 0x03)<<4]; // 取第一字节的后2bit和补零
    base64Data[dataIndex++] = '='; // 补零
    base64Data[dataIndex++] = '='; // 补零
  }
  else if(size_err == 1){
    myfile.read(fileDatas, 2);
    base64Data[dataIndex++] = to_base64_table[(fileDatas[0] & 0xFC)>>2]; // 取第一字节的前6bit
    base64Data[dataIndex++] = to_base64_table[((fileDatas[0] & 0x03)<<4) + ((fileDatas[1] & 0xF0)>>4)]; //取第一字节的后2bit和第二字节的前4bit
    base64Data[dataIndex++] = to_base64_table[(fileDatas[1] & 0x0F)<<2]; // 取第二字节的后4bit和补零
    base64Data[dataIndex++] = '='; // 补零
  }
  base64Data[dataIndex] = 0;//最后一位补停止位
  // void *base64Data_void = (String *)heap_caps_malloc(1024*1024, MALLOC_CAP_SPIRAM);//写死2M的堆区给base64data
  // if (base64Data_void == NULL){
  //   ESP_LOGE(TAG, "Insufficient heap space, *base64Data_void* create failed");
  //   while(1);
  // }
  // String *base64Data_String = new(base64Data_void) String(base64Data,dataIndex);
  // Serial.printf("length = %d\r\n", base64Data_String->length());
  //// 测试用
  // Serial.println(base64Data);
  // Serial.println(*base64Data_String);

  ESP_LOGI(TAG, "The base64 encoding succeed, length=%d", strlen(base64Data));
  return base64Data;
}
