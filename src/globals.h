#pragma once
#include <Arduino.h>
#include <MFRC522.h>

#define SS_PIN 21
#define RST_PIN 22
#define BOOT_BTN 0

extern MFRC522 mfrc522;
extern MFRC522::MIFARE_Key key;
class AsyncWebServer;
extern AsyncWebServer server;
void consolePrintf(const char* format, ...);
void setupWebConsole();

enum DeviceMode
{
    MODE_NONE,
    MODE_RECEPTION,
    MODE_GATE
};
extern DeviceMode currentMode;
