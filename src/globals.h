#pragma once
#include <Arduino.h>
#include <MFRC522.h>
#include <WiFiManager.h>

#define SS_PIN 21
#define RST_PIN 22
#define BOOT_BTN 0
#define LED_RED 4 // D4 - Active Low
#define LED_GREEN 5   // D5 - Active Low

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

enum GateState
{
    GATE_WAITING_CARD,
    GATE_PROCESSING,
    GATE_SUCCESS,
    GATE_FAILURE
};

enum ReceptionState
{
    RX_SHOW_MENU,
    RX_WAIT_FOR_CHOICE,
    RX_WAIT_FOR_NAME,
    RX_WAIT_FOR_SURNAME,
    RX_WAIT_FOR_EMAIL,
    RX_WAIT_FOR_POINTS,
    RX_WAIT_FOR_STATE,
    RX_WAITING_FOR_CARD_MSG, // Show message once
    RX_WAIT_FOR_CARD,         // Wait for card or cancel
    RX_WAIT_FOR_MONTHS
};

extern ReceptionState currentStep;
