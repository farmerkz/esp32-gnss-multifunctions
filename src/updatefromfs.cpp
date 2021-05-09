#include "main.h"

extern bool performUpdate(Stream &updateSource, size_t updateSize);
extern void fatalError(uint8_t n, const char *event, bool _deepsl = true);

/** @brief Проверяем указанную файловую систему на наличие firmware.bin 
 * и, если имеется, запускаем обновление.
 * Если обновление удачно - удаляем файл и перезагружаем ESP32
 * @param fs Файловая система
 */
void updateFromFS(fs::FS &fs)
{
  bool res = false;
  File updateBin = fs.open(FIRMWARE);
  if (updateBin)
  {
    if (updateBin.isDirectory())
    {
      // Serial.println("Error, firmware.bin is not a file");
      fatalError(0, "Error, " FIRMWARE " is not a file", false);
      updateBin.close();
      return;
    }

    size_t updateSize = updateBin.size();

    if (updateSize > 0)
    {
      // Serial.println("Try to start update");
      fatalError(0, "Try to start update from " FIRMWARE, false);
      for (int i = 0; i < 2; i++)
      {
        ledcWriteTone(BUZZER_CHAN, 800);
        delay(100);
        ledcWriteTone(BUZZER_CHAN, 0);
        delay(100);
      }
      res = performUpdate(updateBin, updateSize);
    }
    else
    {
      // Serial.println("Error, file is empty");
      fatalError(0, "Error, file " FIRMWARE " is empty");
    }

    updateBin.close();

    if (res)
    {
      fs.remove(FIRMWARE);
      fatalError(0,"Firmware successfully updated",false);
      delay(100);
      for (int i = 0; i < 5; i++)
      {
        ledcWriteTone(BUZZER_CHAN, 500);
        delay(100);
        ledcWriteTone(BUZZER_CHAN, 0);
        delay(100);
      }
      SD.end();
      delay(1000);
      ESP.restart();
    }
    else
    {
      fatalError(0,"Error, firmware not updated",false);
      ledcWriteTone(BUZZER_CHAN, 700);
      delay(5000);
      ledcWriteTone(BUZZER_CHAN, 0);
    }
  }
  else
  {
    // Serial.println("Could not load update.bin from sd root");
  }
} // updateFromFS()
