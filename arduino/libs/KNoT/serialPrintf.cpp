#include <Arduino.h>
#include "serialPrintf.h"

void serialPrintf(char *fmt, ...){
        char buf[256]; // resulting string limited to 128 chars
        va_list args;
        va_start (args, fmt);
        vsnprintf(buf, 256, fmt, args);
        va_end (args);
        Serial.print(buf);
}