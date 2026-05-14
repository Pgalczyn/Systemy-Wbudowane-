#include <handlers.h>
#include "globals.h"
#include "rfid_logic.h"
#include "cmd_tools.h"
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

ReceptionState currentStep = RX_SHOW_MENU;
String lastChoice = "";
int32_t pointsBuffer = 0;
MembershipState stateBuffer = INACTIVE;

void handleReceptionNonBlocking()
{
    if (!telnetClient || !telnetClient.connected())
    {
        currentStep = RX_SHOW_MENU;
        return;
    }

    String input = "";

    switch (currentStep)
    {
    case RX_SHOW_MENU:
        logMsg("\n--- RECEPTION (Choose 1-4) ---\n1. Check, 2. Register, 3. Points, 4. Status\n> ");
        currentStep = RX_WAIT_FOR_CHOICE;
        break;

    case RX_WAIT_FOR_CHOICE:
        if (getCommand(input))
        {
            input.trim();

            if (input == "1" || input == "2") {
                lastChoice = input;
                logMsg("\nHold the card...\n");
                currentStep = RX_WAIT_FOR_CARD;
            }
            else if (input == "3") {
                lastChoice = "3";
                logMsg("\nHow many points?: ");
                currentStep = RX_WAIT_FOR_POINTS;
            }
            else if (input == "4") {
                lastChoice = "4";
                logMsg("\nStatus (1=ACT, 0=INA): ");
                currentStep = RX_WAIT_FOR_STATE;
            }
            else {
                currentStep = RX_SHOW_MENU;
            }
        }
        break;

    case RX_WAIT_FOR_POINTS:
        if (getCommand(input))
        {
            pointsBuffer = input.toInt();
            logMsg("\nHold the card...\n");
            currentStep = RX_WAIT_FOR_CARD;
        }
        break;

    case RX_WAIT_FOR_STATE:
        if (getCommand(input))
        {
            stateBuffer = (input == "1") ? ACTIVE : INACTIVE;
            logMsg("\nHold the card...\n");
            currentStep = RX_WAIT_FOR_CARD;
        }
        break;

    case RX_WAIT_FOR_CARD:
        if (authenticateCard())
        { // Sprawdza czy karta jest, jak nie - od razu wychodzi
            String uid = uidToHexString();
            logMsg("Handling card: %s\n", uid.c_str());

            if (lastChoice == "1")
                checkMemberData(uid);
            else if (lastChoice == "2")
                registerMember(uid);
            else if (lastChoice == "3")
                modifyPoints(uid, pointsBuffer);
            else if (lastChoice == "4")
                changeMembershipState(uid, stateBuffer);

            stopComm();
            currentStep = RX_SHOW_MENU; // Powrót do początku
        }
        break;
    }
}
