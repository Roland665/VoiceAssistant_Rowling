#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>

typedef struct{
  char **hots;
  uint16_t num;
  uint16_t size;
}HotsList;


class API_online{
protected:
  HTTPClient m_httpClient;
  String m_apiToken;// 在线API的token
  String m_tokenUrl1 = "https://iam.";// 获取token的http常量第一段
  String m_tokenUrl2 = ".myhuaweicloud.com/v3/auth/tokens";// 获取token的http常量第二段
  String m_apiUrl1 = "https://sis-ext."; // 一句话识别的http常量第一段
  String m_apiUrl2 = ".myhuaweicloud.com/v1/"; // 一句话识别的http常量第二段
  String m_apiUrl3 = "/asr/short-audio"; // 一句话识别的http常量第三段
  String m_hotUrl3 = "/asr/vocabularies/5104cd99-a89a-4dce-8d81-8e952da21cd8"; //热词表查询/更新的http常量第三段，一二段和apiUrl一致
  String m_hotCreateUrl3 = "/asr/vocabularies"; //热词表查询/更新的http常量第三段，一二段和apiUrl一致
  String m_endpoint; // API服务器结点
  String m_project_id; // 项目ID

public:
  HotsList m_hotList =  { .hots=NULL,
                          .num=0,
                          .size=0
                        }; // 热词表
  API_online(void);
  API_online(String endpoint, String project_id);
  int8_t start();// 尝试访问API链接，并获取token
  int16_t sis(const char *base64Data, String *sis_payload);
  int8_t createHotList(void);
  int8_t queryHotList(void);
  int8_t updateHotList(void);
  int8_t hAddData(String data);
  int8_t hReduceData(const char *data);
  // int8_t getToken(void);

};

