#include "main.h"

/** @brief Выполняем обновление из файла
 * @param updateSource stream, из котрого читается обновление
 * @param updateSize размер обновления, байт
 * @return true, если обновление удачно, false, если ошибка
 */
bool performUpdate(Stream &updateSource, size_t updateSize)
{
  bool updResult;
  if (Update.begin(updateSize))
  {
    size_t written = Update.writeStream(updateSource);
    if (written == updateSize)
    {
      // Serial.println("Written : " + String(written) + " successfully");
    }
    else
    {
      // Serial.println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
    }
    if (Update.end())
    {
      // Serial.println("OTA done!");
      if (Update.isFinished())
      {
        // Serial.println("Update successfully completed. Rebooting.");
        updResult = true;
      }
      else
      {
        // Serial.println("Update not finished? Something went wrong!");
        updResult = false;
      }
    }
    else
    {
      // Serial.println("Error Occurred. Error #: " + String(Update.getError()));
      updResult = false;
    }
  }
  else
  {
    // Serial.println("Not enough space to begin OTA");
    updResult = false;
  }
  return updResult;
} // performUpdate()
