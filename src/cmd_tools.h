#pragma once
#include <Arduino.h>

void logMsg(const char *format, ...);
bool getCommand(String &result);