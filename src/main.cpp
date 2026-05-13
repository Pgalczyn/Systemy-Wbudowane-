#include "globals.h"
#include "wifi_logic.h"
#include "handlers.h"
#include <Preferences.h>
#include <cmd_tools.h>

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
WiFiManager wifiManager;
DeviceMode currentMode = MODE_NONE;
WiFiServer telnetServer(23);
WiFiClient telnetClient;

bool buttonPressed = false;
String selectedWorkMode = DEFAULT_WORK_MODE;
String serverAddress = DEFAULT_API_ADDR;
Preferences preferences;

void checkIfExternalReset();
void saveWorkMode();
void handleTelnet();

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

  // GET SAVED CONFIG
  preferences.begin("app", true);
  selectedWorkMode = preferences.getString(PARAM_WORK_MODE, DEFAULT_WORK_MODE);
  serverAddress = preferences.getString(PARAM_SERVER_ADDR, DEFAULT_API_ADDR);
  preferences.end();

  // ADD CUSTOM FIELDS TO WIFI MANAGER
  WiFiManagerParameter custom_server_addr(PARAM_SERVER_ADDR, "API server address", serverAddress.c_str(), 50);
  String customSelectHtml = buildWorkModeHtml(selectedWorkMode);
  WiFiManagerParameter custom_field(customSelectHtml.c_str());

  wifiManager.addParameter(&custom_server_addr);
  wifiManager.addParameter(&custom_field);

  wifiManager.setSaveParamsCallback(saveWorkMode);

  // TRY CONNECTING TO WIFI OR START CONFIG PORTAL
  if (!wifiManager.autoConnect("ESP32-Config"))
  {
    logMsg("Couldn't connect to WiFi. Restarting in 2 seconds...\n");
    delay(2000);
    ESP.restart();
  }

  logMsg("Wifi connected!\n");
  logMsg("Assigned IP address: %s\n", WiFi.localIP().toString().c_str());
  logMsg("Selected work mode: %s\n", selectedWorkMode.c_str());
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

  logMsg("System ready.\n");

  telnetServer.begin();
  telnetServer.setNoDelay(true);
}

void loop()
{
  handleTelnet();

  if (currentMode != MODE_NONE && !isWifiConnected())
  {
    logMsg("Wifi disconnected - restarting system...\n");
    delay(1000);
    ESP.restart();
  }

  checkIfExternalReset();

  if (currentMode != MODE_NONE)
  {
    if (currentMode == MODE_GATE)
    {
      handleGateLogic();
    }
    else if (currentMode == MODE_RECEPTION)
    {
      handleReceptionNonBlocking();
    }
  }

  yield();
}

void checkIfExternalReset()
{
  if (digitalRead(BOOT_BTN) == LOW && !buttonPressed)
  {
    logMsg("BOOT button PRESSED.\n");
    buttonPressed = true;
    delay(100);

    if (digitalRead(BOOT_BTN) == LOW)
    {
      logMsg("Resetting WiFi settings...\n");
      wifiManager.resetSettings();
      delay(500);
      ESP.restart();
    }
  }
  else if (digitalRead(BOOT_BTN) == HIGH && buttonPressed)
  {
    logMsg("BOOT button RELEASED.\n");
    buttonPressed = false;
  }
}

void saveWorkMode()
{
  if (wifiManager.server != nullptr)
  {
    preferences.begin("app", false);
    if (wifiManager.server->hasArg(PARAM_WORK_MODE))
    {
      selectedWorkMode = wifiManager.server->arg(PARAM_WORK_MODE);
      preferences.putString(PARAM_WORK_MODE, selectedWorkMode);
    }
    if (wifiManager.server->hasArg(PARAM_SERVER_ADDR))
    {
      serverAddress = wifiManager.server->arg(PARAM_SERVER_ADDR);
      preferences.putString(PARAM_SERVER_ADDR, serverAddress);
    }
    preferences.end();
  }
}



void handleTelnet()
{
  if (telnetServer.hasClient())
  {
    // Ktoś próbuje się połączyć!
    WiFiClient newClient = telnetServer.available();
    Serial.println("\n[DEBUG] Wykryto probe polaczenia Telnet!");

    if (!telnetClient || !telnetClient.connected())
    {
      telnetClient = newClient;
      telnetClient.println("\n=== ESP32 REMOTE TERMINAL ===");
      Serial.println("[DEBUG] Klient zaakceptowany i polaczony.");
    }
    else
    {
      Serial.println("[DEBUG] Odrzucono polaczenie - ktos inny juz jest polaczony.");
      newClient.stop();
    }
  }
}
