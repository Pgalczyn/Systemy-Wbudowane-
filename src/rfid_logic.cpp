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
}

void handleGateLogic()
{
    if (!authenticateCard())
        return;

    consolePrintf("BRAMKA: Wykryto karte. Sprawdzam uprawnienia...\n");

    // Tu w przyszłości dodasz request HTTP do serwera
    // Jeśli serwer odpowie OK -> otwórz przekaźnik

    stopComm();
}

void handleReceptionLogic()
{
    if (!authenticateCard())
        return;

    String uidHex = uidToHexString();

    consolePrintf("\n--- RECEPCJA ---\n");
    consolePrintf("Karta UID: %s\n", uidHex.c_str());
    consolePrintf("Wybierz operacje:\n");
    consolePrintf("1. checkMember\n");
    consolePrintf("2. registerMember\n");
    consolePrintf("3. modifyPoints\n");
    consolePrintf("4. changeMembershipState\n");

    String op = readStringWithEcho("Twoj wybor (1-4): ");
    op.trim();

    if (op == "1")
    {
        bool exists = checkMemberData(uidHex);
        consolePrintf(exists ? "OK: Czlonek istnieje w systemie.\n" : "INFO: Brak czlonka w systemie.\n");
    }
    else if (op == "2")
    {
        bool ok = registerMember(uidHex);
        consolePrintf(ok ? "OK: Uzytkownik zarejestrowany.\n" : "BLAD: Rejestracja nieudana.\n");
    }
    else if (op == "3")
    {
        String amountInput = readStringWithEcho("Podaj zmiane punktow (np. 10 lub -5): ");
        amountInput.trim();
        if (amountInput.length() == 0)
        {
            consolePrintf("BLAD: Nie podano wartosci.\n");
        }
        else
        {
            int32_t amount = amountInput.toInt();
            bool ok = modifyPoints(uidHex, amount);
            consolePrintf(ok ? "OK: Punkty zaktualizowane.\n" : "BLAD: Nie udalo sie zmienic punktow.\n");
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
            consolePrintf("BLAD: Nieprawidlowy status.\n");
            stopComm();
            return;
        }

        bool ok = changeMembershipState(uidHex, state);
        consolePrintf(ok ? "OK: Status czlonkostwa zaktualizowany.\n" : "BLAD: Nie udalo sie zmienic statusu.\n");
    }
    else
    {
        consolePrintf("BLAD: Nieprawidlowy wybor operacji.\n");
    }

    stopComm();
}


//     // In RECEPTION mode we no longer wait for a card.
//     // Instead, show a simple menu and wait for user input via WebSerial.
//     while (true)
//     {
//         consolePrintf("\n--- RECEPCJA ---\n");
//         consolePrintf("Wybierz operacje:\n");
//         consolePrintf("1. checkMember\n");
//         consolePrintf("2. registerMember\n");
//         consolePrintf("3. modifyPoints\n");
//         consolePrintf("4. changeMembershipState\n");

//         String op = readStringWithEcho("Twoj wybor (1-4): ");
//         op.trim();

//         if (op.length() == 0)
//         {
//             // empty input (just Enter) — show prompt again
//             continue;
//         }

//         char c = op.charAt(0);
//         switch (c)
//         {
//         case '1':
//             consolePrintf("checkMember\n");
//             break;
//         case '2':
//             consolePrintf("registerMember\n");
//             break;
//         case '3':
//             consolePrintf("modifyPoints\n");
//             break;
//         case '4':
//             consolePrintf("changeMembershipState\n");
//             break;
//         default:
//             consolePrintf("BLAD: Nieprawidlowy wybor operacji.\n");
//             continue;
//         }

//         // handled one selection — return to main loop
//         return;
//     }
//         consolePrintf(ok ? "OK: Uzytkownik zarejestrowany.\n" : "BLAD: Rejestracja nieudana.\n");
//     }
//     else if (op == "3")
//     {
//         String amountInput = readStringWithEcho("Podaj zmiane punktow (np. 10 lub -5): ");
//         amountInput.trim();
//         if (amountInput.length() == 0)
//         {
//             consolePrintf("BLAD: Nie podano wartosci.\n");
//         }
//         else
//         {
//             int32_t amount = amountInput.toInt();
//             bool ok = modifyPoints(uidHex, amount);
//             consolePrintf(ok ? "OK: Punkty zaktualizowane.\n" : "BLAD: Nie udalo sie zmienic punktow.\n");
//         }
//     }
//     else if (op == "4")
//     {
//         String stateInput = readStringWithEcho("Nowy status (1=ACTIVE, 0=INACTIVE): ");
//         stateInput.trim();

//         MembershipState state;
//         if (stateInput == "1")
//         {
//             state = ACTIVE;
//         }
//         else if (stateInput == "0")
//         {
//             state = INACTIVE;
//         }
//         else
//         {
//             consolePrintf("BLAD: Nieprawidlowy status.\n");
//             stopComm();
//             return;
//         }

//         bool ok = changeMembershipState(uidHex, state);
//         consolePrintf(ok ? "OK: Status czlonkostwa zaktualizowany.\n" : "BLAD: Nie udalo sie zmienic statusu.\n");
//     }
//     else
//     {
//         consolePrintf("BLAD: Nieprawidlowy wybor operacji.\n");
//     }

//     stopComm();
// }