#include "API_online.h"
#include "ArduinoJson.h"

static const char* TAG = "HUAWEI-SIS";

API_online::API_online(String wifi_ssid, String wifi_password, String tokenUrl, String sisUrl)
  :m_wifi_ssid(wifi_ssid), m_wifi_password(wifi_password), m_tokenUrl(tokenUrl), m_sisUrl(sisUrl){};


/**
 * @brief    获取token
 * @param    void
 * @retval   错误信息，0表示无错误
 */
uint8_t API_online::getToken(){
  int httpErr;
  if (WiFi.status() == WL_CONNECTED){
    //记录将要收集的响应头(返回的token在X-Subject-Token中)
		const char* headers[] = {"X-Subject-Token"};
		m_httpClient.collectHeaders(headers,2);
		// 建立http链接
		m_httpClient.begin(m_tokenUrl);
    //// 添加请求头
		// m_httpClient.addHeader("Content-Type", "application/json");//似乎可有可无
    // 发送POST请求
		httpErr = m_httpClient.POST("{\"auth\": {\"identity\": {\"methods\": [\"password\"],\"password\": {\"user\": {\"name\": \"Roland\",\"password\": \"roland66\",\"domain\": {\"name\": \"hw098127479\"}}}},\"scope\": {\"project\": {\"name\": \"cn-north-4\"}}}}");
		if(httpErr/100 == 2){
      m_apiToken = m_httpClient.header("X-Subject-Token");
      ESP_LOGI(TAG, "Get token successfully");
    }
    else{
      ESP_LOGE(TAG, "POST ERROR. Http code is %d", httpErr);
      return 2;
    }
	}
	else{
		Serial.println("Error in WiFi connection");
    return 1;
	}
	// 结束http连接
	m_httpClient.end();
  return 0;
}

/**
 * @brief    为调用华为一句话识别API（SIS）做准备，具体是连接WiFi、获取token
 * @param    void
 * @retval   void
 */
void API_online::start(){
  uint8_t err;
  WiFi.begin(m_wifi_ssid, m_wifi_password);
  while(WiFi.status() != WL_CONNECTED){
    ESP_LOGI(TAG, "Waiting to link wifi: %S", m_wifi_ssid);
    vTaskDelay(200);
  }
  // 获取华为一句话识别token
  // err = this->getToken();
  // if(err != 0){
  //   ESP_LOGE(TAG, "Method \"getToken\" happened error");
  //   while(1);
  // }

  if (WiFi.status() == WL_CONNECTED){
    //记录将要收集的响应头(返回的token在X-Subject-Token中)
		const char* headers[] = {"X-Subject-Token"};
		m_httpClient.collectHeaders(headers,2);
		// 建立http链接
		m_httpClient.begin(m_tokenUrl);
    //// 添加请求头
		// m_httpClient.addHeader("Content-Type", "application/json");//似乎可有可无
    // 发送POST请求
		m_httpClient.POST("{\"auth\": {\"identity\": {\"methods\": [\"password\"],\"password\": {\"user\": {\"name\": \"Roland\",\"password\": \"roland66\",\"domain\": {\"name\": \"hw098127479\"}}}},\"scope\": {\"project\": {\"name\": \"cn-north-4\"}}}}");
		m_apiToken = m_httpClient.header("X-Subject-Token");
	}
	else{
		ESP_LOGE(TAG, "Error in WiFi connection");
	}

}

/**
 * @brief    调用华为一句话识别API（SIS）
 * @param    base64Data: wav文件base64编码数据
 * @param    length    : base64Data的长度
 * @retval   识别后的字符串
 */
String API_online::sis(const char *base64Data){
  uint32_t length = strlen(base64Data);
  char *jsonA = "{\"config\": {\"audio_format\": \"wav\",\"property\": \"chinese_8k_common\",\"add_punc\": \"yes\"},\"data\": \"";
  char *jsonB = "\"}";
  // 先判断是否有联网
  if (WiFi.status() == WL_CONNECTED){
    char *POST_body = (char*)malloc(sizeof(char) * (strlen(jsonA)+strlen(base64Data)+strlen(jsonB)+1));
    if(POST_body == NULL){
      ESP_LOGE(TAG, "Insufficient heap space, *POST_body* create failed");
    }
    // Serial.println(1);
    // DynamicJsonDocument json_doc(3*1024*1024); // 申明一个大小为2K的DynamicJsonDocument对象JSON_Buffer,用于存储反序列化后的（即由字符串转换成JSON格式的）JSON报文，方式声明的对象将存储在堆内存中，推荐size大于1K时使用该方式
    // JsonObject configObj; // 第一层级节点，api的配置数据
    // JsonObject dataObj;// 第一层级节点，存放音频数据
    // Serial.println(2);
    // configObj = json_doc.createNestedObject("config");// 创建第二层级的主节点
    // json_doc["config"]["audio_format"] = "wav";
    // json_doc["config"]["property"] = "chinese_8k_common";
    // json_doc["config"]["add_punc"] = "yes";
    // Serial.println(3);
    // json_doc["data"] = base64Data; // 音频数据
    // // Serial.printf("base64Data=====%s\r\n", base64Data);
    // Serial.println(4);
    // serializeJsonPretty(json_doc, POST_body);
    strcpy(POST_body, jsonA);
    strcat(POST_body, base64Data);
    strcat(POST_body, jsonB);
    // Serial.println(POST_body);
		// 建立http链接
		m_httpClient.begin(m_sisUrl);
    //// 添加请求头
		m_httpClient.addHeader("Content-Type", "application/json");// 似乎可有可无
		m_httpClient.addHeader("X-Auth-Token", m_apiToken);
    // 发送POST请求
		m_httpClient.POST((uint8_t*)POST_body, strlen(POST_body));
    return m_httpClient.getString();
	}
	else{
		ESP_LOGE(TAG, "Error in WiFi connection");
    return "";
	}
}
