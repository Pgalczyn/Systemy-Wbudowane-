#include "gate_logic.h"
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
    {
        printf("Processing card for user: %s %s\n", currentPerson.name, currentPerson.surname);

        RfidResult resService = readServiceData(currentService);
        stopComm();

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
    }

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
