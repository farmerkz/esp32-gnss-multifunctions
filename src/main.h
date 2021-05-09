#ifndef __my_main_h__
#define __my_main_h__

#include <Arduino.h>
#include "SD.h"                                   // Библиотека для работы с SD картами
#include "esp_bt_main.h"                          // Библиотеки для BT
#include "esp_bt.h"                               // Библиотеки для BT
#include "esp_wifi.h"                             // Библиотека WiFi
#include "WiFi.h"                                 // Библиотека WiFi
#include <SparkFun_u-blox_GNSS_Arduino_Library.h> // Библиотека для работы с GNSS модулем
#include "ArduinoJson.h"                          // Библиотека для работа с JSON
#include <Update.h>                               // Для обновления через SD карту
#include "freertos/task.h"                        // Библиотека FreeRTOS
#include "accountinfo.h"                          // Учетные данные WiFi и FTP по умолчанию
#include "ESP32_FTPClient.h"                      // Библиотека для FTP клиента

// ====================================================================================
// Секция макросов
// ====================================================================================

// Макросы учетных данных
#ifndef __ACCOUNTINFO__
#define WIFI_SSID "ssid"
#define WIFI_PASS "wifipassword"

#define FTP_ADDRESS "192.168.168.100"
#define FTP_USER "ftpuser"
#define FTP_PASSWD "ftppasswd"
#define FTP_CMD_PORT (uint16_t)21 // ftp-cmd порт, по умолчанию 21
#endif

#ifdef DEBUG
#define IS_DEBUG "Debug"
#else
#define IS_DEBUG "NoDebug"
#endif

#define RED_PIN 12                     // Красный светодиод
#define GREEN_PIN 13                   // Зеленый светодиод
#define BLUE_PIN 14                    // Голубой светодиод
#define BUZZER_PIN 25                  // Пин для пищалки
#define RED_CHAN 0                     // Канал для красного светодиода
#define GREEN_CHAN 1                   // Канал для зеленого светодиода
#define BLUE_CHAN 2                    // Канал для голубого светодиода
#define BUZZER_CHAN 3                  // Канал для пищалки
#define DEEP_SLEEP_TIME 15000000UL     // Время Deep Sleep в случае возникновения проблем, мкс
#define LED_RED_FATAL_BLINK 10         // Количество циклов моргания красным светодиод при фатальной ошибке
#define TZ_HOURS 6                     // Таймзона - в часах
#define TZ_SECONDS (3600 * TZ_HOURS)   // Таймзона - в секундах
#define SEEK_TRKPT_BACKWARDS 24        // Длина эпилога трека
#define CSV_MAX_FILESIZE 1000000UL     // Максимальный размер файла WiFi по умолчанию (в байтах)
#define JSON_OBJ_SIZE 512              // Размер буфера для JSON
#define CONF_FILE_NAME "/config.json"  // Имя файла конфигурации
#define FIRMWARE "/firmware.bin"       // Имя файла обновления
#define DATE_MASK "yyyy-mm-dd"         // Маска даты для использования в разных именах
#define TIME_MASK_1 "hhmmss"           // Маска времени для имени файла
#define TIME_MASK_2 "hh:mm:ss"         // Маска времени для трека и WiFi
#define DATE_FORMAT "%04d-%02d-%02d"   // Формат даты для использования в разных именах
#define TIME_FORMAT_1 "%02d%02d%02d"   // Формат времени для имени файла
#define TIME_FORMAT_2 "%02d:%02d:%02d" // Формат времени для трека и WiFi
#define TZ_HOURS 6                     // Таймзона в часах
#define TZ_SECONDS (3600 * TZ_HOURS)   // Таймзона в секундах
#define GPS_WK "/gps.wk"               // Рабочая папка
#define GPS_RD "/gps.rd"               // Готово для отправки на FTP
#define GPS_SD "/gps.sd"               // Отправлено на FTP
#define WIFI_WK "/wifi.wk"             // Рабочая папка
#define WIFI_RD "/wifi.rd"             // Готово для отправки на FTP
#define WIFI_SD "/wifi.sd"             // Отправлено на FTP
#define GPS_PREFIX "gps-"              // Префикс для имени файла трека
#define GPS_EXT ".gpx"                 // Расширение имени файла трека
#define WIFI_PREFIX "wifi-"            // Префикс для имени файла WiFi
#define WIFI_EXT ".csv"                // Расширение имени файла WiFi
#define LOG_FILE "/error.log"          // Имя файла лога
#define DELAY_CREATE_ALL_FILES 100     // Delay time для цикла в createAllFiles
#define DELAY_GNSS_TRACK_CODE 10       // Delay time для цикла в gnssTrackCode
#define mm2m (double)1.0e-3            //Для перевода милиметров в метры
#define en7 (double)1.0e-7             // Множитель для координат
#define en5 (double)1.0e-5             // множитель для курса
#define DELAY_WIFI_CODE 10             // Delay для цикла в wifiCode
#define FTP_DEBUG 0                    // Параметр отладки FTP клиента
#define DELAY_FTP_TASK 60000           // Delay для цикла в ftpTask
#define WIFI_SUBSTR_1 8                // Подстрока для исключения имени папки /wifi.xx
#define WIFI_SUBSTR_2 31               // Подстрока для добавления ID модуля в имя фала WiFi
#define GPS_SUBSTR_1 7                 // Подстрока для исключения имени папки /gps.xx
#define GPS_SUBSTR_2 29                // Подстрока для добавления ID модуля в имя файла gps
// Заголовок файла трека
#define TRACK_HEAD "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
                   "<gpx version=\"1.0\" creator=\"ESP32 GPS logger\" xmlns=\"http://www.topografix.com/GPX/1/0\">\n"
