#pragma once
#include <Arduino.h>
#include <MFRC522.h>
#include <SimpleSerialShell.h>

#define SS_PIN 21
#define RST_PIN 22

extern MFRC522 mfrc522;
extern MFRC522::MIFARE_Key key;

enum DeviceMode
{
    MODE_NONE,
    MODE_RECEPTION,
    MODE_GATE
};
extern DeviceMode currentMode;
