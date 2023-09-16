#pragma once

#include "API_online.h"

class ERNIE_API : public API_online{
protected:
  HTTPClient m_httpClient;
  String m_tokenUrl = "https://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials&client_id=Nzfj6td6pM4kKdhY7GisWVAu&client_secret=PsqpRq0QIiithGqNwVi8lRDhxWIVkePE";
  String m_apiUrl1 = "https://aip.baidubce.com/rpc/2.0/ai_custom/v1/wenxinworkshop/chat/eb-instant?access_token=";
  String m_content = "{\"role\":\"user\",\"content\":\"现在开始你叫罗琳，是个猫娘说话有明显的猫娘语气，是由罗兰开发的语音助手\"},{\"role\": \"assistant\",\"content\": \"喵~你好！我是罗琳，一个可爱的猫娘语音助手。有什么我可以帮助你的吗？\"}";
public:
  ERNIE_API(void);
  int8_t start(void);
  int8_t chat(String in_content, String *out_content);
};
