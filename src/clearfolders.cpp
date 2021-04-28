#include "main.h"

extern void fatalError();

extern Folders gpsFlDir;
extern Folders wifiFlDir;

/** @brief Проверка рабочих папок, если там есть файлы - перемещение их в ready.
 * Мьютексы не используются, при необходимости - использовать до и после вызова
 * @param gps очистка папок GPS
 * @param wifi очистка парок WiFi
 */
void clearFolders(bool _gps, bool _wifi)
{

  // Текущее имя файла из папки
  File entry;

  //
  File inDir;
  String newFl;

  if (_gps && SD.exists(gpsFlDir.work) && SD.exists(gpsFlDir.ready))
  {
    if (!(inDir = SD.open(gpsFlDir.work)))
    {
      fatalError();
    }
    while (entry = inDir.openNextFile())
    {
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
      newFl = (String)wifiFlDir.ready + ((String)entry.name()).substring(WIFI_SUBSTR_1);
      if (!SD.rename((String)entry.name(), newFl))
      {
        fatalError();
      }
    }
    inDir.close();
  }
} // clearFolders()
