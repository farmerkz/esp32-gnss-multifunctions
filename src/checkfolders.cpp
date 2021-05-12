#include "main.h"
#include "commonexternal.h"

extern Folders gpsFlDir;
extern Folders wifiFlDir;

/** @brief Проверка наличия всех папок, при необходимости - их создание
 */
void checkFolders(void)
{
  logging("Checking folders\n", false);
  if (!SD.exists(gpsFlDir.work))
  {
    if (!SD.mkdir(gpsFlDir.work))
      fatalError();
    else
      logging("%s folder created\n", gpsFlDir.work, false);
  }
  if (!SD.exists(gpsFlDir.ready))
  {
    if (!SD.mkdir(gpsFlDir.ready))
      fatalError();
    else
      logging("%s folder created\n", gpsFlDir.ready, false);
  }
  if (!SD.exists(gpsFlDir.saved))
  {
    if (!SD.mkdir(gpsFlDir.saved))
      fatalError();
    else
      logging("%s folder created\n", gpsFlDir.saved, false);
  }
  if (!SD.exists(wifiFlDir.work))
  {
    if (!SD.mkdir(wifiFlDir.work))
      fatalError();
    else
      logging("%s folder created\n", wifiFlDir.work, false);
  }
  if (!SD.exists(wifiFlDir.ready))
  {
    if (!SD.mkdir(wifiFlDir.ready))
      fatalError();
    else
      logging("%s folder created\n", wifiFlDir.ready, false);
  }
  if (!SD.exists(wifiFlDir.saved))
  {
    if (!SD.mkdir(wifiFlDir.saved))
      fatalError();
    else
      logging("%s folder created\n", wifiFlDir.saved, false);
  }
  logging("Folder check completed\n", false);
} // checkFolders()
