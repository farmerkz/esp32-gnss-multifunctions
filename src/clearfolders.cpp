#include "main.h"
#include "commonexternal.h"

extern Folders gpsFlDir;
extern Folders wifiFlDir;

/** @brief Проверка рабочих папок, если там есть файлы - перемещение их в ready.
 * Мьютексы не используются, при необходимости - необходимо использовать до и после вызова
 * @param gps очистка папок GPS
 * @param wifi очистка парок WiFi
 */
void clearFolders(bool _gps, bool _wifi)
{
  File entry;
  File inDir;
  String newFl;

  logging("Checking working folders\n", false);

  if (_gps && SD.exists(gpsFlDir.work) && SD.exists(gpsFlDir.ready))
  {
    if (!(inDir = SD.open(gpsFlDir.work)))
    {
      fatalError();
    }
    while (entry = inDir.openNextFile())
    {
      if (entry.size() <= 500U)
      {
        if (!SD.remove(entry.name()))
        {
          fatalError();
        }
        logging("%s deleted: file size less than 500 bytes\n", entry.name(), false);
        continue;
      }

      newFl = (String)gpsFlDir.ready + ((String)entry.name()).substring(GPS_SUBSTR_1);
      if (!SD.rename((String)entry.name(), newFl))
      {
        fatalError();
      }
    }
    inDir.close();
  }

  if (_wifi && SD.exists(wifiFlDir.work) && SD.exists(wifiFlDir.ready))
  {
    if (!(inDir = SD.open(wifiFlDir.work)))
    {
      fatalError();
    }
    while (entry = inDir.openNextFile())
    {
      if (entry.size() <= 500U)
      {
        if (!SD.remove(entry.name()))
        {
          fatalError();
        }
        logging("%s deleted: file size less than 500 bytes\n", entry.name(), false);
        continue;
      }
      newFl = (String)wifiFlDir.ready + ((String)entry.name()).substring(WIFI_SUBSTR_1);
      if (!SD.rename((String)entry.name(), newFl))
      {
        fatalError();
      }
    }
    inDir.close();
  }
  logging("Checking working folders finished\n", false);
} // clearFolders()
