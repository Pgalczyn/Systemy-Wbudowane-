#pragma once
#include <Arduino.h>

void telnetPrintFmt(const char *format, ...);
bool getCommand(String &result);
void handleTelnet();
bool isTelnetConnected();