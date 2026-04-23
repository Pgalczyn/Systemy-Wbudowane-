#include <Arduino.h>                // Podstawowa biblioteka środowiska Arduino
#include <ESP32Console.h>           // Biblioteka do obsługi terminala CLI na ESP32
#include <SPI.h>                    // Biblioteka do komunikacji szeregowej SPI z czytnikiem
#include <MFRC522.h>                // Biblioteka do obsługi czytnika RFID RC522
#include <stdlib.h>                 // Standardowa biblioteka C (potrzebna do atoi)

using namespace ESP32Console;       // Używamy przestrzeni nazw konsoli, by nie pisać przedrostków

#define SS_PIN    21                 // Definicja pinu CS/SDA dla czytnika RFID
#define RST_PIN   22               // Definicja pinu Reset dla czytnika RFID
#define LED_PIN   4                 // Definicja pinu dla diody LED (np. sygnalizacja błędu)

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Tworzymy obiekt czytnika z przypisanymi pinami
MFRC522::MIFARE_Key key;            // Tworzymy zmienną przechowującą klucz dostępu do karty

const byte TARGET_BLOCK = 60;       // Sektor 15, blok 0 (pierwszy blok danych w ostatnim sektorze)

// --- Funkcja sprawdzająca obecność karty i uprawnienia ---
bool authenticateCard() {           
  if (!mfrc522.PICC_IsNewCardPresent()) return false;    // Jeśli nie ma nowej karty, przerwij funkcję
  if (!mfrc522.PICC_ReadCardSerial()) return false;      // Jeśli nie można odczytać numeru karty, przerwij
  
  // Próba autoryzacji do wybranego bloku za pomocą Klucza A
  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, TARGET_BLOCK, &key, &(mfrc522.uid));
  return (status == MFRC522::STATUS_OK);                 // Zwróć prawdę, jeśli autoryzacja się udała
}

// --- Funkcja wyciągająca punkty z pamięci karty ---
int32_t readPoints() {              
  byte buffer[18];                                       // Bufor na dane (16 bajtów danych + 2 bajty kontrolne)
  byte size = sizeof(buffer);                            // Określamy rozmiar bufora dla funkcji bibliotecznej
  
  // Fizyczny odczyt bloku z karty do bufora
  MFRC522::StatusCode status = mfrc522.MIFARE_Read(TARGET_BLOCK, buffer, &size);
  
  if (status != MFRC522::STATUS_OK) return -1;           // Jeśli błąd odczytu, zwróć -1
  
  int32_t points;                                        // Zmienna na wynik (4 bajty)
  memcpy(&points, buffer, sizeof(int32_t));              // Skopiuj pierwsze 4 bajty z bufora do zmiennej int
  return points;                                         // Zwróć odczytaną liczbę punktów
}

// --- Funkcja zapisująca nową liczbę punktów na kartę ---
bool writePoints(int32_t points) {  
  byte dataBlock[16] = {0};                              // Tworzymy pusty blok 16-bajtowy (wymóg karty)
  memcpy(dataBlock, &points, sizeof(int32_t));           // Wkładamy nasze punkty (4 bajty) na początek bloku
  
  // Fizyczny zapis całego bloku na kartę rfid
  MFRC522::StatusCode status = mfrc522.MIFARE_Write(TARGET_BLOCK, dataBlock, 16);
  return (status == MFRC522::STATUS_OK);                 // Zwróć prawdę, jeśli zapis się powiódł
}

// --- Funkcja czyszcząca połączenie po operacji ---
void stopComm() {                   
  mfrc522.PICC_HaltA();                                  // Zatrzymaj komunikację z obecną kartą
  mfrc522.PCD_StopCrypto1();                             // Wyłącz szyfrowanie, by zwolnić moduł
}

// --- KOMENDA: Sprawdzanie balansu ---
int cmdBalance(int argc, char **argv) {
  printf("Prosze przylozyc karte...\n");                 // Instrukcja dla użytkownika w terminalu
  while (!authenticateCard()) { delay(10); }             // Pętla blokująca: czekaj na fizyczny kontakt z kartą
  
  int32_t p = readPoints();                              // Wywołaj funkcję odczytu
  printf("Na karcie jest: %d punktow.\n", p);            // Wyświetl wynik w konsoli
  
  stopComm();                                            // Zakończ sesję z kartą
  return EXIT_SUCCESS;                                   // Poinformuj system, że komenda wykonana pomyślnie
}

// --- KOMENDA: Dodawanie punktów ---
int cmdDeposit(int argc, char **argv) {
  if (argc < 2) { printf("Blad! Podaj ilosc.\n"); return EXIT_FAILURE; } // Sprawdź czy wpisano liczbę
  int32_t toAdd = atoi(argv[1]);                         // Zamień tekst z terminala na liczbę całkowitą

  printf("Przyloz karte, aby dodac %d pkt...\n", toAdd); // Komunikat o oczekiwaniu na kartę
  while (!authenticateCard()) { delay(10); }             // Czekaj na kartę

  int32_t current = readPoints();                        // Pobierz obecny stan konta z karty
  if (writePoints(current + toAdd)) {                    // Dodaj punkty i spróbuj zapisać nową sumę
    printf("Sukces! Nowy stan: %d pkt.\n", current + toAdd); // Potwierdzenie zapisu
  }
  
  stopComm();                                            // Zamknij sesję
  return EXIT_SUCCESS;                                   // Sukces komendy
}

// --- KOMENDA: Odejmowanie punktów ---
int cmdWithdraw(int argc, char **argv) {
  if (argc < 2) { printf("Blad! Podaj ilosc.\n"); return EXIT_FAILURE; } // Walidacja argumentów
  int32_t toSub = atoi(argv[1]);                         // Konwersja tekstu na liczbę

  printf("Przyloz karte, aby odjac %d pkt...\n", toSub); // Komunikat
  while (!authenticateCard()) { delay(10); }             // Czekaj na kartę

  int32_t current = readPoints();                        // Sprawdź ile jest punktów
  if (current < toSub) {                                 // Sprawdź czy nie chcesz zabrać więcej niż jest
    printf("Blad: Masz tylko %d pkt!\n", current);       // Komunikat o braku środków
  } else if (writePoints(current - toSub)) {             // Zapisz mniejszą ilość punktów
    printf("Pobrano. Zostalo: %d pkt.\n", current - toSub); // Potwierdzenie transakcji
  }
  
  stopComm();                                            // Koniec sesji
  return EXIT_SUCCESS;                                   // Koniec funkcji
}

Console console;                                         // Tworzymy obiekt obsługujący terminal

void setup() {
  Serial.begin(115200);                                  // Start komunikacji USB z komputerem
  SPI.begin();                                           // Start magistrali SPI dla czytnika
  mfrc522.PCD_Init();                                    // Włącz fizyczny moduł czytnika RC522
  
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;    // Ustawienie domyślnego klucza fabrycznego karty

  console.begin(115200);                                 // Start konsoli na ESP32
  console.setPrompt("rfid_wallet> ");                    // Ustawienie napisu przed kursorem
  console.registerCommand("balance", cmdBalance, "Sprawdz punkty"); // Powiązanie słowa z funkcją
  console.registerCommand("deposit", cmdDeposit, "Dodaj punkty");   // Powiązanie wpłaty
  console.registerCommand("withdraw", cmdWithdraw, "Odejmij punkty"); // Powiązanie wypłaty

  printf("\n--- System gotowy. Sektor 15 (Blok 60) ---\n"); // Powitanie w terminalu
}

void loop() {
  delay(10);                                             // Pętla pusta, konsola działa w tle (FreeRTOS)
}