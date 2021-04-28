#include "main.h"

extern void checkFolders(void);
extern void fatalError(void);
extern xQueueHandle queueFileDateName;
extern xQueueHandle queueFileTimeName;
extern xQueueHandle queueTrackTime;
extern volatile xSemaphoreHandle sdMutex;
extern volatile xSemaphoreHandle wifiMutex;
extern EventGroupHandle_t eventGroup_1;
extern File trackFile;
extern File wifiFile;

/** @brief Создание и открытие на запись всех файлов, запись заголовков
 * Выполняется только при наличии сформированных даты и времени: проверяются флаги,
 * должны быть установлены все три, после проверки сбрасываются.
 * Если папки не проверялись на наличие - вызываем проверку.
 * @param pvParameters стандартный параметр Task
 */
void createAllFiles(void *pvParameters)
{
    fs::SDFS *_pxTaskParam;
    _pxTaskParam = (fs::SDFS *)pvParameters;
    EventBits_t _eventBits;
    size_t _writedBytes;
    char _ldate[] = DATE_MASK;
    char _ltime1[] = TIME_MASK_1;
    char _ltime2[] = TIME_MASK_2;
    char _fileTrackName[] = GPS_WK "/" GPS_PREFIX DATE_MASK "_" TIME_MASK_1 GPS_EXT;
    char _fileWifiName[] = WIFI_WK "/" WIFI_PREFIX DATE_MASK "_" TIME_MASK_1 WIFI_EXT;

    for (;;)
    {
        xEventGroupWaitBits(eventGroup_1, (BIT_G1_3 | BIT_G1_4 | BIT_G1_5), pdTRUE, pdTRUE, portMAX_DELAY);
        _eventBits = xEventGroupGetBits(eventGroup_1);
        xQueuePeek(queueFileDateName, _ldate, portMAX_DELAY);
        xQueuePeek(queueFileTimeName, _ltime1, portMAX_DELAY);
        xQueuePeek(queueTrackTime, _ltime2, portMAX_DELAY);

        // Если папки еще не проверялись - проверяем и, при необходимости создаем
        if ((_eventBits & BIT_G1_7) == 0x00)
        {
            xSemaphoreTake(sdMutex, portMAX_DELAY);
            checkFolders();
            xSemaphoreGive(sdMutex);
            xEventGroupSetBits(eventGroup_1, BIT_G1_7);
        }

        // ---------------------------------------------------
        // Если файл трека GPS еще не создавался - создаем его.
        if ((_eventBits & BIT_G1_8) == 0x00)
        {
            sniprintf(_fileTrackName,
                      sizeof(_fileTrackName),
                      GPS_WK "/" GPS_PREFIX "%s_%s" GPS_EXT,
                      _ldate,
                      _ltime1);
            xSemaphoreTake(sdMutex, portMAX_DELAY);
            trackFile = _pxTaskParam->open(_fileTrackName, FILE_WRITE);
            if (!trackFile)
            {
                fatalError();
            }
            _writedBytes = trackFile.printf(TRACK_HEAD);
            if (_writedBytes <= 0)
            {
                fatalError();
            }
            _writedBytes = trackFile.printf("<name>GPS-%s</name>\n", _ldate);
            if (_writedBytes <= 0)
            {
                fatalError();
            }
            _writedBytes = trackFile.printf("<trk><name>TRK-%sT%s</name>\n<trkseg>\n", _ldate, _ltime2);
            if (_writedBytes <= 0)
            {
                fatalError();
            }
            _writedBytes = trackFile.printf(TRACK_EPILOG);
            if (_writedBytes <= 0)
            {
                fatalError();
            }
            trackFile.flush();
            xSemaphoreGive(sdMutex);
            xEventGroupSetBits(eventGroup_1, BIT_G1_8);
        }

        // ---------------------------------------------------
        // Если файл Wifi еще не создавался - создаем его.
        if ((_eventBits & BIT_G1_9) == 0x00)
        {
            sniprintf(_fileWifiName,
                      sizeof(_fileWifiName),
                      WIFI_WK "/" WIFI_PREFIX "%s_%s" WIFI_EXT,
                      _ldate,
                      _ltime1);
            xSemaphoreTake(sdMutex, portMAX_DELAY);
            wifiFile = _pxTaskParam->open(_fileWifiName, FILE_WRITE);
            if (!wifiFile)
            {
                fatalError();
            }
            _writedBytes = wifiFile.println("WigleWifi-1.4,"
                                            "appRelease=2.26,"
                                            "model=HomeModel,"
                                            "release=0.0.2,"
                                            "device=arduinoWardriving,"
                                            "display=none,"
                                            "board=esp32-based,"
                                            "brand=Espressif");
            if (_writedBytes <= 0)
            {
                fatalError();
            }
            _writedBytes = wifiFile.println("MAC,"
                                            "SSID,"
                                            "AuthMode,"
                                            "FirstSeen,"
                                            "Channel,"
                                            "RSSI,"
                                            "CurrentLatitude,"
                                            "CurrentLongitude,"
                                            "AltitudeMeters,"
                                            "AccuracyMeters,"
                                            "Type");
            if (_writedBytes <= 0)
            {
                fatalError();
            }
            wifiFile.flush();
            xSemaphoreGive(sdMutex);
            xEventGroupSetBits(eventGroup_1, BIT_G1_9);
        }
    }
    delay(DELAY_CREATE_ALL_FILES);
}
