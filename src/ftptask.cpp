#include "main.h"
#include "commonexternal.h"

extern EventGroupHandle_t eventGroup_1;
extern volatile xSemaphoreHandle sdMutex;
extern volatile xSemaphoreHandle wifiMutex;
extern Folders gpsFlDir;
extern Folders wifiFlDir;
extern fileConfig config;
ESP32_FTPClient _ftp(config.ftpAddress, FTP_CMD_PORT, config.ftpUser, config.ftpPasswd, 5000, FTP_DEBUG);

/** @brief Отправляем все файлы в папках ready на FTP сервер
 * Сначала проверяем, есть ли в результатах сканирования точка доступа,
 * через которую отправляем. Далее смотрим флаги отправки, при наличии разрешения -
 * отправляем с добавлением к имени файла ID модуля.
 * @param pvParameters Стандартный параметр для Task
 */
void ftpTask(void *pvParameters)
{

    String _newName;
    String _tmpFtpName;
    String _tmpFtpName2;

    unsigned char _wifiFileBuf[256];
    bool _ftpErr = false;
    bool _wifiSended = false;
    bool _gpsSended = false;

    File _ftpSend;
    File _ftpReadyDir;
    File _ftpSavedDir;
    File _entry;

    for (;;)
    {
        _wifiSended = false;
        _gpsSended = false;
        xEventGroupWaitBits(eventGroup_1, BIT_G1_10, pdTRUE, pdTRUE, portMAX_DELAY);
        xSemaphoreTake(sdMutex, portMAX_DELAY);

        if (!SD.exists(wifiFlDir.ready) ||
            !SD.exists(wifiFlDir.saved) ||
            !SD.exists(gpsFlDir.ready) ||
            !SD.exists(gpsFlDir.saved))
        {
            xSemaphoreGive(sdMutex);
            continue;
        }
        xSemaphoreGive(sdMutex);
        if (config.wifisend)
        {
            xSemaphoreTake(sdMutex, portMAX_DELAY);
            if (!(_ftpReadyDir = SD.open(wifiFlDir.ready)) || !(_ftpSavedDir = SD.open(wifiFlDir.saved)))
            {
                fatalError();
            }
            _entry = _ftpReadyDir.openNextFile();
            xSemaphoreGive(sdMutex);
            if (_entry)
            {
                xSemaphoreTake(sdMutex, portMAX_DELAY);
                _ftpReadyDir.rewindDirectory();
                xSemaphoreGive(sdMutex);

                xSemaphoreTake(wifiMutex, portMAX_DELAY);
                delay(10);
                WiFi.begin(config.ssid, config.wifiPasswd);

                // Добавить ограничение по таймауту
                int i = 0;
                while (WiFi.status() != WL_CONNECTED)
                {
                    delay(100);
                    i++;
                    if (i == 200)
                        break;
                }
                if (i >= 200)
                {
                    WiFi.disconnect();
                    _ftpReadyDir.close();
                    _ftpSavedDir.close();
                    delay(100);
                    xSemaphoreGive(wifiMutex);
                    continue;
                }
                _ftp.OpenConnection();
                _ftp.ChangeWorkDir("/");

                if (!_ftp.isConnected())
                {
                    _ftp.CloseConnection();
                }
                else
                {
                    // Отправка файлов
                    _ftpErr = false;
                    while (!_ftpErr)
                    {
                        xSemaphoreTake(sdMutex, portMAX_DELAY);
                        _entry = _ftpReadyDir.openNextFile();
/*
                        if (_entry.size() <= 243U)
                        {
                            SD.remove(_entry.name());
                            logging("%s deleted: size 243 bytes or less\n", _entry.name(), false);
                            xSemaphoreGive(sdMutex);
                            continue;
                        }
*/
                        xSemaphoreGive(sdMutex);
                        if (!_entry)
                        {
                            break;
                        }
                        _newName = (String)wifiFlDir.saved + ((String)_entry.name()).substring(WIFI_SUBSTR_1);
                        _tmpFtpName = (String)_entry.name() + (String)config.moduleID + (String) ".partial";
                        _tmpFtpName2 = ((String)_entry.name()).substring(0, WIFI_SUBSTR_2) +
                                       (String)'-' +
                                       (String)config.moduleID +
                                       ((String)_entry.name()).substring(WIFI_SUBSTR_2);
                        _ftp.InitFile("Type I");
                        _ftp.NewFile(_tmpFtpName.c_str());
                        xSemaphoreTake(sdMutex, portMAX_DELAY);
                        _ftpSend = SD.open(_entry.name());
                        xSemaphoreGive(sdMutex);
                        size_t wifiFileCount = 0;
                        xSemaphoreTake(sdMutex, portMAX_DELAY);
                        while ((wifiFileCount = _ftpSend.readBytes((char *)_wifiFileBuf, sizeof(_wifiFileBuf))) != 0)
                        {
                            xSemaphoreGive(sdMutex);
                            _ftp.WriteData(_wifiFileBuf, wifiFileCount);
                            if (!_ftp.isConnected())
                            {
                                _ftpErr = true;
                                break;
                            }
                            xSemaphoreTake(sdMutex, portMAX_DELAY);
                        }
                        xSemaphoreGive(sdMutex);
                        _ftpSend.close();
                        _ftp.CloseFile();
                        if (!_ftpErr)
                        {
                            _ftp.RenameFile((char *)_tmpFtpName.c_str(), (char *)_tmpFtpName2.c_str());
                            if (!_ftpErr)
                            {
                                xSemaphoreTake(sdMutex, portMAX_DELAY);
                                SD.rename((String)_entry.name(), _newName);
                                logging("%s file is sent to the FTP server\n", _entry.name(), false);
                                xSemaphoreGive(sdMutex);
                            }
                        }
                    }
                    _wifiSended = !_ftpErr;
                }
                WiFi.disconnect();
                xSemaphoreGive(wifiMutex);
            }

            _ftpReadyDir.close();
            _ftpSavedDir.close();
            delay(100);
        }
        if (config.gpssend && !_ftpErr)
        {
            xSemaphoreTake(sdMutex, portMAX_DELAY);
            if (!(_ftpReadyDir = SD.open(gpsFlDir.ready)) || !(_ftpSavedDir = SD.open(gpsFlDir.saved)))
            {
                fatalError();
            }
            _entry = _ftpReadyDir.openNextFile();
            xSemaphoreGive(sdMutex);
            if (_entry)
            {
                xSemaphoreTake(sdMutex, portMAX_DELAY);
                _ftpReadyDir.rewindDirectory();
                xSemaphoreGive(sdMutex);

                xSemaphoreTake(wifiMutex, portMAX_DELAY);
                delay(10);
                WiFi.begin(config.ssid, config.wifiPasswd);

                // Добавить ограничение по таймауту
                int i = 0;
                while (WiFi.status() != WL_CONNECTED)
                {
                    delay(100);
                    i++;
                    if (i == 200)
                        break;
                }
                if (i >= 200)
                {
                    WiFi.disconnect();
                    _ftpReadyDir.close();
                    _ftpSavedDir.close();
                    delay(100);
                    xSemaphoreGive(wifiMutex);
                    continue;
                }

                _ftp.OpenConnection();
                _ftp.ChangeWorkDir("/");

                if (!_ftp.isConnected())
                {
                    _ftp.CloseConnection();
                }
                else
                {
                    // Отправка файлов
                    _ftpErr = false;
                    while (!_ftpErr)
                    {
                        xSemaphoreTake(sdMutex, portMAX_DELAY);
                        _entry = _ftpReadyDir.openNextFile();
/*
                        if (_entry.size() <= 234U)
                        {
                            SD.remove(_entry.name());
                            logging("%s deleted: size 234 bytes or less\n", _entry.name(), false);
                            xSemaphoreGive(sdMutex);
                            continue;
                        }
*/
                        xSemaphoreGive(sdMutex);
                        if (!_entry)
                        {
                            break;
                        }
                        _newName = (String)gpsFlDir.saved + ((String)_entry.name()).substring(GPS_SUBSTR_1);
                        _tmpFtpName = (String)_entry.name() + (String)config.moduleID + (String) ".partial";
                        _tmpFtpName2 = ((String)_entry.name()).substring(0, GPS_SUBSTR_2) +
                                       (String)'-' +
                                       (String)config.moduleID +
                                       ((String)_entry.name()).substring(GPS_SUBSTR_2);
                        _ftp.InitFile("Type I");
                        // ftp.NewFile(entry.name());
                        _ftp.NewFile(_tmpFtpName.c_str());
                        xSemaphoreTake(sdMutex, portMAX_DELAY);
                        _ftpSend = SD.open(_entry.name());
                        xSemaphoreGive(sdMutex);
                        size_t gpsFileCount = 0;
                        xSemaphoreTake(sdMutex, portMAX_DELAY);
                        while ((gpsFileCount = _ftpSend.readBytes((char *)_wifiFileBuf, sizeof(_wifiFileBuf))) != 0)
                        {
                            xSemaphoreGive(sdMutex);
                            _ftp.WriteData(_wifiFileBuf, gpsFileCount);
                            if (!_ftp.isConnected())
                            {
                                _ftpErr = true;
                                break;
                            }
                            xSemaphoreTake(sdMutex, portMAX_DELAY);
                        }
                        xSemaphoreGive(sdMutex);
                        _ftpSend.close();
                        _ftp.CloseFile();
                        if (!_ftpErr)
                        {
                            // ftp.RenameFile((char *)tmpFtpName.c_str(), (char *)entry.name());
                            _ftp.RenameFile((char *)_tmpFtpName.c_str(), (char *)_tmpFtpName2.c_str());
                            if (!_ftpErr)
                            {
                                xSemaphoreTake(sdMutex, portMAX_DELAY);
                                SD.rename((String)_entry.name(), _newName);
                                logging("%s file is sent to the FTP server\n", _entry.name(), false);
                                xSemaphoreGive(sdMutex);
                            }
                        }
                    }
                    _gpsSended = !_ftpErr;
                }
                WiFi.disconnect();
                xSemaphoreGive(wifiMutex);
            }

            _ftpReadyDir.close();
            _ftpSavedDir.close();
        }

        if (config.ftpbeep && (_gpsSended || _wifiSended))
        {
            for (int i = 0; i < 3; i++)
            {
                ledcWriteTone(BUZZER_CHAN, 1200);
                delay(100);
                ledcWriteTone(BUZZER_CHAN, 0);
                delay(100);
            }
        }
        delay(DELAY_FTP_TASK);
    }
} // ftpTask()
