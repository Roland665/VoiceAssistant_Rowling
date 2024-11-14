
/************************
 * 本SD卡管理库基于SPI-SD模块，仅在Micro-SD模块上试验过并成功
 */

#include "sd_card.h"
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

static const char *TAG = "sd_card";

/**
 * @brief    sd卡操作类构造函数
 * @param    mount_point : 挂载点，请以“/”开头
 * @param    bus_cfg     : SPI总线配置信息
 * @param    spi_cs_num  : SD卡模块片选引脚号
 */
sd_card::sd_card(const char *mount_point, spi_bus_config_t bus_cfg, gpio_num_t spi_cs_num)
{
  esp_err_t err;
  // 值拷贝挂载点地址
  m_mount_point = (char *)malloc(sizeof(char) * strlen(mount_point) + 1);
  strcpy(m_mount_point, mount_point);

  // 挂载外部存储的配置信息
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false, // 挂载失败时禁止格式化卡
      .max_files = 5,
      .allocation_unit_size = 16 * 1024};

  ESP_LOGI(TAG, "Using SPI peripheral to initialize SD card");
  ESP_ERROR_CHECK(spi_bus_initialize((spi_host_device_t)m_host.slot, &bus_cfg, SDSPI_DEFAULT_DMA)); // 初始化spi总线

  // 挂载卡
  ESP_LOGI(TAG, "Mounting filesystem");
  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT(); // 获取默认的 SDSPI 设备的配置信息
  slot_config.gpio_cs = spi_cs_num;
  slot_config.host_id = (spi_host_device_t)m_host.slot; // 设置主机 ID
  while (1)
  {
    // 获取在 VFS 中注册的 SD 卡的 FAT 文件系统
    err = esp_vfs_fat_sdspi_mount(mount_point, &m_host, &slot_config, &mount_config, &m_card);
    if (err == ESP_OK)
    {
      ESP_LOGI(TAG, "Filesystem mounted");
      break;
    }
    else if (err == ESP_ERR_INVALID_STATE)
    {
      ESP_LOGW(TAG, "SD card was already mount!");
      break;
    }
    else if (err == ESP_ERR_NO_MEM)
    {
      ESP_LOGE(TAG, "Memory can not be allocated for vfs-fat! Program is dangerous!");
      break;
    }
    else if (err == ESP_FAIL)
    {
      ESP_LOGE(TAG, "Please check that the sd card is installed correctly!");
      vTaskDelay(1000);
    }
    else
    {
      ESP_LOGE(TAG, "Mount failed due to an unknown error...qwq");
      vTaskDelay(1000);
    }
  }

  // 打印卡的属性
  sdmmc_card_print_info(stdout, m_card);
}

/**
 * @brief    请在文件操作的行为前调用此函数
 * @param    path : 文件名，可以以“/”开头，也可以单单只写文件名称
 * @param    type : 文件操作类型，符合 c/c++ 的 fopen 函数第二个传参即可 如 "r","w","a"...
 * @retval   0-正常开始文件操作
 */
rowl_err_t sd_card::start(const char *path, const char *type)
{
  // 打开文件
  ESP_LOGI(TAG, "Start file control");
  char *whole_path = NULL;
  if (path[0] != '/')
  {
    whole_path = (char *)malloc(sizeof(char) * (1 + strlen(m_mount_point) + strlen(path) + 1));
    strcpy(whole_path, m_mount_point);
    strcat(whole_path, "/");
    strcat(whole_path, path);
  }
  else
  {
    whole_path = (char *)malloc(sizeof(char) * (strlen(m_mount_point) + strlen(path) + 1));
    strcpy(whole_path, m_mount_point);
    strcat(whole_path, path);
  }
  m_file = fopen(whole_path, type);
  // assert(m_file);
  if (m_file == NULL)
  {
    ESP_LOGE(TAG, "File didn`t in system, whole_path=%s", whole_path);
    free(whole_path);
    return 1;
  }
  if(whole_path != NULL)
    free(whole_path);

  return 0;
}

