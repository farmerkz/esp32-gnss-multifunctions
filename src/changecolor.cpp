#include "main.h"

/** @brief Установка цвета трехцветного светодиода
 * @param red яркость красного диода
 * @param green яркость зеленого диода
 * @param blue яркость голубого диода
 * */
void changeColor(uint _red, uint _green, uint _blue)
{
  ledcWrite(RED_CHAN, _red);
  ledcWrite(GREEN_CHAN, _green);
  ledcWrite(BLUE_CHAN, _blue);
} // changeColor()
