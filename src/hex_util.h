#pragma once
#include <Arduino.h>

String bytesToHex(const uint8_t* bytes, size_t length);
void hexToBytes(const String& hexStr, uint8_t* outBytes, size_t length);