#include "main.h"

extern fileConfig config;
extern SFE_UBLOX_GNSS myGNSS;

bool setfixmask(void)
{
    uint16_t _pdopmask;
    uint8_t customPayload[MAX_PAYLOAD_SIZE];
    ubxPacket customCfg = {0,
                           0,
                           0,
                           0,
                           0,
                           customPayload,
                           0,
                           0,
                           SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED,
                           SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};
    if (config.pDopMask == 0 && config.pAccMask == 0) // Проверяем маски
        return true;                                  // Если не определены - ничего не делаем

    customCfg.cls = UBX_CLASS_CFG;                                                 // message Class
    customCfg.id = UBX_CFG_NAV5;                                                   // message ID
    customCfg.len = 0;                                                             // Setting the len (length) to zero let's us poll the current settings
    customCfg.startingSpot = 0;                                                    // Always set the startingSpot to zero (unless you really know what you are doing)
    uint16_t maxWait = 1100;                                                       // Устанавливаем таймаут 1100 миллисекунд
    if (myGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // Ожидаем данные и ACK
        return (false);
    if (config.pDopMask != 0)
    {
        _pdopmask = config.pDopMask * 10;
        customPayload[14] = _pdopmask & 0xFF;
        customPayload[15] = _pdopmask >> 8;
    }
    if (config.pAccMask != 0)
    {
        customPayload[18] = config.pAccMask & 0xFF;
        customPayload[19] = config.pAccMask >> 8;
    }
    customPayload[0] = 0x10;
    customPayload[1] = 0x00;
    customCfg.len = 36;
    customCfg.startingSpot = 0;
    return (myGNSS.sendCommand(&customCfg, maxWait) == SFE_UBLOX_STATUS_DATA_SENT); // Ожидаем только ACK
}