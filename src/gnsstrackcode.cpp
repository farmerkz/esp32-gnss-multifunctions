#include "main.h"

extern EventGroupHandle_t eventGroup_1;
extern xQueueHandle queueGnssTrack;
extern volatile xSemaphoreHandle sdMutex;
extern File trackFile;
// extern const double mm2m;
// extern const double en7;
// extern const double en5;
extern uint32_t loopTaskHandleStack;
// extern uint32_t gnssTaskStack;
extern uint32_t trackTaskStack;
extern uint32_t wifiTaskStack;
// extern uint32_t checkFixStatusTaskStack;
extern uint32_t createFilesTaskStack;
extern uint32_t ftpSendTaskStack;
extern void fatalError(void);

// Запись точки трека в файл при наличии установленного флага BIT_G1_2 (читаем - сбрасываем)
// Формирование точки и запись производится только при установленном флаге BIT_G1_8
void gnssTrackCode(void *pvParameters)
{

  EventBits_t eventBits;
  uint32_t filePos;
  char trackPoint[350];
  UBX_NAV_PVT_data_t _gnssData;

  for (;;)
  {
    size_t writedBytes;

    xEventGroupWaitBits(eventGroup_1, BIT_G1_2, pdTRUE, pdTRUE, portMAX_DELAY);
    eventBits = xEventGroupGetBits(eventGroup_1);
    if ((eventBits & BIT_G1_8) == BIT_G1_8)
    {
      xQueuePeek(queueGnssTrack, &_gnssData, 1200UL / portTICK_RATE_MS);

      snprintf(trackPoint, sizeof(trackPoint), "<trkpt lat=\"%2.7f\" lon=\"%2.7f\" >"
                                               "<time>%04d-%02d-%02dT%02d:%02d:%02dZ</time>"
                                               "<ele>%.2f</ele>"
                                               "<speed>%.2f</speed>"
                                               "<course>%.2f</course>"
                                               "<sat>%i</sat>"
                                               "<pdop>%.2f</pdop>"
                                               "<hmsl>%.2f</hmsl>"
                                               "<fix>3d</fix>"
                                               "<hacc>%.2f</hacc>"
                                               "<vacc>%.2f></vacc>"
                                               "<heapmem>%d</heapmem>"
                                               "<stack>%s:%d:%d:%d:%d:%d</stack>"
                                               "</trkpt>\n" TRACK_EPILOG,
               _gnssData.lat * en7,
               _gnssData.lon * en7,
               _gnssData.year,
               _gnssData.month,
               _gnssData.day,
               _gnssData.hour,
               _gnssData.min,
               _gnssData.sec,
               _gnssData.height * mm2m,
               _gnssData.gSpeed * mm2m,
               _gnssData.headMot * en5,
               _gnssData.numSV,
               _gnssData.pDOP * 0.01,
               _gnssData.hMSL * mm2m,
               _gnssData.hAcc * mm2m,
               _gnssData.vAcc * mm2m,
               ESP.getFreeHeap(),
               IS_DEBUG,
               loopTaskHandleStack,
               trackTaskStack,
               wifiTaskStack,
               createFilesTaskStack,
               ftpSendTaskStack);

      xSemaphoreTake(sdMutex, portMAX_DELAY);
      filePos = (uint32_t)trackFile.size() - (uint32_t)SEEK_TRKPT_BACKWARDS;
      trackFile.seek(filePos);
      writedBytes = trackFile.print(trackPoint);
      trackFile.flush();
      xSemaphoreGive(sdMutex);
      if (writedBytes <= 0)
      {
        fatalError();
      }
    }
    delay(DELAY_GNSS_TRACK_CODE);
  }
} // gnssTrackCode()
