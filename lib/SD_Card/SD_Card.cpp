#include "SD_Card.h"


/**
 * @brief    获取SD卡内目录细节
 * @param    fs       : 文件操作对象，只能是SD库中的SD对象
 * @param    dirname  : 目录地址
 * @param    levels   : 细节深度
 * @retval   void
 */
void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root){
    Serial.println("Failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if(levels){
        listDir(fs, file.name(), levels -1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

/**
 * @brief    新建子目录（新建文件夹）
 * @param    fs       : 文件操作对象，只能是SD库中的SD对象
 * @param    path     : 文件夹所在路径
 * @retval   void
 */
void createDir(fs::FS &fs, const char * path){
  Serial.printf("Creating Dir: %s\n", path);
  if(fs.mkdir(path)){
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}


/**
 * @brief    删除子目录（删除文件夹）
 * @param    fs       : 文件操作对象，只能是SD库中的SD对象
 * @param    path     : 文件夹所在路径
 * @retval   void
 */
void removeDir(fs::FS &fs, const char * path){
  Serial.printf("Removing Dir: %s\n", path);
  if(fs.rmdir(path)){
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}


/**
 * @brief    读取文件内容
 * @param    fs       : 文件操作对象，只能是SD库中的SD对象
 * @param    path     : 文件路径
 * @retval   void
 */
void readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while(file.available()){
    Serial.write(file.read());
  }
  file.close();
}


/**
 * @brief    写文件内容(先清空后写入)
 * @param    fs       : 文件操作对象，只能是SD库中的SD对象
 * @param    path     : 文件路径
 * @param    message  : 写入内容
 * @retval   void
 */
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}


/**
 * @brief    写文件内容(追加式写入)
 * @param    fs       : 文件操作对象，只能是SD库中的SD对象
 * @param    path     : 文件路径
 * @param    message  : 写入内容
 * @retval   void
 */
void appendFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)){
      Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}


/**
 * @brief    重命名文件
 * @param    fs        : 文件操作对象，只能是SD库中的SD对象
 * @param    path1     : 文件原路径
 * @param    path2     : 文件新路径
 * @retval   void
 */
void renameFile(fs::FS &fs, const char * path1, const char * path2){
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}


/**
 * @brief    删除文件
 * @param    fs        : 文件操作对象，只能是SD库中的SD对象
 * @param    path      : 文件路径
 * @retval   void
 */
void deleteFile(fs::FS &fs, const char * path){
  Serial.printf("Deleting file: %s\n", path);
  if(fs.remove(path)){
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

//意义不明
void testFileIO(fs::FS &fs, const char * path){
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if(file){
    len = file.size();
    size_t flen = len;
    start = millis();
    while(len){
      size_t toRead = len;
      if(toRead > 512){
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %u ms\n", flen, end);
    file.close();
  } else {
    Serial.println("Failed to open file for reading");
  }


  file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for(i=0; i<2048; i++){
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();
}