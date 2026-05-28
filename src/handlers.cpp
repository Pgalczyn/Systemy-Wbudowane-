#include <handlers.h>
#include "globals.h"
#include "rfid_logic.h"
#include "telnet_tools.h"
#include "app_service.h"

// GATE MODE VARIABLES
GateState gateState = GATE_WAITING_CARD;
unsigned long gateStateChangeTime = 0;
String gateCurrentUid = "";
static PersonalData currentPerson;
static EmailData currentEmail;
static ServiceData currentService;

// RECEPTION MODE VARIABLES
ReceptionState currentStep = RX_SHOW_MENU;
String lastChoice = "";
String nameBuffer = "";
String surnameBuffer = "";
String emailBuffer = "";
int32_t pointsBuffer = 0;
MembershipState stateBuffer = INACTIVE;

void handleGateLogic()
{
    switch (gateState)
    {
    case GATE_WAITING_CARD:
        if (isCardPresent())
        {
            gateCurrentUid = uidToHexString();
            printf("Card detected: %s\n", gateCurrentUid.c_str());

            RfidResult resPersonal = readPersonalData(currentPerson);
            RfidResult resEmail = readEmailData(currentEmail);

            if (resPersonal != RFID_OK || resEmail != RFID_OK)
            {
                printf("Error: Card data corrupted!\n");
                stopComm();
                gateState = GATE_PREPARE_FAILURE;
                break;
            }

            gateState = GATE_PROCESSING;
        }
        break;

    case GATE_PROCESSING:
        printf("Processing card for user: %s %s\n", currentPerson.name, currentPerson.surname);
        
        RfidResult resService = readServiceData(currentService);


        // ApiResult apiRes = checkMemberData(gateCurrentUid);
        
        // if (checkMemberData(gateCurrentUid))
        // {
        //     gateState = GATE_SUCCESS;
        // }
        // else
        // {
        //     gateState = GATE_FAILURE;
        // }
        // gateStateChangeTime = millis();

        // RfidResult resService = readServiceData(currentService);

        // // stopComm();

        // if (resService == RFID_CRC_INVALID)
        // {
        //     printf("Error: Service data corrupted (CRC mismatch)!\n");
        //     gateState = GATE_FAILURE;
        // }
        // else
        // {
        //     // RFID_OK lub błędy odczytu/autoryzacji (karta zabrana) - ignorujemy i idziemy do API
        //     gateState = GATE_PROCESSING;
        // }
        break;

    case GATE_PREPARE_FAILURE:
        gateStateChangeTime = millis();
        digitalWrite(LED_RED, LOW);
        gateState = GATE_FAILURE;
        break;

    case GATE_PREPARE_SUCCESS:
        gateStateChangeTime = millis();
        digitalWrite(LED_GREEN, LOW);
        gateState = GATE_SUCCESS;
        break;

    case GATE_SUCCESS:
        if (millis() - gateStateChangeTime > 2000)
        {
            digitalWrite(LED_GREEN, HIGH);
            gateState = GATE_WAITING_CARD;
        }
        break;

    case GATE_FAILURE:
        if (millis() - gateStateChangeTime > 2000)
        {
            digitalWrite(LED_RED, HIGH);
            gateState = GATE_WAITING_CARD;
        }
        break;
    }
}

void handleReceptionNonBlocking()
{
    if (!isTelnetConnected())
    {
        currentStep = RX_SHOW_MENU;
        return;
    }

    String input = "";

    switch (currentStep)
    {
    case RX_SHOW_MENU:
        telnetPrintFmt("--- RECEPTION (Choose 1-4) ---\n1. Check, 2. Register, 3. Points, 4. Status\n> ");
        currentStep = RX_WAIT_FOR_CHOICE;
        break;

    case RX_WAIT_FOR_CHOICE:
        if (getCommand(input))
        {
            input.trim();

            if (input == "1")
            {
                lastChoice = input;
                currentStep = RX_WAITING_FOR_CARD_MSG;
            }
            else if (input == "2")
            {
                lastChoice = "2";
                telnetPrintFmt("Enter name: ");
                currentStep = RX_WAIT_FOR_NAME;
            }
            else if (input == "3")
            {
                lastChoice = "3";
                telnetPrintFmt("How many points?: ");
                currentStep = RX_WAIT_FOR_POINTS;
            }
            else if (input == "4")
            {
                lastChoice = "4";
                telnetPrintFmt("Status (1=ACT, 0=INA): ");
                currentStep = RX_WAIT_FOR_STATE;
            }
            else
            {
                currentStep = RX_SHOW_MENU;
            }
        }
        break;

    case RX_WAIT_FOR_NAME:
        if (getCommand(input))
        {
            nameBuffer = input;
            telnetPrintFmt("Enter surname: ");
            currentStep = RX_WAIT_FOR_SURNAME;
        }
        break;

    case RX_WAIT_FOR_SURNAME:
        if (getCommand(input))
        {
            surnameBuffer = input;
            telnetPrintFmt("Enter email: ");
            currentStep = RX_WAIT_FOR_EMAIL;
        }
        break;

    case RX_WAIT_FOR_EMAIL:
        if (getCommand(input))
        {
            emailBuffer = input;
            currentStep = RX_WAITING_FOR_CARD_MSG;
        }
        break;

    case RX_WAIT_FOR_POINTS:
        if (getCommand(input))
        {
            pointsBuffer = input.toInt();
            currentStep = RX_WAITING_FOR_CARD_MSG;
        }
        break;

    case RX_WAIT_FOR_STATE:
        if (getCommand(input))
        {
            stateBuffer = (input == "1") ? ACTIVE : INACTIVE;
            currentStep = RX_WAITING_FOR_CARD_MSG;
        }
        break;

    case RX_WAITING_FOR_CARD_MSG:
        telnetPrintFmt("Hold the card... (press any key to cancel)\n");
        currentStep = RX_WAIT_FOR_CARD;
        break;

    case RX_WAIT_FOR_CARD:
        if (getCommand(input))
        {
            telnetPrintFmt("Operation cancelled.\n");
            stopComm();
            currentStep = RX_SHOW_MENU;
        }
        else if (isCardPresent())
        {
            String uid = uidToHexString();
            telnetPrintFmt("Handling card: %s\n", uid.c_str());

            if (lastChoice == "1")
                checkMemberData(uid);
            else if (lastChoice == "2")
                registerMember(uid, nameBuffer, surnameBuffer, emailBuffer);
            else if (lastChoice == "3")
                modifyPoints(uid, pointsBuffer);
            else if (lastChoice == "4")
                changeMembershipState(uid, stateBuffer);

            stopComm();
            currentStep = RX_SHOW_MENU;
        }
        break;
    }
}
