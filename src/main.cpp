#include <Arduino.h>
#include <ESP32Console.h>
#include <stdlib.h>

using namespace ESP32Console;

float accountBalance = 100.0f;
constexpr uint8_t LED_PIN = 4; // D4 on many ESP32 DevKit boards
constexpr bool LED_ACTIVE_LOW = true;

inline void ledOn()
{
  digitalWrite(LED_PIN, LED_ACTIVE_LOW ? LOW : HIGH);
}

inline void ledOff()
{
  digitalWrite(LED_PIN, LED_ACTIVE_LOW ? HIGH : LOW);
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

// --- Console Setup ---

Console console;

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  ledOff();

  // Initialize console UART
  console.begin(115200);
  console.setPrompt("wallet> ");

  // Register commands
  console.registerCommand("balance", cmdBalance, "Shows current account balance");
  console.registerCommand("deposit", cmdDeposit, "Add funds (e.g. deposit 50)");
  console.registerCommand("withdraw", cmdWithdraw, "Subtract funds (e.g. withdraw 20)");
  console.registerCommand("blink", cmdBlink, "Turn LED on for 0.5 s");

  printf("\n--- ESP32 Professional CLI Ready ---\n");
  printf("Type 'help' to see available commands.\n");
}

void loop()
{
  // Console runs in its own task in this library version.
  delay(10);
}