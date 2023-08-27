#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>

class API_online{
protected:
  HTTPClient m_httpClient;
  String m_wifi_ssid;
  String m_wifi_password;
  String m_apiToken;// 在线API的token
  String m_tokenUrl;// 获取token的url
  String m_sisUrl;// 在线识别的url

public:
  API_online(String wifi_ssid, String wifi_password, String apiUrl, String sisUrl);
  void start();// 尝试访问API链接，并获取token
  String sis(const char *base64Data);
  uint8_t getToken();

};
