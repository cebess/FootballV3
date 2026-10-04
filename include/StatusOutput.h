#pragma once

#include <Arduino.h>

void statusPrintLine(const String& text);
void eventPrintLine(const String& text);
void lcdPrintLine(uint8_t row, const String& text);