#pragma once
#include <Arduino.h>
#include "config.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

typedef uint8_t rowl_err_t;

class sd_card
{
private:
  char *m_mount_point;
  FILE *m_file = NULL;
  sdmmc_host_t m_host = SDSPI_HOST_DEFAULT(); // 获取默认的 SDSPI 主机的属性和信息
  sdmmc_card_t *m_card; // TF卡信息句柄
public:
  sd_card(const char *mount_point, spi_bus_config_t bus_cfg, gpio_num_t spi_cs_num);
  rowl_err_t start(const char* path, const char *type);
  rowl_err_t write(const void *buf, size_t data_size, size_t number);
  rowl_err_t read(void *buf, size_t data_size, size_t number);
  rowl_err_t read(void *buf, size_t data_size, size_t number, size_t *count);
  rowl_err_t seek(long pos, int mode);
  char * enBase64(const char* path);
  rowl_err_t stop(void);
  ~sd_card();
};
