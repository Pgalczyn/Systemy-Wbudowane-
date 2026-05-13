#pragma once
#include <Arduino.h>

void handleWebSerialInput(const String& msg);
String readStringWithEcho(const char* prompt, bool echo = true);
