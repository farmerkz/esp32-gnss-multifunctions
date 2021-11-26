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
        _ftpErr = false;
        // Ждем, пока будет доступен WiFi
        xEventGroupWaitBits(eventGroup_1, BIT_G1_10, pdTRUE, pdTRUE, portMAX_DELAY);
        xSemaphoreTake(sdMutex, portMAX_DELAY);                                       // Захватываем SD
        logging("Обнаружена точка доступа WiFi для отправки на FTP\n", false);        // Отмечаемся в логе
                                                                                      //
        if (!SD.exists(wifiFlDir.ready) ||                                            //
            !SD.exists(wifiFlDir.saved) ||                                            //
            !SD.exists(gpsFlDir.ready) ||                                             //
            !SD.exists(gpsFlDir.saved))                                               //
        {                                                                             // Нет одной из этих папок
            xSemaphoreGive(sdMutex);                                                  // Освобождаем SD
            continue;                                                                 // Начинаем новую итерацию
        }                                                                             //
        xSemaphoreGive(sdMutex);                                                      // Освобождаем SD и идем дальше
        if (config.wifisend)                                                          // Отправка файлов Wigle разрешена?
        {                                                                             // Разрешена
            xSemaphoreTake(sdMutex, portMAX_DELAY);                                   // Захватываем SD
            logging("Начинаем проверку файлов WiFi доступных для отправки\n", false); // Отмечаемся в логе
            if (!(_ftpReadyDir = SD.open(wifiFlDir.ready)) ||
                !(_ftpSavedDir = SD.open(wifiFlDir.saved)))
            {                                               // Ошибка открытия папок
                fatalError();                               // Уходим в перезагрузку
            }                                               //
            _entry = _ftpReadyDir.openNextFile();           // Ищем первый файл в папке на отправку
            xSemaphoreGive(sdMutex);                        // Освобождаем SD
            if (_entry)                                     // Имеется очередной файл на отправку???
            {                                               //
                xSemaphoreTake(sdMutex, portMAX_DELAY);     // Захватываем SD
                _ftpReadyDir.rewindDirectory();             // На первый файл в папке
                xSemaphoreGive(sdMutex);                    // Освобождаем SD
                                                            //
                xSemaphoreTake(wifiMutex, portMAX_DELAY);   // Захватываем WiFi
                delay(10);                                  // На всякий случай ждем
                WiFi.begin(config.ssid, config.wifiPasswd); // Запускаем подключение к WiFi
                                                            //
                int i = 0;
                while (WiFi.status() != WL_CONNECTED)       // Ждем, пока подключимся к WiFi
                {                                           //
                    delay(100);                             // 200 циклов по 100 мс, 20 секунд, достаточно для подключения
                    i++;                                    //
                    if (i == 200)                           //
                        break;                              //
                }                                           //
                if (i >= 200)                               // Не подключились за 20 секунд
                {                                           //
                    WiFi.disconnect();                      //
                    xSemaphoreTake(sdMutex, portMAX_DELAY); // Захватываем SD
                    _ftpReadyDir.close();                   // Закрываем папки
                    _ftpSavedDir.close();                   //
                    xSemaphoreGive(sdMutex);                // Освобождаем SD
                    delay(100);                             //
                    xSemaphoreGive(wifiMutex);              // Освобождаем WiFi
                    continue;                               // Начинаем новую итерацию
                }
                _ftp.OpenConnection();      // Подключаемся к FTP
                _ftp.ChangeWorkDir("/");    //
                                            //
                if (!_ftp.isConnected())    // Проверяем подключение к FTP
                {                           // Не подключились
                    _ftp.CloseConnection(); // Закрываем подключение
                }                           //
                else                        // Есть подключение к FTP
                {                           // Начинаем отправку файлов
                    _ftpErr = false;        //
                    while (!_ftpErr)        // Если ошибка - отправку прекращаем
                    {
                        xSemaphoreTake(sdMutex, portMAX_DELAY); // Захватываем SD
                        _entry = _ftpReadyDir.openNextFile();   // Находим первый файл
                        xSemaphoreGive(sdMutex);                // Освобождаем SD
                        if (!_entry)                            // Если файлов больше нет - прекращаем отправку
                        {
                            break;
                        }
                        _newName = (String)wifiFlDir.saved +                                 // Формируем имена файлов:
                                   ((String)_entry.name()).substring(WIFI_SUBSTR_1);         // Для сохранения в save папке
                        _tmpFtpName = (String)_entry.name() +                                // Временное имя используется
                                      (String)config.moduleID + (String) ".partial";         // в процессе отправки
                        _tmpFtpName2 = ((String)_entry.name()).substring(0, WIFI_SUBSTR_2) + // Окончательное
                                       (String)'-' +                                         // имя файла на FTP сервере
                                       (String)config.moduleID +
                                       ((String)_entry.name()).substring(WIFI_SUBSTR_2);
                        _ftp.InitFile("Type I");                // Бинарный режим FTP
                        _ftp.NewFile(_tmpFtpName.c_str());      // Создаем файл на FTP сервере
                        xSemaphoreTake(sdMutex, portMAX_DELAY); // Захватываем SD
                        _ftpSend = SD.open(_entry.name());      // Открываем файл для отправки
                        // xSemaphoreGive(sdMutex);                // ???????
                        size_t wifiFileCount = 0;
                        // xSemaphoreTake(sdMutex, portMAX_DELAY); // ????????
                        // Читаем файл блоками, пока не закончится файл
                        while ((wifiFileCount =
                                    _ftpSend.readBytes((char *)_wifiFileBuf, sizeof(_wifiFileBuf))) != 0)
                        {
                            xSemaphoreGive(sdMutex);                     // Освобождаем SD
                            _ftp.WriteData(_wifiFileBuf, wifiFileCount); // отправляем на FTP очередной блок
                            xSemaphoreTake(sdMutex, portMAX_DELAY);      // Захватываем SD для чтения следующего блока
                            if (!_ftp.isConnected())                     // проверяем ошибку
                            {                                            //
                                _ftpErr = true;                          // если ошибка - устанавливаем флаг
                                break;                                   // прекращаем передачу
                            }                                            //
                        }
                        _ftpSend.close();        // Закрываем файл
                        xSemaphoreGive(sdMutex); // Освобождаем SD
                        _ftp.CloseFile();        // Закрываем файл на FTP сервере
                        if (!_ftpErr)            // Если не было ошибки, переименовываем файл на FTP сервере
                        {
                            _ftp.RenameFile((char *)_tmpFtpName.c_str(), (char *)_tmpFtpName2.c_str());
                            _ftpErr = !_ftp.isConnected();
                            if (!_ftpErr) // Если файл на FTP успешно переименован, перемещаем его в saved папку
                            {
                                xSemaphoreTake(sdMutex, portMAX_DELAY);
                                SD.rename((String)_entry.name(), _newName);
                                logging("%s file is sent to the FTP server\n", _entry.name(), false);
                                xSemaphoreGive(sdMutex);
                            }
                            else
                            {
                                logging("Ошибка отправки файла %s\n", _entry.name(), true);
                            }
                        }
                    }
                    _wifiSended = !_ftpErr;
                }
                WiFi.disconnect();
                xSemaphoreGive(wifiMutex); // Освобождаем WiFi
            }
            xSemaphoreTake(sdMutex, portMAX_DELAY);                           // Захватываем SD
            _ftpReadyDir.close();                                             // Закрываем
            _ftpSavedDir.close();                                             // папки
            logging("Завершен процесс отправки файлов WiFi на FTP\n", false); // Отмечаемся в логе
            xSemaphoreGive(sdMutex);                                          // Освобождаем SD
            delay(100);
        }
        if (config.gpssend && !_ftpErr) // Если не было ошибки и разрешена отправка треков - повторяем для треков
        {
            xSemaphoreTake(sdMutex, portMAX_DELAY);
            logging("Начинаем проверку файлов GPS доступных для отправки\n", false);
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
                    xSemaphoreTake(sdMutex, portMAX_DELAY);
                    _ftpReadyDir.close();
                    _ftpSavedDir.close();
                    xSemaphoreGive(sdMutex);
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
                        _ftp.NewFile(_tmpFtpName.c_str());
                        xSemaphoreTake(sdMutex, portMAX_DELAY);
                        _ftpSend = SD.open(_entry.name());
                        // xSemaphoreGive(sdMutex);
                        size_t gpsFileCount = 0;
                        // xSemaphoreTake(sdMutex, portMAX_DELAY);
                        while ((gpsFileCount = _ftpSend.readBytes((char *)_wifiFileBuf, sizeof(_wifiFileBuf))) != 0)
                        {
                            xSemaphoreGive(sdMutex);
                            _ftp.WriteData(_wifiFileBuf, gpsFileCount);
                            xSemaphoreTake(sdMutex, portMAX_DELAY); // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                            if (!_ftp.isConnected())
                            {
                                _ftpErr = true;
                                break;
                            }
                        }
                        _ftpSend.close();
                        xSemaphoreGive(sdMutex);
                        _ftp.CloseFile();
                        if (!_ftpErr)
                        {
                            _ftp.RenameFile((char *)_tmpFtpName.c_str(), (char *)_tmpFtpName2.c_str());
                            _ftpErr = !_ftp.isConnected();
                            if (!_ftpErr)
                            {
                                xSemaphoreTake(sdMutex, portMAX_DELAY);
                                SD.rename((String)_entry.name(), _newName);
                                logging("%s file is sent to the FTP server\n", _entry.name(), false);
                                xSemaphoreGive(sdMutex);
                            }
                            else
                            {
                                logging("Ошибка отправки файла %s\n", _entry.name(), true);
                            }
                        }
                    }
                    _gpsSended = !_ftpErr;
                }
                WiFi.disconnect();
                xSemaphoreGive(wifiMutex);
            }

            xSemaphoreTake(sdMutex, portMAX_DELAY);
            _ftpReadyDir.close();
            _ftpSavedDir.close();
            logging("Завершен процесс отправки файлов GPS на FTP\n", false);
            xSemaphoreGive(sdMutex);
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
