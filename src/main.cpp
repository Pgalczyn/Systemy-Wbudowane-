#include "globals.h"
#include "cmd_prompt.h"
#include "commands.h"
#include "rfid_logic.h"

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
DeviceMode currentMode = MODE_NONE;

void setup()
{
  Serial.begin(115200);

  SPI.begin();
  mfrc522.PCD_Init();

  shell.attach(Serial);

  for (byte i = 0; i < 6; i++)
    key.keyByte[i] = 0xFF;

  shell.addCommand(F("wifi"), cmdWifi);
  shell.addCommand(F("wifi-disconnect"), cmdWifiDisconnect);
  shell.addCommand(F("reception"), cmdReception);
  shell.addCommand(F("gate"), cmdGate);

  printf("\n--- ESP32 System CLI Ready ---\n");
  showPrompt();
}

void loop()
{
  shell.executeIfInput();

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