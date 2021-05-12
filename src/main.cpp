#include "main.h"
#include "commonexternal.h"

// ====================================================================================
// Секция объявлений переменных
// ====================================================================================

// Уточнить, нужны ли глобальные переменные
// UBX_NAV_PVT_data_t gnssTrackData;                         // Позиция для трека
// wifiPos wifiPosData;                                      // Позиция для WiFi
//

SFE_UBLOX_GNSS myGNSS;                           // Экземпляр класса - наш модуль
HardwareSerial gpsSerial(2);                     // Для подключения GNSS модуля используем UART2
fileConfig config;                               // Конфигурационный файл
Folders gpsFlDir = {GPS_WK, GPS_RD, GPS_SD};     // Имена папок для треков
Folders wifiFlDir = {WIFI_WK, WIFI_RD, WIFI_SD}; // Имена папок для WiFi
EventGroupHandle_t eventGroup_1;                 // Эвент-группа для разных флагов
volatile xSemaphoreHandle sdMutex;               // Мьютекс доступа к SD карте
volatile xSemaphoreHandle wifiMutex;             // Мьютекс доступа к WiFi
File trackFile;                                  // Текущий файл трека
File wifiFile;                                   // Текущий файл WiFi
File logFile;                                    // Файл лога
TaskHandle_t createFilesTask = NULL;             // Хэндл задачи создания всех файлов
TaskHandle_t trackTask = NULL;                   // Хэндл задачи записи точки трека
TaskHandle_t wifiTask = NULL;                    // Хэндл задачи сканирования WiFi и записи в файл
TaskHandle_t ftpSendTask = NULL;                 // Хэндл задачи отправки файлов на FTP сервер
extern TaskHandle_t loopTaskHandle;              // Хэндл основного цикла Arduino
uint32_t createFilesTaskStack = 0;               // Размер стека задачи
uint32_t trackTaskStack = 0;                     // Размер стека задачи
uint32_t wifiTaskStack = 0;                      // Размер стека задачи
uint32_t ftpSendTaskStack = 0;                   // Размер стека задачи
uint32_t loopTaskHandleStack = 0;                // Размер стека задачи

// Флаг конфигурирования модуля GNSS, хранится в RTC памяти
// После настройки модуля GNSS устанавливаем его,
// чтобы не настраивать модуль повторно после выхода из
// DeepSleep
RTC_DATA_ATTR bool gnssConf = false;

// ------------------------------------
// Очереди
// ------------------------------------

xQueueHandle queueGnssTrack;    // Данные позиции для трека
xQueueHandle queueGnssWiFi;     // Данные позиции для WiFi
xQueueHandle queueFileDateName; // Дата для имени файла
xQueueHandle queueFileTimeName; // Время для имени файла
xQueueHandle queueTrackTime;    // Время для трека и WiFi

// ====================================================================================
// Секция объявлений подпрограмм
// ====================================================================================

extern void changeColor(uint _red, uint _green, uint _blue);
extern void readConf(char *_filename);
extern bool performUpdate(Stream &updateSource, size_t updateSize);
extern void updateFromFS(fs::FS &fs);
extern void getPVTdata(UBX_NAV_PVT_data_t ubxDataStruct);
extern void clearFolders(bool gps, bool wifi);
extern void checkFolders(void);
extern void createAllFiles(void *pvParameters);
extern void gnssTrackCode(void *pvParameters);
extern void wifiCode(void *pvParameters);
extern void ftpTask(void *pvParameters);
extern void ledChange(char color, uint light);
extern bool setfixmask(void);

// ====================================================================================
// Секция кода
// ====================================================================================

