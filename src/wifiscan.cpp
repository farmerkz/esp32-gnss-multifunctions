#include "main.h"

extern volatile xSemaphoreHandle wifiMutex;
extern volatile xSemaphoreHandle sdMutex;
extern EventGroupHandle_t eventGroup_1;
extern xQueueHandle queueGnssWiFi;
extern xQueueHandle queueFileDateName;
extern xQueueHandle queueTrackTime;
extern File wifiFile;
extern void fatalError(void);
extern void clearFolders(bool gps, bool wifi);
extern fileConfig config;

// Преобразование кода encryption в строку
String translateEncryptionType(wifi_auth_mode_t encryptionType)
{
    switch (encryptionType)
    {
    case (0):
        return "[Open]";
    case (1):
        return "[WEP][ESS]";
    case (2):
        return "[WPA-PSK-CCMP+TKIP][ESS]";
    case (3):
        return "[WPA2-PSK-CCMP+TKIP][ESS]";
    case (4):
        return "[WPA-PSK-CCMP+TKIP][WPA2-PSK-CCMP+TKIP][ESS]";
    case (5):
        return "[WPA2-ENTERPRISE-CCMP+TKIP][ESS]";
    default:
        return "[UNKOWN]";
    }
} // translateEncryptionType()

// ------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------

// Сканирование WiFi и запись результата в файл
// Сканирование производится всегда, запись в файл - только
// при условии выставленных флагов BIT_G1_9 (файл WiFi создан) и isValid (позиция валидна),
// а также при наличии хотя бы одной WiFi сети в результатах сканирования.
// Если после записи размер файла стал больше config.filesize, то то текущий файл
// закрывается, вызывается очистка папки wifi.wk и обнуляется
// флаг BIT_G1_9 (файл WiFi создан)
void wifiCode(void *pvParameters)
{
    EventBits_t _eventBits;
    char _lDate[] = DATE_MASK;
    char _lTime2[] = TIME_MASK_2;
    wifiPos _lWifi;
    char _wigleString[250];
    int n = 0;
    size_t _writedBytes;
    String _newFlName;

    for (;;)
    {
        // xEventGroupWaitBits(eventGroup_1, BIT_G1_9, pdFALSE, pdTRUE, portMAX_DELAY);
        _eventBits = xEventGroupGetBits(eventGroup_1);
        xSemaphoreTake(wifiMutex, portMAX_DELAY);
        n = WiFi.scanNetworks(false, true);

        xQueuePeek(queueGnssWiFi, &_lWifi, 1200UL / portTICK_RATE_MS);
        xQueuePeek(queueFileDateName, &_lDate, 1200UL / portTICK_RATE_MS);
        xQueuePeek(queueTrackTime, &_lTime2, 1200 / portTICK_RATE_MS);

        if ((n != 0) && _lWifi.isValid && (((_eventBits & BIT_G1_9) == BIT_G1_9)))
        {
            for (int i = 0; i < n; ++i)
            {
                snprintf(_wigleString, sizeof(_wigleString),
                         "%s,%s,%s,%s %s,%d,%d,%2.8f,%2.8f,%d,%.8f,WIFI\n",
                         WiFi.BSSIDstr(i).c_str(),
                         WiFi.SSID(i).c_str(),
                         translateEncryptionType(WiFi.encryptionType(i)).c_str(),
                         _lDate,
                         _lTime2,
                         WiFi.channel(i),
                         WiFi.RSSI(i),
                         _lWifi.wifiLat * en7,
                         _lWifi.wifiLong * en7,
                         (uint32_t)(_lWifi.wifiAlt * mm2m),
                         _lWifi.wifiAccur * mm2m);
                xSemaphoreTake(sdMutex, portMAX_DELAY);
                _writedBytes = wifiFile.print(_wigleString);
                xSemaphoreGive(sdMutex);
                if (_writedBytes <= 0)
                {
                    fatalError();
                }
                delay(10);
                if (strcmp(WiFi.SSID(i).c_str(), config.ssid) == 0)
                {
                    xEventGroupSetBits(eventGroup_1, BIT_G1_10);
                }
            }
            xSemaphoreTake(sdMutex, portMAX_DELAY);
            wifiFile.flush();
            if (wifiFile.size() >= config.filesize)
            {
                wifiFile.close();
                xEventGroupClearBits(eventGroup_1, BIT_G1_9);
                clearFolders(false, true);
            }
            xSemaphoreGive(sdMutex);
        }
        else if (n != 0)
        {
            for (int i = 0; i < n; ++i)
            {
                if (strcmp(WiFi.SSID(i).c_str(), config.ssid) == 0)
                {
                    xEventGroupSetBits(eventGroup_1, BIT_G1_10);
                }
            }
        }

        WiFi.scanDelete();
        xSemaphoreGive(wifiMutex);
        delay(DELAY_WIFI_CODE);
    }
} // wifiCode()
