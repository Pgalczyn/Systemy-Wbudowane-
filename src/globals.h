#pragma once
#include <Arduino.h>
#include <MFRC522.h>
#include <WiFiManager.h>

#define SS_PIN 21
#define RST_PIN 22
#define BOOT_BTN 0

#define PREFS_APP "app"
#define PARAM_WORK_MODE "work_mode"
#define PARAM_SERVER_ADDR "server_addr"
#define DEFAULT_WORK_MODE "GATE"
#define DEFAULT_API_ADDR "http://192.168.1.100:5000/api"

extern MFRC522 mfrc522;
extern MFRC522::MIFARE_Key key;
extern WiFiManager wifiManager;
extern WiFiServer telnetServer;
extern WiFiClient telnetClient;

enum DeviceMode
{
    MODE_NONE,
    MODE_RECEPTION,
    MODE_GATE
};
extern DeviceMode currentMode;
extern String serverAddress;

enum ReceptionState { 
    RX_SHOW_MENU, 
    RX_WAIT_FOR_CHOICE, 
    RX_WAIT_FOR_POINTS, 
    RX_WAIT_FOR_STATE, 
    RX_WAIT_FOR_CARD 
};

extern ReceptionState currentStep;
