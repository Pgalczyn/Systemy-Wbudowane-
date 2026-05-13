#include "globals.h"
#include "cmd_prompt.h"
#include "commands.h"
#include "rfid_logic.h"
#include "wifi_logic.h"
#include <Preferences.h>
#include <WiFiManager.h>

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
WiFiManager wifiManager;
DeviceMode currentMode = MODE_NONE;


bool buttonPressed = false;
String selectedWorkMode = "GATE";
Preferences preferences;

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
  html += currentMode == "REGISTER"
              ? "<option value='REGISTER' selected>REGISTER</option>"
              : "<option value='REGISTER'>REGISTER</option>";
  html += "</select>";
  return html;
}

void setup()
{
  Serial.begin(9600);

  pinMode(BOOT_BTN, INPUT_PULLUP);

  preferences.begin("app", false);
  selectedWorkMode = preferences.getString("work_mode", "GATE");
  preferences.end();

  String customSelectHtml = buildWorkModeHtml(selectedWorkMode);
  WiFiManagerParameter custom_field(customSelectHtml.c_str());
  wifiManager.addParameter(&custom_field);
  wifiManager.setSaveParamsCallback(saveWorkMode);

  Serial.println("Lacze do zapamietanego WiFi...");
  if (!wifiManager.autoConnect("ESP32-Config"))
  {
    Serial.println("Error connecting to WiFi / portal...");
    delay(2000);
    ESP.restart();
  }

  Serial.println("Połączono z WiFi!");
  Serial.print("Adres IP: ");
  Serial.println(WiFi.localIP());

  Serial.print("Wybrany tryb pracy: ");
  Serial.println(selectedWorkMode);
  if (selectedWorkMode == "GATE")
  {
    currentMode = MODE_GATE;
  }
  else if (selectedWorkMode == "REGISTER")
  {
    currentMode = MODE_RECEPTION;
  }
  SPI.begin();
  mfrc522.PCD_Init();

  for (byte i = 0; i < 6; i++)
    key.keyByte[i] = 0xFF;

  Serial.println("System ready.");
}

void loop()
{
  if (currentMode != MODE_NONE && !isWifiConnected())
  {
    Serial.println("WiFi rozlaczone - restart systemu...");
    delay(1000);
    ESP.restart();
  }

  if (digitalRead(BOOT_BTN) == LOW && !buttonPressed)
  {
    Serial.println("BOOT button PRESSED (LOW detected)");
    buttonPressed = true;
    delay(100);
    
    if (digitalRead(BOOT_BTN) == LOW)
    {
      Serial.println("Resetting WiFi settings...");
      wifiManager.resetSettings();
      delay(500);
      ESP.restart();
    }
  }
  else if (digitalRead(BOOT_BTN) == HIGH && buttonPressed)
  {
    Serial.println("BOOT button RELEASED");
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