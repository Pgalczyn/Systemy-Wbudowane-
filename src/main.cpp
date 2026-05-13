#include "globals.h"
#include "cmd_prompt.h"
#include "rfid_logic.h"
#include "wifi_logic.h"
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiManager.h>

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
WiFiManager wifiManager;
DeviceMode currentMode = MODE_NONE;

bool buttonPressed = false;
String selectedWorkMode = "GATE";
Preferences preferences;
bool useRFID = false; // set to true to enable RFID-driven flows

void saveWorkMode()
{
  if (wifiManager.server != nullptr && wifiManager.server->hasArg("work_mode"))
  {
    selectedWorkMode = wifiManager.server->arg("work_mode");
    preferences.begin("app", false);
    preferences.putString("work_mode", selectedWorkMode);
    preferences.end();
  }
}

String buildWorkModeHtml(const String &currentMode)
{
  String html = "<label for='work_mode'><b>Tryb pracy</b></label><br>";
  html += "<select name='work_mode' id='work_mode'>";
  html += currentMode == "GATE"
              ? "<option value='GATE' selected>GATE</option>"
              : "<option value='GATE'>GATE</option>";
  html += currentMode == "RECEPTION"
              ? "<option value='RECEPTION' selected>RECEPTION</option>"
              : "<option value='RECEPTION'>RECEPTION</option>";
  html += "</select>";
  return html;
}

void setup()
{
  Serial.begin(115200);
  pinMode(BOOT_BTN, INPUT_PULLUP);

  preferences.begin("app", false);
  selectedWorkMode = preferences.getString("work_mode", "GATE");
  preferences.end();

  String customSelectHtml = buildWorkModeHtml(selectedWorkMode);
  WiFiManagerParameter custom_field(customSelectHtml.c_str());
  wifiManager.addParameter(&custom_field);
  wifiManager.setSaveParamsCallback(saveWorkMode);

  if (!wifiManager.autoConnect("ESP32-Config"))
  {
    delay(2000);
    ESP.restart();
  }

  setupWebConsole();

  consolePrintf("Połączono z WiFi!\n");
  consolePrintf("Adres IP: %s\n", WiFi.localIP().toString().c_str());
  consolePrintf("Wybrany tryb pracy: %s\n", selectedWorkMode.c_str());
  if (selectedWorkMode == "GATE")
  {
    currentMode = MODE_GATE;
  }
  else if (selectedWorkMode == "RECEPTION")
  {
    currentMode = MODE_RECEPTION;
  }
  SPI.begin();
  mfrc522.PCD_Init();

  for (byte i = 0; i < 6; i++)
    key.keyByte[i] = 0xFF;

  consolePrintf("System ready.\n");
}

void loop()
{
  if (currentMode != MODE_NONE && !isWifiConnected())
  {
    consolePrintf("WiFi rozlaczone - restart systemu...\n");
    delay(1000);
    ESP.restart();
  }

  if (digitalRead(BOOT_BTN) == LOW && !buttonPressed)
  {
    consolePrintf("BOOT button PRESSED (LOW detected)\n");
    buttonPressed = true;
    delay(100);
    
    if (digitalRead(BOOT_BTN) == LOW)
    {
      consolePrintf("Resetting WiFi settings...\n");
      wifiManager.resetSettings();
      delay(500);
      ESP.restart();
    }
  }
  else if (digitalRead(BOOT_BTN) == HIGH && buttonPressed)
  {
    consolePrintf("BOOT button RELEASED\n");
    buttonPressed = false;
  }

  if (currentMode != MODE_NONE)
  {
    if (currentMode == MODE_GATE)
    {
      handleGateLogic();
    }
    else if (currentMode == MODE_RECEPTION)
    {
      handleReceptionLogic();
    }
  }

  yield();
}