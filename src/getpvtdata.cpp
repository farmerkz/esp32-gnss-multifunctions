#include "main.h"

extern EventGroupHandle_t eventGroup_1;
extern xQueueHandle queueGnssWiFi;
extern xQueueHandle queueGnssTrack;
extern xQueueHandle queueFileDateName;
extern xQueueHandle queueFileTimeName;
extern xQueueHandle queueTrackTime;

/** @brief Callback: выставляем флаги валидности даты и позиции для WiFi, при валидности
 * позиции пишем в очередь данные для WiFi.
 * При валидности времени - формируем дату и время для различных применений, выставляем
 * флаги валидности даты и времени BIT_G1_3, BIT_G1_4, BIT_G1_5.
 * Выставляем флаг валидности BIT_G1_2 для трека.
 * Если не установлен флаг системного времени BIT_G1_6 - устанавливаем системное время и 
 * выставляем флаг BIT_G1_6.
 * @param pvParameters Стандартный параметр для Task
 */
void getPVTdata(UBX_NAV_PVT_data_t ubxDataStruct)
{
    wifiPos _localWiFiPos; // Локальная структура для WiFi позиции
    char _fDate[] = DATE_MASK;
    char _fTime1[] = TIME_MASK_1;
    char _fTime2[] = TIME_MASK_2;
    tm tm;
    timeval tv;
    EventBits_t _eventBits;

    xEventGroupClearBits(eventGroup_1, BIT_G1_2 | BIT_G1_3 | BIT_G1_4 | BIT_G1_5); // Обнуляем флаги позиции и имен
    xQueueOverwrite(queueGnssTrack, &ubxDataStruct);                               // Сохраняем пакет в очередь
    if ((ubxDataStruct.fixType == 3) && (ubxDataStruct.flags.bits.gnssFixOK == 1)) // Позиция валидна???
    {
        _localWiFiPos.isValid = true;                 // Да, валидна
        _localWiFiPos.wifiLat = ubxDataStruct.lat;    // Lattitude, deg * 1e-7
        _localWiFiPos.wifiLong = ubxDataStruct.lon;   // Longtitude, deg * 1e-7
        _localWiFiPos.wifiAccur = ubxDataStruct.hAcc; // Горизонтальная точность, mm
        _localWiFiPos.wifiAlt = ubxDataStruct.height; // Высота, mm
        xEventGroupSetBits(eventGroup_1, BIT_G1_2);   // Теперь мы знаем, что позиция валидна
    }
    else
    {
        _localWiFiPos.isValid = false; // Нет, не валидна
        _localWiFiPos.wifiLat = 0;
        _localWiFiPos.wifiLong = 0;
        _localWiFiPos.wifiAccur = 0;
        _localWiFiPos.wifiAlt = 0;
    }
    xQueueOverwrite(queueGnssWiFi, &_localWiFiPos); // Сохраняем позицию для WiFi в очередь, независимо от валидности

    // Дата и время валидны???
    if ((ubxDataStruct.valid.bits.validDate == 1) &&
        (ubxDataStruct.valid.bits.validTime == 1) &&
        (ubxDataStruct.valid.bits.fullyResolved == 1) &&
        (ubxDataStruct.fixType == 3))
    {
        _eventBits = xEventGroupGetBits(eventGroup_1);
        if ((_eventBits & BIT_G1_6) == 0) // Системное время установлено???
        {
            tm.tm_year = ubxDataStruct.year - 1900; // Системное время не установлено, устанавливаем
            tm.tm_mon = ubxDataStruct.month - 1;
            tm.tm_mday = ubxDataStruct.day;
            tm.tm_hour = ubxDataStruct.hour;
            tm.tm_min = ubxDataStruct.min;
            tm.tm_sec = ubxDataStruct.sec;
            tm.tm_isdst = 0;
            tv.tv_sec = mktime(&tm) + (time_t)TZ_SECONDS;
            tv.tv_usec = 0;
            settimeofday(&tv, NULL);
            xEventGroupSetBits(eventGroup_1, BIT_G1_6); // Выставляем флаг установки системного времени
        }
        sprintf(_fDate,
                DATE_FORMAT,
                ubxDataStruct.year,
                ubxDataStruct.month,
                ubxDataStruct.day);
        sprintf(_fTime1,
                TIME_FORMAT_1,
                ubxDataStruct.hour,
                ubxDataStruct.min,
                ubxDataStruct.sec);
        sprintf(_fTime2,
                TIME_FORMAT_2,
                ubxDataStruct.hour,
                ubxDataStruct.min,
                ubxDataStruct.sec);
        xQueueOverwrite(queueFileDateName, _fDate);                       // Сохраняем дату для имени файлов
        xQueueOverwrite(queueFileTimeName, _fTime1);                      // Время для имени файлов
        xQueueOverwrite(queueTrackTime, _fTime2);                         // Время для трека и WiFi
        xEventGroupSetBits(eventGroup_1, BIT_G1_3 | BIT_G1_4 | BIT_G1_5); // И выставяем флаги их готовности
    }                                                                     // Дата и время валидны

} // getPVTdata()
