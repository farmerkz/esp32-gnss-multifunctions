#include "main.h"

extern File logFile;
extern volatile xSemaphoreHandle sdMutex;

/** @brief Выводим в файл лога системные дату и время
 */
size_t _dataTime(void)
{
    size_t sz;
    struct timeval tv;
    time_t t;
    char bf[] = "MMM DD HH:MM:SS";
    gettimeofday(&tv, NULL);
    t = tv.tv_sec;
    strftime(bf, sizeof(bf), "%b %d %H:%M:%S", localtime(&t));
    sz = logFile.printf("%s ", bf);
    return sz;
}

/** @brief Выводим в файл лога строку.
 * @param ft Строка для вывода
 * @param sdmute Флаг использования мьютекса для доступа к SD. По умолчанию - использовать
 */
size_t logging(const char *ft, bool sdmute = true)
{
    size_t sz;
    if (sdmute)
    {
        xSemaphoreTake(sdMutex, portMAX_DELAY);
    }
    sz = _dataTime();
    sz += logFile.printf(ft);
    logFile.flush();
    if (sdmute)
    {
        xSemaphoreGive(sdMutex);
    }
    return sz;
}

/** @brief Выводим в файл лога строку формата и значение переменной.
 * @param ft Строка формата
 * @param var Значение для вывода
 * @param sdmute Флаг использования мьютекса для доступа к SD. По умолчанию - использовать
 */
size_t logging(const char *ft, uint32_t var, bool sdmute = true)
{
    size_t sz;
    if (sdmute)
    {
        xSemaphoreTake(sdMutex, portMAX_DELAY);
    }
    sz = _dataTime();
    sz += logFile.printf(ft, var);
    logFile.flush();
    if (sdmute)
    {
        xSemaphoreGive(sdMutex);
    }
    return sz;
}

/** @brief Выводим в файл лога строку формата и значение переменной типа массив.
 * @param ft Строка формата
 * @param var Значение для вывода
 * @param sdmute Флаг использования мьютекса для доступа к SD. По умолчанию - использовать
 */
size_t logging(const char *ft, const char *var, bool sdmute = true)
{
    size_t sz;
    if (sdmute)
    {
        xSemaphoreTake(sdMutex, portMAX_DELAY);
    }
    sz = _dataTime();
    sz += logFile.printf(ft, var);
    logFile.flush();
    if (sdmute)
    {
        xSemaphoreGive(sdMutex);
    }
    return sz;
}