// Эпилог файла трека
#define TRACK_EPILOG "</trkseg>\n</trk>\n</gpx>\n"
// #define TRACK_END "</trkseg>\n</trk>\n"

// ====================================================================================
// Секция объявлений структур
// ====================================================================================

// Структура конфигурации
struct fileConfig
{
  char ssid[15];       // SSID точки доступа для подключения
  char wifiPasswd[15]; // Пароль точки доступа
  char ftpAddress[16]; // IP адрес FTP сервера
  char ftpUser[15];    // Имя пользователя на FTP сервере
  char ftpPasswd[15];  // Пароль пользователя на FTP сервере
  uint32_t filesize;   // Максимальный размер файла WiFi
  bool gpssend;        // Флаг отправки файлов треков на FTP сервер
  bool wifisend;       // Флаг отправки файлов WiFi на FTP сервер
  bool ftpbeep;        // Флаг включения звука
  uint16_t ftpPort;    // Порт FTP сервера
  char moduleID[3];    // Идентификатор устройства
  uint16_t pAccMask;   // Position Accuracy Mask to use
  uint16_t pDopMask;   // Position DOP Mask to use
};

// Структура для имен папок
struct Folders
{
  char work[9];  // Папка для текущего файла, в который идет запись
  char ready[9]; // Папка для файлов, готовых к отправке на FTP
  char saved[9]; // Папка для файлов, отправленных на FTP
};

// Данные позиции для WiFi (еденицы приведены к градусам и метрам)
struct wifiPos
{
  bool isValid;       // Позиция валидна
  int32_t wifiLong;   // Долгота для Wifi: deg *
  int32_t wifiLat;    // Широта для WiFi: deg *
  int32_t wifiAlt;    // Высота над эллипсоидом для WiFi: mm
  uint32_t wifiAccur; // Ожидаемая горизонтальная точность: mm
};                    // wifiPos

// ====================================================================================
// Секция объявлений флагов
// ====================================================================================

#define BIT_G1_0 (1 << 0)   // Папки очищены
#define BIT_G1_1 (1 << 1)   // Дата и время валидны
#define BIT_G1_2 (1 << 2)   // Позиция валидна для трека
#define BIT_G1_3 (1 << 3)   // Дата для имен файлов сформирована
#define BIT_G1_4 (1 << 4)   // Время 1 сформировано
#define BIT_G1_5 (1 << 5)   // Время 2 сформировано
#define BIT_G1_6 (1 << 6)   // Системное время установлено
#define BIT_G1_7 (1 << 7)   // Папки проверены
#define BIT_G1_8 (1 << 8)   // Файл трека создан
#define BIT_G1_9 (1 << 9)   // Файл WiFi создан
#define BIT_G1_10 (1 << 10) // Доступна точка доступа для отправки на FTP сервер

#endif
