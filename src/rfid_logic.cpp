#include "rfid_logic.h"
#include "globals.h"
#include "app_service.h"
#include "cmd_prompt.h"
#include "wifi_logic.h"
#include <HTTPClient.h>

static String uidToHexString()
{
    String uidHex;
    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
        if (mfrc522.uid.uidByte[i] < 0x10)
            uidHex += "0";
        uidHex += String(mfrc522.uid.uidByte[i], HEX);
    }
    uidHex.toUpperCase();
    return uidHex;
}

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
    // after stopping comms, show shell prompt reflecting current mode
    showPrompt();
}

void handleGateLogic()
{
    if (!authenticateCard())
        return;

    printf("BRAMKA: Wykryto karte. Sprawdzam uprawnienia...\n");

    // Tu w przyszłości dodasz request HTTP do serwera
    // Jeśli serwer odpowie OK -> otwórz przekaźnik

    stopComm();
}

void handleReceptionLogic()
{
    if (!authenticateCard())
        return;

    if (!isWifiConnected())
    {
        stopComm();
        return;
    }

    String uidHex = uidToHexString();

    printf("\n--- RECEPCJA ---\n");
    printf("Karta UID: %s\n", uidHex.c_str());
    printf("Wybierz operacje:\n");
    printf("1. checkMember\n");
    printf("2. registerMember\n");
    printf("3. modifyPoints\n");
    printf("4. changeMembershipState\n");

    String op = readStringWithEcho("Twoj wybor (1-4): ");
    op.trim();

    if (op == "1")
    {
        bool exists = checkMemberData(uidHex);
        printf(exists ? "OK: Czlonek istnieje w systemie.\n" : "INFO: Brak czlonka w systemie.\n");
    }
    else if (op == "2")
    {
        bool ok = registerMember(uidHex);
        printf(ok ? "OK: Uzytkownik zarejestrowany.\n" : "BLAD: Rejestracja nieudana.\n");
    }
    else if (op == "3")
    {
        String amountInput = readStringWithEcho("Podaj zmiane punktow (np. 10 lub -5): ");
        amountInput.trim();
        if (amountInput.length() == 0)
        {
            printf("BLAD: Nie podano wartosci.\n");
        }
        else
        {
            int32_t amount = amountInput.toInt();
            bool ok = modifyPoints(uidHex, amount);
            printf(ok ? "OK: Punkty zaktualizowane.\n" : "BLAD: Nie udalo sie zmienic punktow.\n");
        }
    }
    else if (op == "4")
    {
        String stateInput = readStringWithEcho("Nowy status (1=ACTIVE, 0=INACTIVE): ");
        stateInput.trim();

        MembershipState state;
        if (stateInput == "1")
        {
            state = ACTIVE;
        }
        else if (stateInput == "0")
        {
            state = INACTIVE;
        }
        else
        {
            printf("BLAD: Nieprawidlowy status.\n");
            stopComm();
            return;
        }

        bool ok = changeMembershipState(uidHex, state);
        printf(ok ? "OK: Status czlonkostwa zaktualizowany.\n" : "BLAD: Nie udalo sie zmienic statusu.\n");
    }
    else
    {
        printf("BLAD: Nieprawidlowy wybor operacji.\n");
    }

    stopComm();
}