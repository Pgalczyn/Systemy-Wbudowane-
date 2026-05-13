#include <handlers.h>
#include "globals.h"
#include "rfid_logic.h"
#include "cmd_prompt.h"
#include "app_service.h"

void handleGateLogic()
{
    // todo:
    // zaczytac detailsy zblizonej karty (odnotowac wejscie w systemie)
    // jesli blad api -> sprawdzic stan zapisany w karcie

    // jesli api zwraca OK -> zapisac nową datę na karcie
    // zaswiecic zieloną diodę

    // jesli api zwraca ERROR -> zapisac nową datę na karcie
    // zaswiecic czerwoną diodę

    // NAJLEPIEJ JAKBY KARTA ZOSTALA ZBLIZONA TYLKO RAZ
    // ALE REQUEST DO API TRWA TROCHE CZASU
    // JEZELI UZYTKOWNIK ZABIERZE KARTE ZBYT SZYBKO
    // CZERWONA DIODA POWINNA SIE ZASWIECIC
    // MOZNA DODAC JAKIES MIGANIE ŻÓŁTĄ DIODĄ, ŻE PROCES TRWA
    return;
}

void handleReceptionLogic()
{
    printf("\n=== RECEPTION PANEL ===\n");
    printf("1. checkMembershipDetails\n");
    printf("2. registerMembership\n");
    printf("3. modifyPoints\n");
    printf("4. changeMembershipState\n");

    String op = readStringWithEcho("Select operation (1-4): ");
    op.trim();

    // Sprawdzenie czy wybór jest prawidłowy zanim przejdziemy dalej
    if (op != "1" && op != "2" && op != "3" && op != "4") {
        printf("ERROR: Invalid selection.\n");
        return;
    }

    // Zmienne pomocnicze na dodatkowe dane
    int32_t pointsToAdd = 0;
    MembershipState newState = INACTIVE;

    // Pobieramy dodatkowe dane przed zbliżeniem karty (żeby nie pisać na klawiaturze trzymając kartę)
    if (op == "3") {
        String input = readStringWithEcho("Enter number of points to add/subtract: ");
        pointsToAdd = input.toInt();
    } 
    else if (op == "4") {
        String input = readStringWithEcho("New status (1=ACTIVE, 0=INACTIVE): ");
        newState = (input == "1") ? ACTIVE : INACTIVE;
    }

    // --- MOMENT ZCZYTANIA KARTY ---
    printf("\nPlease bring the card close to the reader...\n");
    
    // Pętla blokująca - czekamy aż karta się pojawi
    while (!authenticateCard()) {
        yield(); // Zapobiega restartowi Watchdoga ESP32
        delay(100); 
    }

    String uidHex = uidToHexString();
    printf("Card detected (UID: %s). Processing...\n", uidHex.c_str());

    // --- WYKONANIE OPERACJI ---
    if (op == "1") {
        bool exists = checkMemberData(uidHex); // Używam nazwy z Twojego app_service
        printf(exists ? "RESULT: Member exists.\n" : "RESULT: Member not found in database.\n");
    }
    else if (op == "2") {
        bool ok = registerMember(uidHex);
        printf(ok ? "RESULT: Registration successful.\n" : "RESULT: Registration failed.\n");
    }
    else if (op == "3") {
        bool ok = modifyPoints(uidHex, pointsToAdd);
        printf(ok ? "RESULT: Points updated.\n" : "RESULT: Failed to update points.\n");
    }
    else if (op == "4") {
        bool ok = changeMembershipState(uidHex, newState);
        printf(ok ? "RESULT: Status changed.\n" : "RESULT: Error changing status.\n");
    }

    // Sprzątanie po RFID
    stopComm();
    printf("=== OPERATION COMPLETED ===\n\n");
}