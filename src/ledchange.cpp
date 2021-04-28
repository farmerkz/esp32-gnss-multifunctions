#include "main.h"

/** @brief Устанавливаем яркость одного из каналов/светодиодов
 * @param color Канал/светодиод: R/r, G/g, B/b
 * @param light Яркость
 */
void ledChange(char _color, uint _light)
{

    if (_color == 'r' || _color == 'R')
    {
        ledcWrite(RED_CHAN, _light);
    }
    else if (_color == 'g' || _color == 'G')
    {
        ledcWrite(GREEN_CHAN, _light);
    }
    else if (_color == 'b' || _color == 'B')
    {
        ledcWrite(BLUE_CHAN, _light);
    }
}