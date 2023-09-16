#include "API_online.h"
#include "ArduinoJson.h"

static const char* TAG = "API_online";
API_online::API_online(void){}

API_online::API_online(String endpoint, String project_id)
  :m_endpoint(endpoint), m_project_id(project_id){};


// /**
//  * @brief    获取token
//  * @param    void
//  * @retval   错误信息，0表示无错误
//  */
// uint8_t API_online::getToken(){
// }

/**
 * @brief    为调用华为一句话识别API（SIS）做准备，具体是连接WiFi、获取token
 * @param    void
 * @retval   void
 */
int8_t API_online::start(){
  if (WiFi.status() == WL_CONNECTED){
    //记录将要收集的响应头(返回的token在X-Subject-Token中)
		const char* headers[] = {"X-Subject-Token"};
		m_httpClient.collectHeaders(headers,2);
		// 建立http链接
		m_httpClient.begin(m_tokenUrl1+m_endpoint+m_tokenUrl2);
    //// 添加请求头
		// m_httpClient.addHeader("Content-Type", "application/json"); // 可有可无
    // 发送POST请求
		m_httpClient.POST("{\"auth\": {\"identity\": {\"methods\": [\"password\"],\"password\": {\"user\": {\"name\": \"Roland\",\"password\": \"roland66\",\"domain\": {\"name\": \"hw098127479\"}}}},\"scope\": {\"project\": {\"name\": \"cn-north-4\"}}}}");
		m_apiToken = m_httpClient.header("X-Subject-Token");
    m_httpClient.end();
    return 0;
	}
	else{
		ESP_LOGE(TAG, "Error in WiFi connection");
    return 1;
	}

}

/**
 * @brief    调用华为一句话识别API（SIS）
 * @param    base64Data: wav文件base64编码数据
 * @param    sis_payload    : 识别结果
 * @retval   0-识别成功 1-没联网 2-内存不足 3-没检测到语音
 */
int16_t API_online::sis(const char *base64Data, String *sis_payload){
  // 先判断是否有联网
  if (WiFi.status() == WL_CONNECTED){
    int httpcode;
    uint32_t length = strlen(base64Data);
    // 固化json数据段
    char *jsonA = "{\"config\": {\"audio_format\": \"wav\",\"property\": \"chinese_16k_common\",\"add_punc\": \"yes\",\"vocabulary_id\": \"5104cd99-a89a-4dce-8d81-8e952da21cd8\"},\"data\": \"";
    char *jsonB = "\"}";
    char *POST_body = (char*)malloc(sizeof(char) * (strlen(jsonA)+strlen(base64Data)+strlen(jsonB)+1));
    if(POST_body == NULL){
      ESP_LOGE(TAG, "Insufficient heap space, *POST_body* create failed");
      return 2;
    }
    strcpy(POST_body, jsonA);
    strcat(POST_body, base64Data);
    strcat(POST_body, jsonB);
    // Serial.println(POST_body);
		// 建立http链接
		m_httpClient.begin(m_apiUrl1 + m_endpoint + m_apiUrl2 + m_project_id + m_apiUrl3);
    //// 添加请求头
		m_httpClient.addHeader("Content-Type", "application/json");// 不可无
		m_httpClient.addHeader("X-Auth-Token", m_apiToken);
    // 发送POST请求
		httpcode = m_httpClient.POST((uint8_t*)POST_body, strlen(POST_body));
    free(POST_body);
    String payload = m_httpClient.getString();
    Serial.println(payload);
    if(httpcode == 200){ // 成功识别
      // 提取响应体
      ESP_LOGI(TAG, "POST success, information:");
      DynamicJsonDocument doc(payload.length()*2); //声明一个JsonDocument对象
      deserializeJson(doc, payload); //反序列化JSON数据
      m_httpClient.end();
      *sis_payload = doc["result"]["text"].as<String>();
      if(doc["result"]["text"].as<String>().length() <= 1)
        return 3;
      return 0;
    }
    else
      return httpcode; // API调用了但是不成功，具体原因根据返回值到官网查看
	}
	else{
		ESP_LOGE(TAG, "Error in WiFi connection");
    return 1;
	}
}

// TODO下面这个函数没验证过,PUT没法用？？
/**
 * @brief   将本地热词表更新至云端
 * @retval  0-成功更新，1-未联网
 */
