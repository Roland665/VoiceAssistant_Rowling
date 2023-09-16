#include "ERNIE-Bot.h"
#include <ArduinoJson.h>

static const char* TAG = "ERNIE-Bot";
// 调用ERNIE-Bot-torbo模型
ERNIE_API::ERNIE_API(void){

}

/**
 * @brief    为调用千帆语言模型做准备，具体是获取token
 * @param    void
 * @retval   错误状态, 0-正常返回
 */
int8_t ERNIE_API::start(void){
  if (WiFi.status() == WL_CONNECTED){
		// 建立http链接
		m_httpClient.begin(m_tokenUrl);
    //// 添加请求头
		m_httpClient.addHeader("Content-Type", "application/json");
    // 发送POST请求
		m_httpClient.POST("");
    String payload = m_httpClient.getString();
    // Serial.println(payload);
    // 提取json数据
    DynamicJsonDocument doc(payload.length()*2); //声明一个JsonDocument对象
    deserializeJson(doc, payload); //反序列化JSON数据
    m_apiToken = doc["access_token"].as<String>();
    m_httpClient.end();
    return 0;
	}
	else{
		ESP_LOGE(TAG, "Error in WiFi connection");
    return 1;
	}
}


/**
 * @brief    调用千帆语言模型
 * @param    void
 * @retval   错误状态, 0-正常返回，1-没网，2-网络连接不畅对话失败
 */
int8_t ERNIE_API::chat(String in_content, String *out_content){
  if (WiFi.status() == WL_CONNECTED){
    int httpcode;
    String jsonA = "{\"messages\": [";
    String jsonB = "]}";
    // 添加对话内容
    m_content += ",{\"role\": \"user\",\"content\": \"";
    m_content += in_content;
    m_content += "\"}";
		// 建立http链接
		m_httpClient.begin(m_apiUrl1+m_apiToken);
    // 添加请求头
		m_httpClient.addHeader("Content-Type", "application/json");
    // 发送POST请求
    ESP_LOGD(TAG, "jsonA+m_content+jsonB = %s", (jsonA+m_content+jsonB).c_str());
		httpcode = m_httpClient.POST(jsonA+m_content+jsonB);
    String payload = m_httpClient.getString();
    m_httpClient.end();
    Serial.println(payload); //test
    // 提取json数据
    DynamicJsonDocument doc(payload.length()*2); //声明一个JsonDocument对象
    deserializeJson(doc, payload); //反序列化JSON数据
    *out_content = doc["result"].as<String>();
    // 添加对话内容
    if(*out_content != "null"){
      m_content += ",{\"role\": \"assistant\",\"content\": \"";
      m_content += *out_content;
      m_content += "\"}";
      return 0;
    }
    else{
      m_content += ",{\"role\": \"assistant\",\"content\": \"";
      m_content += "网络连接不良，请再说一次";
      m_content += "\"}";
      if(httpcode == -2){
        return -2;
      }
      return 2;
    }
	}
	else{
		ESP_LOGE(TAG, "Error in WiFi connection");
    return 1;
	}
}
