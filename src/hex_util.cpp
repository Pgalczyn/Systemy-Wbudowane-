#pragma once
#include <Arduino.h>
#include "hex_util.h"

String bytesToHex(const uint8_t* bytes, size_t length) {
    String hex = "";
    hex.reserve(length * 2);
    for (size_t i = 0; i < length; i++) {
        if (bytes[i] < 0x10) hex += "0";
        hex += String(bytes[i], HEX);
    }
    hex.toUpperCase();
    return hex;
}

void hexToBytes(const String& hexStr, uint8_t* outBytes, size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (i * 2 + 2 <= hexStr.length()) {
            String byteStr = hexStr.substring(i * 2, i * 2 + 2);
            outBytes[i] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
        } else {
            outBytes[i] = 0;
        }
    }
}