/**
 * @brief    向文件内写入数据，调用此方法前必须执行 start 方法并且第二个传参不能是 "r"
 * @param    buf       : 待写数据缓冲区
 * @param    data_size : 待写数据的单数据大小(单位byte)
 * @param    number    : 待写数据量
 * @retval   0-成功写入，1-指定数据量未全写入
 */
rowl_err_t sd_card::write(const void *buf, size_t data_size, size_t number)
{
  // 写数据
  size_t count = fwrite(buf, data_size, number, m_file);
  if (count == number)
    return 0;
  else
  {
    ESP_LOGE(TAG, "Didn`t writed all number of buffer! Just writed %d/%d", count, number);
    return 1;
  }
}

/**
 * @brief    读取文件数据，调用此方法前必须执行 start 方法并且第二个传参不能是 "w"、"a"
 * @param    buf       : 读出数据存放缓冲
 * @param    data_size : 读出数据的单数据大小(单位byte)
 * @param    number    : 目标读出数据量
 * @retval   0-成功读取， 1-未读出指定数据量， 2-文件操作指针为空(即未提前调用start方法)
 */
rowl_err_t sd_card::read(void *buf, size_t data_size, size_t number)
{
  if (m_file == NULL)
    return 2;
  // 读数据
  size_t count = fread(buf, data_size, number, m_file);
  if (count == number)
    return 0;
  else
  {
    ESP_LOGE(TAG, "Didn`t read all number of buffer! Just read %d/%d", count, number);
    return 1;
  }
}

/**
 * @brief    读取文件数据，调用此方法前必须执行 start 方法并且第二个传参不能是 "w"、"a"
 * @param    buf       : 读出数据存放缓冲
 * @param    data_size : 读出数据的单数据大小(单位byte)
 * @param    number    : 目标读出数据量
 * @param    count     : 实际读出数据量，可以传入NULL，表示对此数据无需求
 * @retval   0-成功读取， 1-未读出指定数据量， 2-文件操作指针为空(即未提前调用start方法)
 */
rowl_err_t sd_card::read(void *buf, size_t data_size, size_t number, size_t *count)
{
  if (m_file == NULL)
    return 2;
  // 读数据
  if (count == NULL)
    return read(buf, data_size, number);

  *count = fread(buf, data_size, number, m_file);
  if (*count == number)
    return 0;
  else
  {
    ESP_LOGE(TAG, "Didn`t read all number of buffer! Just read %d/%d", *count, number);
    return 1;
  }
}

/**
 * @brief  把fseek函数封装进类，使用方法参考c/c++ fseek函数，调用此方法前必须执行 start
 * @retval 0-成功执行 2-文件操作指针为空(即未提前调用start方法)
 */
rowl_err_t sd_card::seek(long pos, int mode)
{
  if (m_file == NULL)
    return 2;
  fseek(m_file, pos, mode);
  return 0;
}

/**
 * @brief    结束本次文件操作
 * @param    void
 * @retval   0-成功结束
 */
rowl_err_t sd_card::stop(void)
{
  ESP_LOGI(TAG, "Stoping file control");
  fclose(m_file);
  m_file = NULL;
  return 0;
}

/**
 * @brief    将指定文件进行Base64编码
 * @param    path : 文件名，可以以“/”开头，也可以单单只写文件名称
 * @retval   编码后数据存放区首地址指针
 */
