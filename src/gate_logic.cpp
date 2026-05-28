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
                gateState = GATE_FAILURE;
                break;
            }

            gateState = GATE_PROCESSING;
        }
        break;

    case GATE_PROCESSING:
    {
        printf("Info: Processing card for user: %s %s\n", currentPerson.name, currentPerson.surname);

        RfidResult resService = readServiceData(currentService);

        MemberDataResponse apiData;

        ApiResult apiRes = checkMemberData(currentPerson.userId, apiData, true);

        if (apiRes != ApiResult::API_OK)
        {
            if (resService != RFID_OK)
            {
                printf("Error: Data undefined or corrupted.\n");
                gateState = GATE_FAILURE;
            }
            else // FALLBACK
            {
                uint32_t currentTimestamp = time(nullptr);
                bool isExpired = (currentService.validUntil < currentTimestamp);
                bool isActive = (currentService.state == 1);

                if (isActive && !isExpired)
                {
                    printf("Info: Access granted.\n");
                    gateState = GATE_SUCCESS;
                }
                else
                {
                    printf("Info: Access denied. Status: %s\n", (!isActive) ? "Inactive" : (isExpired ? "Expired" : "Unknown"));
                    gateState = GATE_FAILURE;
                }
            }
        }
        else
        {
            ServiceData apiServiceData = {
                .validUntil = apiData.validUntil,
                .points = apiData.points,
                .state = (apiData.state == ACTIVE) ? (uint8_t)1 : (uint8_t)0
            };

            uint32_t expectedCrc = calculateCRC32((uint8_t *)&apiServiceData, sizeof(ServiceData) - 4);

            if (resService != RFID_OK || currentService.crc != expectedCrc)
            {
                printf("Warning: Card data CRC mismatch! Updating...\n");
                if (writeServiceData(apiServiceData) == RFID_OK)
                {
                    printf("Success: Card data updated successfully.\n");
                }
                else
                {
                    printf("Warning: Failed to update card data.\n");
                }
            }

            uint32_t currentTimestamp = time(nullptr);
            bool isExpired = (apiData.validUntil < currentTimestamp);
            bool isActive = (apiData.state == ACTIVE);

            if (isActive && !isExpired)
            {
                printf("Info: Access granted.\n");
                gateState = GATE_SUCCESS;
            }
            else
            {
                printf("Info: Access denied. Status: %s\n", (!isActive) ? "Inactive" : (isExpired ? "Expired" : "Unknown"));
                gateState = GATE_FAILURE;
            }
        }

        stopComm();
        break;
    }

    case GATE_FAILURE:
        gateStateChangeTime = millis();
        digitalWrite(LED_RED, LOW);
        gateState = GATE_FAILURE_FADE;
        break;

    case GATE_SUCCESS:
        gateStateChangeTime = millis();
        digitalWrite(LED_GREEN, LOW);
        gateState = GATE_SUCCESS_FADE;
        break;

    case GATE_SUCCESS_FADE:
        if (millis() - gateStateChangeTime > 2000)
        {
            digitalWrite(LED_GREEN, HIGH);
            gateState = GATE_WAITING_CARD;
        }
        break;

    case GATE_FAILURE_FADE:
        if (millis() - gateStateChangeTime > 2000)
        {
            digitalWrite(LED_RED, HIGH);
            gateState = GATE_WAITING_CARD;
        }
        break;
    }
}
