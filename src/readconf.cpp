#include "main.h"
#include "commonexternal.h"

extern fileConfig config;

/** @brief Чтение конфигурации из файла, если он существует.
 * Если файла не существует - загружается дефолтовая конфигурация
 * @param filename Имя файла конфигурации
 */
void readConf(char *_filename)
{
  File _fl;
  char _gpssend[] = "true";
  char _wifisend[] = "true";
  char _ftpbeep[] = "false";

  strlcpy(config.ssid, WIFI_SSID, sizeof(config.ssid));
  strlcpy(config.wifiPasswd, WIFI_PASS, sizeof(config.wifiPasswd));
  strlcpy(config.ftpAddress, FTP_ADDRESS, sizeof(config.ftpAddress));
  strlcpy(config.ftpUser, FTP_USER, sizeof(config.ftpUser));
  strlcpy(config.ftpPasswd, FTP_PASSWD, sizeof(config.ftpPasswd));
  config.filesize = CSV_MAX_FILESIZE;
  config.gpssend = true;
  config.wifisend = true;
  config.ftpbeep = false;
  config.ftpPort = FTP_CMD_PORT;
  strlcpy(config.moduleID, "00", sizeof(config.moduleID));
  config.pDopMask = 0;
  config.pAccMask = 0;

  if (SD.exists(_filename))
  {
    logging("Opening the configuration file\n", false);
    _fl = SD.open(_filename, "r");
    StaticJsonDocument<JSON_OBJ_SIZE> doc; // https://arduinojson.org/v6/assistant/
    DeserializationError error = deserializeJson(doc, _fl);
    if (!error)
    {
      strlcpy(config.ssid, doc["ssid"] | "NULL", sizeof(config.ssid));
      strlcpy(config.wifiPasswd, doc["wifipasswd"] | "NULL", sizeof(config.wifiPasswd));
      strlcpy(config.ftpAddress, doc["ftpaddress"] | "NULL", sizeof(config.ftpAddress));
      strlcpy(config.ftpUser, doc["ftpuser"] | "NULL", sizeof(config.ftpUser));
      strlcpy(config.ftpPasswd, doc["ftppassword"] | "NULL", sizeof(config.ftpPasswd));
      config.ftpPort = doc["ftpport"] | FTP_CMD_PORT;
      config.filesize = doc["filesize"] | CSV_MAX_FILESIZE;
      strlcpy(_gpssend, doc["gpssend"] | "true", sizeof(_gpssend));
      strlcpy(_wifisend, doc["wifisend"] | "true", sizeof(_wifisend));
      strlcpy(_ftpbeep, doc["ftpbeep"] | "false", sizeof(_ftpbeep));
      strlcpy(config.moduleID, doc["moduleid"] | "00", sizeof(config.moduleID));
      config.pDopMask = doc["pdopmask"] | 0;
      config.pAccMask = doc["paccmask"] | 0;

      if ((_gpssend[0] == 't') || (_gpssend[0] == 'y'))
      {
        config.gpssend = true;
      }
      else
      {
        config.gpssend = false;
      }

      if ((_wifisend[0] == 't') || (_wifisend[0] == 'y'))
      {
        config.wifisend = true;
      }
      else
      {
        config.wifisend = false;
      }

      if ((_ftpbeep[0] == 't') || (_ftpbeep[0] == 'y'))
      {
        config.ftpbeep = true;
      }
      else
      {
        config.ftpbeep = false;
      }

      if ((config.wifiPasswd[0] == 'N') || (config.ssid[0] == 'N'))
      {
        strlcpy(config.ssid, WIFI_SSID, sizeof(config.ssid));
        strlcpy(config.wifiPasswd, WIFI_PASS, sizeof(config.wifiPasswd));
      }

      if (config.ftpAddress[0] == 'N')
      {
        strlcpy(config.ftpAddress, FTP_ADDRESS, sizeof(config.ftpAddress));
      }

      if ((config.ftpUser[0] == 'N') || config.ftpPasswd[0] == 'N')
      {
        strlcpy(config.ftpUser, FTP_USER, sizeof(config.ftpUser));
        strlcpy(config.ftpPasswd, FTP_PASSWD, sizeof(config.ftpPasswd));
        config.ftpPort = FTP_CMD_PORT;
        logging("Configuration loaded\n", false);
      }
    }
    else
    {
      logging("Configuration load error\n", false);
    }
    _fl.close();
  }
  else
  {
    logging("No config file, load default config\n", false);
  }
} //readConf