void setup()
{
  // ====================================================================================
  // Секция объявлений переменных
  // ====================================================================================

  // esp_sleep_wakeup_cause_t wakeup_reason;
  esp_reset_reason_t reset_reason;
  bool baudOK = false;
  unsigned long detectedBaudRate = 0;

  // ====================================================================================
  // Секция кода
  // ====================================================================================

  // Запрашиваем причину загрузки.
  // wakeup_reason = esp_sleep_get_wakeup_cause();
  reset_reason = esp_reset_reason();

  // Пауза 0,5 сек для завершения инициализации всех модулей
  delay(500);

  // Включаем watcdog основного цикла
  enableLoopWDT();

  // Инициализация WiFi
  WiFi.persistent(false);
  WiFi.begin();
  WiFi.disconnect();

  // Инициализация пинов для RGB светодиода и пищалки
  ledcSetup(RED_CHAN, 5000, 8);
  ledcSetup(GREEN_CHAN, 5000, 8);
  ledcSetup(BLUE_CHAN, 5000, 8);
  ledcSetup(BUZZER_CHAN, 8000, 12);
  ledcAttachPin(RED_PIN, RED_CHAN);
  ledcAttachPin(GREEN_PIN, GREEN_CHAN);
  ledcAttachPin(BLUE_PIN, BLUE_CHAN);
  ledcAttachPin(BUZZER_PIN, BUZZER_CHAN);

  // Включаем светодиод для индикации инициализации, коротко пискаем
  changeColor(24, 4, 4);
  ledcWriteTone(BUZZER_CHAN, 1200);
  delay(100);
  ledcWriteTone(BUZZER_CHAN, 0);

  // ************************************************************************************
  // Блок инициализации SD (с файлами здесь еще пока не работаем)
  // ------------------------------------------------------------------------------------
  if (!SD.begin())
  {
    fatalError();
  }
  if (SD.cardType() == CARD_NONE)
  {
    fatalError();
  }
  // ------------------------------------------------------------------------------------
  // SD инициирован
  // ************************************************************************************

  // ************************************************************************************
  // Открытие файла лога
  // ------------------------------------------------------------------------------------
  logFile = SD.open(LOG_FILE, "a");
  if (!logFile)
  {
    fatalError();
  }
  // ------------------------------------------------------------------------------------
  // файл лога открыт
  // ************************************************************************************

  // ************************************************************************************
  // Определяем причину загрузки и пишем в лог
  // ------------------------------------------------------------------------------------
  logging("\n========== Start logging ==========\n", false);
  logging("Reset reason is: ", false);
  switch (reset_reason)
  {
  case ESP_RST_POWERON:
    logFile.print("Reset due to power-on event\n");
    gnssConf = false;
    break;

  case ESP_RST_INT_WDT:
  case ESP_RST_TASK_WDT:
  case ESP_RST_WDT:
    logFile.print("Reset due watchdog\n");
    gnssConf = false;
    break;

  case ESP_RST_DEEPSLEEP:
    logFile.print("Reset after exiting deep sleep mode\n");
    break;

  case ESP_RST_PANIC:
    logFile.print("Software reset due to exception/panic\n");
    gnssConf = false;
    break;

  case ESP_RST_BROWNOUT:
    logFile.print("Brownout reset (software or hardware)\n");
    gnssConf = false;
    break;

  case ESP_RST_SW:
    logFile.print("Software reset via esp_restart\n");
    gnssConf = false;
    break;

  default:
    logFile.print("Reset reason can not be determined\n");
    gnssConf = false;
    break;
  }
  // ------------------------------------------------------------------------------------
  // Причина загрузки выведена в лог
  // ************************************************************************************

  // ************************************************************************************
  // Загрузка конфигурации
  // ------------------------------------------------------------------------------------
  changeColor(0, 128, 0);
  readConf((char *)CONF_FILE_NAME);
  changeColor(0, 0, 0);
  // ------------------------------------------------------------------------------------
  // Конфигурация загружена
  // ************************************************************************************

  // ************************************************************************************
  // Проверяем наличие обновления и, при наличии, обновляемся
  // ------------------------------------------------------------------------------------
  ledChange('B', 128);
  disableLoopWDT();
  updateFromFS(SD);
  enableLoopWDT();
  ledChange('B', 0);
  // ------------------------------------------------------------------------------------
  // С обновлением закончили
  // ************************************************************************************

  // ************************************************************************************
  // Блок настройки GNSS модуля
  // ------------------------------------------------------------------------------------
  // Определяем скорость порта GNSS
  ledChange('G', 128);
  logging("Detect the port speed of GNSS\n", false);
  while (!baudOK)
  {
    gpsSerial.begin(0, SERIAL_8N1, 16, 17, false, 10000UL);
    detectedBaudRate = abs((gpsSerial.baudRate() >> 4) << 4);
    if ((detectedBaudRate > 0) && (detectedBaudRate < 115207UL))
    {
      baudOK = true;
      delay(10);
    }
    else
    {
      baudOK = false;
      delay(1000);
    }
    gpsSerial.end();
  }

  logging("Detected baud rate: %d\n", detectedBaudRate, false);

  if (detectedBaudRate != 115200UL)
  {
    gpsSerial.begin(detectedBaudRate, SERIAL_8N1, 16, 17);
    delay(10);
    if (myGNSS.begin(gpsSerial) == false)
    {
      logging("ERROR myGNSS.begin at the baud rate: %d\n", detectedBaudRate, false);
      fatalError(5);
    }
    myGNSS.setSerialRate(115200, 1);
    gpsSerial.end();
  }
  delay(20);
  gpsSerial.begin(115200);
  if (myGNSS.begin(gpsSerial) == false)
  {
    logging("ERROR myGNSS.begin at the baud rate: 115200\n", false);
    fatalError(5);
  }
  else
  {
    logging("GNSS port speed set to 115200\n", false);
  }

  // Значение gnssConf хранится в RTC памяти, после выхода из DeepSleep модуль GNSS
  // не настраиваем, если он уже был ранее настроен
  if (!gnssConf)
  {
    myGNSS.setUART1Output(COM_TYPE_UBX);
    myGNSS.setDynamicModel(DYN_MODEL_PEDESTRIAN);
    myGNSS.setNavigationFrequency(1);
    if (config.pDopMask != 0 || config.pAccMask != 0)
      if (!setfixmask())
      {
        logging("Accuracy mask setting error\n", false);
        fatalError(4);
      }
    gnssConf = true; // Выставляем флаг "Модуль GNSS настроен"
  }

  // Разрешаем Auto PVT сообщения, используем callback
  myGNSS.setAutoPVTcallback(&getPVTdata);
  logging("GNSS module is OK\n", false);

  // ------------------------------------------------------------------------------------
  // Модуль GNSS настроен
  // ************************************************************************************

  // ************************************************************************************
  // создаем очереди, мьютексы, event группы
  // ------------------------------------------------------------------------------------

  logging("Create queues, mutexes, event groups\n", false);
  eventGroup_1 = xEventGroupCreate();                           // Event группа 1
  xEventGroupClearBits(eventGroup_1, (uint32_t)0x0FFFFFF);      // Обнуляем все флаги
  queueGnssWiFi = xQueueCreate(1, sizeof(wifiPos));             // Очередь данных о позиции для WiFi
  queueGnssTrack = xQueueCreate(1, sizeof(UBX_NAV_PVT_data_t)); // Очередь данных позиции для трека
  queueFileDateName = xQueueCreate(1, sizeof(DATE_MASK));       // Очередь для даты, используемой в именах
  queueFileTimeName = xQueueCreate(1, sizeof(TIME_MASK_1));     // Очередь для времени, используемого в имени файла
  queueTrackTime = xQueueCreate(1, sizeof(TIME_MASK_2));        // Очередь для времени, используемого в треках и WiFi
  sdMutex = xSemaphoreCreateMutex();                            // Мьютекс для доступа к SD
  wifiMutex = xSemaphoreCreateMutex();                          // Мьютекс для доступа к WiFi

  // ------------------------------------------------------------------------------------
  // Закончили с очередями и мьютексами
  // ************************************************************************************

  // ************************************************************************************
  // Перемещаем файлы из рабочих папок (при наличии всех необходимых папок)
  // ------------------------------------------------------------------------------------

  clearFolders(true, true);
  xEventGroupSetBits(eventGroup_1, BIT_G1_0);

  // ------------------------------------------------------------------------------------
  // Закончили с перемещением папок
  // ************************************************************************************

  // ************************************************************************************
  // создаем задачи
  // ------------------------------------------------------------------------------------

  logging("Create tasks\n", false);

  xTaskCreatePinnedToCore(
      createAllFiles,
      "Create_Files",
      4000,
      (void *)&SD,
      2,
      &createFilesTask,
      1);

  xTaskCreatePinnedToCore(
      gnssTrackCode,
      "TrackTask",
      4000,
      NULL,
      2,
      &trackTask,
      1);

  xTaskCreatePinnedToCore(
      wifiCode,
      "WiFi_Task",
      6000,
      NULL,
      2,
      &wifiTask,
      1);

  xTaskCreatePinnedToCore(
      ftpTask,
      "FTP_Send",
      5000,
      NULL,
      2,
      &ftpSendTask,
      1);

  // ------------------------------------------------------------------------------------
  // Задачи созданы, инициализация полностью закончена
  // ************************************************************************************

  changeColor(0, 0, 0);
  logging("Setup is done\n\n", false);

  // ====================================================================================
  // Инициализация полностью закочена
  // ====================================================================================
} // setup()

void loop()
{
  myGNSS.checkUblox();     // Check for the arrival of new data and process it.
  myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.
  loopTaskHandleStack = (uint32_t)uxTaskGetStackHighWaterMark(loopTaskHandle);
  trackTaskStack = (uint32_t)uxTaskGetStackHighWaterMark(trackTask);
  wifiTaskStack = (uint32_t)uxTaskGetStackHighWaterMark(wifiTask);
  createFilesTaskStack = (uint32_t)uxTaskGetStackHighWaterMark(createFilesTask);
  ftpSendTaskStack = (uint32_t)uxTaskGetStackHighWaterMark(ftpSendTask);

  delay(10);
}
