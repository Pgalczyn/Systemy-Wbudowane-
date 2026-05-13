#include "rfid_logic.h"
#include "globals.h"
#include <HTTPClient.h>

bool authenticateCard()
{
    if (!mfrc522.PICC_IsNewCardPresent())
        return false;
    if (!mfrc522.PICC_ReadCardSerial())
        return false;

    // Próba autoryzacji do sektora 15 (blok 60)
    MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A, 60, &key, &(mfrc522.uid));
    return (status == MFRC522::STATUS_OK);
}

void stopComm()
{
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
}

void handleGateLogic()
{
    printf("BRAMKA: Wykryto karte. Sprawdzam uprawnienia...\n");

    // Tu w przyszłości dodasz request HTTP do serwera
    // Jeśli serwer odpowie OK -> otwórz przekaźnik
}

void handleReceptionLogic()
{
    printf("\n--- RECEPCJA ---\n");
    printf("Karta UID:");
    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
        printf(" %02X", mfrc522.uid.uidByte[i]);
    }
    printf("\nCo chcesz zrobic? (uzyj komend CLI: register, block itp.)\n");

    // W tym trybie loop() może tylko wyświetlać info,
    // a konkretne akcje (jak blokowanie) wywołujesz z palca w konsoli.
}