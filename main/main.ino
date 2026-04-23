#include <SPI.h>
#include <MFRC522.h>
#include <vector>

#define LED_PIN 4
#define BOOT_BTN 0

#define SS_PIN 21
#define RST_PIN 22
#define LED_BUILTIN 2

#define LED_PIN1 26
#define LED_PIN2 25

MFRC522 rfid(SS_PIN, RST_PIN);

// byte adminUID[] = { 0xF0, 0xD0, 0x49, 0x5F };
// std::vector<std::vector<byte>> authorizedCards;

int mode = 0;
int TOTAL_MODES = 3;
bool buttonPrev = HIGH;

void setup() {
  // Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  
  pinMode(LED_PIN, OUTPUT);
  
  pinMode(BOOT_BTN, INPUT_PULLUP);

  pinMode(LED_PIN1, OUTPUT);
  pinMode(LED_PIN2, OUTPUT);

  digitalWrite(LED_PIN, HIGH);
  digitalWrite(LED_PIN1, HIGH);
  digitalWrite(LED_PIN2, HIGH);

  // SPI.begin();
  // rfid.PCD_Init();
}

void loop() {
  bool buttonActual = digitalRead(BOOT_BTN);

  if (buttonPrev == HIGH && buttonActual == LOW) {
    delay(50);

    mode = (mode + 1) % TOTAL_MODES;

    displayMode();

    // Czekaj aż puścisz przycisk, żeby nie przełączać w pętli
    while (digitalRead(BOOT_BTN) == LOW)
      ;
  }

  buttonPrev = buttonActual;

  // if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  // Serial.print("UID karty:");
  // for (byte i = 0; i < rfid.uid.size; i++) {
  //   Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
  //   Serial.print(rfid.uid.uidByte[i], HEX);
  // }
  // Serial.println();

  // digitalWrite(LED_PIN, LOW);  // Włącz diodę
  // delay(500);                  // Świeć przez pół sekundy
  // digitalWrite(LED_PIN, HIGH); // Wyłącz diodę

  // rfid.PICC_HaltA();
}

void displayMode() {
  if (mode == 0) {
    digitalWrite(LED_PIN1, HIGH);
    digitalWrite(LED_PIN2, HIGH);
  } else if (mode == 1) {
    digitalWrite(LED_PIN1, LOW);
    digitalWrite(LED_PIN2, HIGH);
  } else if (mode == 2) {
    digitalWrite(LED_PIN1, HIGH);
    digitalWrite(LED_PIN2, LOW);
  }
}