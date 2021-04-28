#include "main.h"

extern void fatalError();
extern Folders gpsFlDir;
extern Folders wifiFlDir;

/** @brief Проверка наличия всех папок, при необходимости - их создание
 */
void checkFolders(void)
{
  if (!SD.exists(gpsFlDir.work))
  {
    if (!SD.mkdir(gpsFlDir.work))
      fatalError();
  }
  if (!SD.exists(gpsFlDir.ready))
  {
    if (!SD.mkdir(gpsFlDir.ready))
      fatalError();
  }
  if (!SD.exists(gpsFlDir.saved))
  {
    if (!SD.mkdir(gpsFlDir.saved))
      fatalError();
  }
  if (!SD.exists(wifiFlDir.work))
  {
    if (!SD.mkdir(wifiFlDir.work))
      fatalError();
  }
  if (!SD.exists(wifiFlDir.ready))
  {
    if (!SD.mkdir(wifiFlDir.ready))
      fatalError();
  }
  if (!SD.exists(wifiFlDir.saved))
  {
    if (!SD.mkdir(wifiFlDir.saved))
      fatalError();
  }
} // checkFolders()