#include <Arduino.h>
#include <ESP32Console.h>
#include <MFRC522.h>
#include <SPI.h>
#include <stdlib.h>

using namespace ESP32Console;

float accountBalance = 100.0f;

constexpr uint8_t DIODE_RED = 4;
constexpr uint8_t RFID_SS_PIN = 21;
constexpr uint8_t RFID_RST_PIN = 22;

MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);

void printUid(const MFRC522::Uid &uid)
{
  printf("Card UID:");
  for (byte i = 0; i < uid.size; ++i)
  {
    printf(" %02X", uid.uidByte[i]);
  }
  printf("\n");
}

inline void ledOn()
{
  digitalWrite(DIODE_RED, LOW);
}

inline void ledOff()
{
  digitalWrite(DIODE_RED, HIGH);
}

// --- Command Handlers ---

// Callback for "balance" command
int cmdBalance(int argc, char **argv)
{
  (void)argc;
  (void)argv;
  printf("Current balance: %.2f USD\n", accountBalance);
  return EXIT_SUCCESS;
}

// Callback for "deposit" command
int cmdDeposit(int argc, char **argv)
{
  if (argc < 2)
  {
    printf("Usage: deposit <amount>\n");
    return EXIT_FAILURE;
  }
  float amount = atof(argv[1]);
  accountBalance += amount;
  printf("Deposited: %.2f USD. New balance: %.2f USD\n", amount, accountBalance);
  return EXIT_SUCCESS;
}

// Callback for "withdraw" command
int cmdWithdraw(int argc, char **argv)
{
  if (argc < 2)
  {
    printf("Usage: withdraw <amount>\n");
    return EXIT_FAILURE;
  }
  float amount = atof(argv[1]);
  if (amount > accountBalance)
  {
    printf("Error: Insufficient funds!\n");
    return EXIT_FAILURE;
  }

  accountBalance -= amount;
  printf("Withdrawn: %.2f USD. Remaining balance: %.2f USD\n", amount, accountBalance);
  return EXIT_SUCCESS;
}

// Callback for "blink" command
int cmdBlink(int argc, char **argv)
{
  (void)argv;
  if (argc != 1)
  {
    printf("Usage: blink\n");
    return EXIT_FAILURE;
  }

  ledOn();
  delay(500);
  ledOff();

  printf("Blink done: LED on for 0.5 s\n");
  return EXIT_SUCCESS;
}

// Callback for "add-card" command
int cmdAddCard(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  printf("Scanning for card for 5 seconds...\n");

  unsigned long start = millis();
  while (millis() - start < 5000UL)
  {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
    {
      printUid(mfrc522.uid);
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      return EXIT_SUCCESS;
    }

    delay(50);
  }

  printf("No card detected.\n");
  return EXIT_FAILURE;
}

// --- Console Setup ---

Console console;

void setup()
{
  pinMode(DIODE_RED, OUTPUT);

  ledOff();

  SPI.begin();
  mfrc522.PCD_Init();

  // Initialize console UART
  console.begin(115200);
  console.setPrompt("wallet> ");

  // Register commands
  console.registerCommand("balance", cmdBalance, "Shows current account balance");
  console.registerCommand("deposit", cmdDeposit, "Add funds (e.g. deposit 50)");
  console.registerCommand("withdraw", cmdWithdraw, "Subtract funds (e.g. withdraw 20)");
  console.registerCommand("blink", cmdBlink, "Turn LED on for 0.5 s");
  console.registerCommand("add-card", cmdAddCard, "Wait 5 seconds for a card and print its UID");

  printf("\n--- ESP32 Wallet CLI Ready ---\n");
  printf("Type 'help' to see available commands.\n");
}

void loop()
{

}