int8_t API_online::updateHotList(void){
  if (WiFi.status() == WL_CONNECTED){
    // 固化json数据段
    String jsonA = "{\"name\":\"demo\",\"language\":\"chinese_mandarin\",\"contents\":[";
    String jsonB = "]}";
    // char *PUT_body = (char*)malloc(sizeof(char) * (strlen(jsonA)+m_hotList.size+strlen(jsonB)+1));
    String PUT_body;
    // if(PUT_body == NULL){
    //   ESP_LOGE(TAG, "Insufficient heap space, *POST_body* create failed");
    //   return 2;
    // }
    // 合并json数据段
    PUT_body = jsonA;
    // strcpy(PUT_body, jsonA);
    // Serial.println(PUT_body);
    uint16_t i = 0;
    // 将内存中所有热词封装入json
    while(m_hotList.num){
      // strcat(PUT_body, "\"");
      PUT_body+="\"";
      if(i<m_hotList.num-1){
        // strcat(PUT_body, m_hotList.hots[i]);
        PUT_body+=m_hotList.hots[i];
        // strcat(PUT_body, "\",");
        PUT_body+="\",";
        // Serial.println(PUT_body);
      }
      else{
        // strcat(PUT_body, m_hotList.hots[i]);
        PUT_body+=m_hotList.hots[i];
        // strcat(PUT_body, "\"");
        PUT_body+="\"";
        // Serial.println(PUT_body);
        break;
      }
      i++;
    }
    // strcat(PUT_body, jsonB);
    PUT_body+=jsonB;
    Serial.printf("%s length=%d\n", PUT_body.c_str(), PUT_body.length());
    Serial.print("Free heap before allocation: ");
    Serial.println(ESP.getFreeHeap());
    Serial.print("Free PSRAM before allocation: ");
    Serial.println(heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    // 发送PUT请求
    m_httpClient.begin(m_apiUrl1+m_endpoint+m_apiUrl2+m_project_id+m_hotUrl3);
		m_httpClient.addHeader("Content-Type", "application/json");// 不可无
    m_httpClient.addHeader("X-Auth-Token", m_apiToken);
    // for(int i = 0 ; i < PUT_body.length(); i++){
    //   printf("%hX ", PUT_body.c_str()[i]);
    // }
    // Serial.println("===");
    uint8_t *p = (uint8_t *)PUT_body.c_str();
    Serial.printf("size=%d\n", PUT_body.length());
    for(int i = 0 ; i < PUT_body.length(); i++){
      Serial.printf("%hX ", p[i]);
    }
    // m_httpClient.PUT(PUT_body);
    m_httpClient.PUT(p, PUT_body.length());
    // 提取响应体
    String payload = m_httpClient.getString();
    Serial.println(payload);
    // TODO 测试到这里，下面先不测
    while (1);
    DynamicJsonDocument doc(payload.length()*2); //声明一个JsonDocument对象
    deserializeJson(doc, payload); //反序列化JSON数据
    String temp = doc["contents"];
    Serial.println(temp);
    while(1);
    // free(PUT_body);
    m_httpClient.end();
	}
	else{
		ESP_LOGE(TAG, "Error in WiFi connection");
    return 1;
	}
}

/**
 * @brief    调用查询热词API保存至本地
 * @retval   0-成功查询，1-未联网，2-内存不足
 */
int8_t API_online::queryHotList(void){
  if (WiFi.status() == WL_CONNECTED){
    m_httpClient.begin(m_apiUrl1+m_endpoint+m_apiUrl2+m_project_id+m_hotUrl3);
    m_httpClient.addHeader("X-Auth-Token", m_apiToken);

    // 发送GET请求
    m_httpClient.GET();
    // 提取响应体
    String payload = m_httpClient.getString();
    Serial.println(payload);
    DynamicJsonDocument doc(payload.length()*2); //声明一个JsonDocument对象
    deserializeJson(doc, payload); //反序列化JSON数据
    // String temp;
    int i = 0;
    // 先计算长度
    while(doc["contents"][i]){
      // temp = doc["contents"][i].as<String>();
      i++;
    }
    free(m_hotList.hots); // 确保调用前释放干净内存
    m_hotList.hots = (char **)malloc(sizeof(char *)*i);
    if(m_hotList.hots == NULL){
      ESP_LOGE(TAG, "Insufficient heap space, *m_hotList.hots* create failed");
      return 2;
    }
    m_hotList.num = i; // 记录长度
    // 将JSON列表数据取出
    for(i = 0; i < m_hotList.num; i++){
      m_hotList.hots[i] = (char *)malloc(sizeof(char)*(doc["contents"][i].as<String>().length()+1));
      m_hotList.size+=doc["contents"][i].as<String>().length();
      strcpy(m_hotList.hots[i], doc["contents"][i].as<String>().c_str());
      // Serial.println(m_hotList.hots[i]);
      // Serial.printf("size=%d", m_hotList.size);
      // Serial.println(doc["contents"][i].as<String>().length());
    }
    m_httpClient.end();
    return 0;
	}
	else{
		ESP_LOGE(TAG, "Error in WiFi connection");
    return 1;
	}
}

/**
 * @brief   将本地热词表更新至云端
 * @retval  0-成功更新，1-未联网
 */
int8_t API_online::createHotList(void){
  if (WiFi.status() == WL_CONNECTED){
    // 固化json数据段
    String jsonA = "{\"name\":\"demo\",\"language\":\"chinese_mandarin\",\"contents\":[";
    // String jsonA = "{\"name\":\"demo\",\"language\":\"chinese_mandarin\"}";
    String jsonB = "]}";
    // char *PUT_body = (char*)malloc(sizeof(char) * (strlen(jsonA)+m_hotList.size+strlen(jsonB)+1));
    String PUT_body;
    // if(PUT_body == NULL){
    //   ESP_LOGE(TAG, "Insufficient heap space, *POST_body* create failed");
    //   return 2;
    // }
    // 合并json数据段
    PUT_body = jsonA;
    // strcpy(PUT_body, jsonA);
    // Serial.println(PUT_body);
    uint16_t i = 0;
    // // 将内存中所有热词封装入json
    // while(m_hotList.num){
    //   // strcat(PUT_body, "\"");
    //   PUT_body+="\"";
    //   if(i<m_hotList.num-1){
    //     // strcat(PUT_body, m_hotList.hots[i]);
    //     PUT_body+=m_hotList.hots[i];
    //     // strcat(PUT_body, "\",");
    //     PUT_body+="\",";
    //     // Serial.println(PUT_body);
    //   }
    //   else{
    //     // strcat(PUT_body, m_hotList.hots[i]);
    //     PUT_body+=m_hotList.hots[i];
    //     // strcat(PUT_body, "\"");
    //     PUT_body+="\"";
    //     // Serial.println(PUT_body);
    //     break;
    //   }
    //   i++;
    // }
    // strcat(PUT_body, jsonB);
    PUT_body+=jsonB;
    Serial.printf("%s length=%d\n", PUT_body.c_str(), PUT_body.length());
    Serial.print("Free heap before allocation: ");
    Serial.println(ESP.getFreeHeap());
    Serial.print("Free PSRAM before allocation: ");
    Serial.println(heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    // 发送PUT请求
    HTTPClient m_httpClient;
    // m_httpClient.begin(m_apiUrl1+m_endpoint+m_apiUrl2+m_project_id+m_hotCreateUrl3);
    m_httpClient.begin("https://sis-ext.cn-north-4.myhuaweicloud.com/v1/9512b326c85747cbade191a38a51691c/asr/vocabularies");
    // m_httpClient.begin("https://iam.cn-north-4.myhuaweicloud.com/v3/auth/tokens");
    Serial.println("begin~");
		m_httpClient.addHeader("Content-Type", "application/json");// 不可无
    m_httpClient.addHeader("X-Auth-Token", m_apiToken);
    Serial.println("addheader~");
    // for(int i = 0 ; i < PUT_body.length(); i++){
    //   printf("%hX ", PUT_body.c_str()[i]);
    // }
    // Serial.println("===");
    uint8_t *p = (uint8_t *)PUT_body.c_str();
    Serial.printf("size=%d\n", PUT_body.length());
    for(int i = 0 ; i < PUT_body.length(); i++){
      Serial.printf("%hX ", p[i]);
    }
    // m_httpClient.PUT(PUT_body);
    // m_httpClient.POST(p, PUT_body.length());
    // m_httpClient.POST("{\"name\": \"demo\",\"language\": \"chinese_mandarin\"}");
    m_httpClient.POST("{\"auth\": {\"identity\": {\"methods\": [\"password\"],\"password\": {\"user\": {\"name\": \"Roland\",\"password\": \"roland66\",\"domain\": {\"name\": \"hw098127479\"}}}},\"scope\": {\"project\": {\"name\": \"cn-north-4\"}}}}");

    Serial.println("post~");
    // 提取响应体
    String payload = m_httpClient.getString();
    Serial.println(payload);
    // TODO 测试到这里，下面先不测
    while (1);
    DynamicJsonDocument doc(payload.length()*2); //声明一个JsonDocument对象
    deserializeJson(doc, payload); //反序列化JSON数据
    String temp = doc["contents"];
    Serial.println(temp);
    while(1);
    // free(PUT_body);
    m_httpClient.end();
    return 0;
	}
	else{
		ESP_LOGE(TAG, "Error in WiFi connection");
    return 1;
	}
}

int8_t API_online::hAddData(String data){
  m_hotList.hots = (char **)realloc(m_hotList.hots, sizeof(char *)*(m_hotList.num+1)); //扩容本地列表
  if(m_hotList.hots == NULL)
    return 1;
  m_hotList.hots[m_hotList.num] = (char *)malloc(sizeof(char)*(data.length()+1)); // 开空间
  if(m_hotList.hots[m_hotList.num] == NULL)
    return 2;
  strcpy(m_hotList.hots[m_hotList.num], data.c_str()); // 拷贝数值
  ESP_LOGI(TAG, "New hot-word: %s", m_hotList.hots[m_hotList.num-1]);
  m_hotList.num++; // 计数+1
  m_hotList.size+=data.length();
  return 0;
}

int8_t API_online::hReduceData(const char *data){
  if(m_hotList.num == 0)
    return 1; // 无法再减
  int i;
  char *temp;
  for(i = 0; i < m_hotList.num; i++){
    if(strcmp(m_hotList.hots[i], data)==0){
      free(m_hotList.hots[i]); // 释放空间
      m_hotList.num--;
      for(int j = i; j < m_hotList.num; j++){
        m_hotList.hots[j] = m_hotList.hots[j+1];
      }
      return 0;
    }
  }
  return 2; // 没有找到目标热词
}