char *sd_card::enBase64(const char *path)
{
  struct stat file_stat;
  const char *to_base64_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; // 64位编码表
  // 打开文件
  char *whole_path;
  if (path[0] != '/')
  {
    whole_path = (char *)malloc(sizeof(char) * (1 + strlen(m_mount_point) + strlen(path) + 1));
    strcpy(whole_path, m_mount_point);
    strcat(whole_path, "/");
    strcat(whole_path, path);
  }
  else
  {
    whole_path = (char *)malloc(sizeof(char) * (strlen(m_mount_point) + strlen(path) + 1));
    strcpy(whole_path, m_mount_point);
    strcat(whole_path, path);
  }
  m_file = fopen(whole_path, "r");
  if (m_file == NULL)
  {
    ESP_LOGE(TAG, "The file to be encoded does not exist");
    return NULL;
  }

  // 获取文件大小并打印
  stat(whole_path, &file_stat); // 获取文件所有信息
  size_t file_size = file_stat.st_size;
  ESP_LOGI(TAG, "File:%s  Size: %d", path, file_size);

  // 计算文件大小与3byte的最大公倍数之差
  uint8_t size_err = 0;
  if (file_size % 3 != 0)
  {
    size_err = 3 - file_size % 3;
  }

  uint8_t fileDatas[3]; // 文件读取临时存放区

  // 开空间存放base64格式的数据
  char *base64_data = (char *)malloc(sizeof(char) * (4 * (file_size + size_err) / 3 + 1));
  if (base64_data == NULL)
  {
    ESP_LOGE(TAG, "Insufficient heap space, *base64_data* create failed");
    return NULL;
  }
  // 开始base64编码
  int dataIndex = 0;
  int i = 0;
  for (; i < file_size / 3; i++)
  {
    read(fileDatas, sizeof(uint8_t), 3);
    base64_data[dataIndex++] = to_base64_table[(fileDatas[0] & 0xFC) >> 2];                                  // 取第一字节的前6bit
    base64_data[dataIndex++] = to_base64_table[((fileDatas[0] & 0x03) << 4) + ((fileDatas[1] & 0xF0) >> 4)]; // 取第一字节的后2bit和第二字节的前4bit
    base64_data[dataIndex++] = to_base64_table[((fileDatas[1] & 0x0F) << 2) + ((fileDatas[2] & 0xC0) >> 6)]; // 取第二字节的后4bit和第三字节的前2bit
    base64_data[dataIndex++] = to_base64_table[(fileDatas[2] & 0x3F)];                                       // 取第三字节的后6bit
  }
  if (size_err == 2)
  {
    read(fileDatas, sizeof(uint8_t), 1);
    base64_data[dataIndex++] = to_base64_table[(fileDatas[0] & 0xFC) >> 2]; // 取第一字节的前6bit
    base64_data[dataIndex++] = to_base64_table[(fileDatas[0] & 0x03) << 4]; // 取第一字节的后2bit和补零
    base64_data[dataIndex++] = '=';                                         // 补零
    base64_data[dataIndex++] = '=';                                         // 补零
  }
  else if (size_err == 1)
  {
    read(fileDatas, sizeof(uint8_t), 2);
    base64_data[dataIndex++] = to_base64_table[(fileDatas[0] & 0xFC) >> 2];                                  // 取第一字节的前6bit
    base64_data[dataIndex++] = to_base64_table[((fileDatas[0] & 0x03) << 4) + ((fileDatas[1] & 0xF0) >> 4)]; // 取第一字节的后2bit和第二字节的前4bit
    base64_data[dataIndex++] = to_base64_table[(fileDatas[1] & 0x0F) << 2];                                  // 取第二字节的后4bit和补零
    base64_data[dataIndex++] = '=';                                                                          // 补零
  }
  base64_data[dataIndex] = 0; // 最后一位补停止位
  ESP_LOGI(TAG, "The base64 encoding succeed, length=%d", strlen(base64_data));
  return base64_data;
}

/**
 * @brief 析构函数，释放此类时解挂sd卡
 */
sd_card::~sd_card()
{
  // 解除sd卡挂载
  ESP_ERROR_CHECK(esp_vfs_fat_sdcard_unmount(m_mount_point, m_card));
  ESP_ERROR_CHECK(spi_bus_free((spi_host_device_t)m_host.slot));
  free(m_mount_point);
  m_mount_point = NULL;
  ESP_LOGI(TAG, "Card unmounted");
}
