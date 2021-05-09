#include "main.h"

extern void changeColor(uint _red, uint _green, uint _blue);
extern File logFile;

/** @brief Фатальная ошибка, мигаем красным светодиодом LED_RED_FATAL_BLINK раз и уходим
 * в Deep Sleep на DEEP_SLEEP_TIME
 */
void fatalError()
{
  disableLoopWDT();
  for (int i = 0; i < (LED_RED_FATAL_BLINK); i++)
  {
    ledcWriteTone(BUZZER_CHAN, 700);
    for (uint j = 0; j <= 255; j++)
    {
      changeColor(j, 0, 0);
      delay(1);
    }
    ledcWriteTone(BUZZER_CHAN, 0);
    for (uint j = 0; j <= 255; j++)
    {
      changeColor((uint)255 - j, 0, 0);
      delay(1);
    }
  }
  ledcWriteTone(BUZZER_CHAN, 0);
  esp_bluedroid_disable();
  esp_bt_controller_disable();
  esp_wifi_stop();
  ESP.deepSleep(DEEP_SLEEP_TIME);
} // fatalError()

/** @brief Ошибка. Мигаем красным светодиодом N раз, пишем
 * в файл error.log сообщение и, если установлен флаг, уходим
 * в Deep Sleep на DEEP_SLEEP_TIME
 * @param n количество циклов моргания светодиодом и писков
 * @param event строка для записи в файл error.log
 * @param deepsl флаг ухода в deep sleep, по умолчанию true
 */
void fatalError(uint8_t _n, const char *_event, bool _deepsl = true)
{
  disableLoopWDT();
  logFile.println(_event);
  logFile.flush();
  for (int i = 0; i < _n; i++)
  {
    ledcWriteTone(BUZZER_CHAN, 700);
    for (uint j = 0; j <= 255; j++)
    {
      changeColor(j, 0, 0);
      delay(1);
    }
    ledcWriteTone(BUZZER_CHAN, 0);
    for (uint j = 0; j <= 255; j++)
    {
      changeColor((uint)255 - j, 0, 0);
      delay(1);
    }
  }
  ledcWriteTone(BUZZER_CHAN, 0);
  if (_deepsl)
  {
    esp_bluedroid_disable();
    esp_bt_controller_disable();
    esp_wifi_stop();
    ESP.deepSleep(DEEP_SLEEP_TIME);
  }
}
