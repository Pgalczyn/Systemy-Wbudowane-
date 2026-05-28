#include "reception_logic.h"
#include "globals.h"
#include "rfid_logic.h"
#include "telnet_tools.h"
#include "app_service.h"

ReceptionState currentStep = RX_SHOW_MENU;
String lastChoice = "";
String nameBuffer = "";
String surnameBuffer = "";
String emailBuffer = "";
int32_t pointsBuffer = 0;
MembershipState stateBuffer = ACTIVE;

static PersonalData currentPerson;
static EmailData currentEmail;
static ServiceData currentService;

bool preAuthorize(const String &choice);
bool saveDataToCard(const MemberDataResponse &data);
void printFullCardData(const PersonalData &personal, const EmailData &email, const ServiceData &service);

void handleReceptionNonBlocking()
{
    if (!isTelnetConnected())
    {
        currentStep = RX_SHOW_MENU;
        return;
    }

    String input = "";
    String cardUid;

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
            cardUid = uidToHexString();
            telnetPrintFmt("Info: Handling card: %s\n", cardUid.c_str());

            if (preAuthorize(lastChoice))
            {
                RfidResult personalResult = readPersonalData(currentPerson);
                RfidResult emailResult = readEmailData(currentEmail);

                if (personalResult != RFID_OK || emailResult != RFID_OK)
                {
                    telnetPrintFmt("Error: Card data undefined or corrupted!\n");
                    stopComm();
                    currentStep = RX_SHOW_MENU;
                    return;
                }
            }

            currentStep = RX_EXECUTE_ACTION;
        }
        break;
    case RX_EXECUTE_ACTION:
    {
        if (lastChoice == "1")
        {
            MemberDataResponse outData;
            ApiResult res = checkMemberData(currentPerson.userId, outData);

            if (res != ApiResult::API_OK)
            {
                telnetPrintFmt("Error: API error. Fallback: Reading data directly from card...\n");

                ServiceData cardData;
                RfidResult serviceResult = readServiceData(cardData);

                if (serviceResult == RFID_OK)
                {
                    printFullCardData(currentPerson, currentEmail, cardData);
                }
                else
                {
                    telnetPrintFmt("Warning: Card data is corrupted (CRC mismatch)!\n");
                }
            }
            else
            {
                ServiceData apiServiceData;
                apiServiceData.points = outData.points;
                apiServiceData.validUntil = outData.validUntil;
                apiServiceData.state = (outData.state == ACTIVE) ? 1 : 0;

                uint32_t expectedCrc = calculateCRC32((uint8_t *)&apiServiceData, sizeof(ServiceData) - 4);

                ServiceData cardData;
                RfidResult serviceResult = readServiceData(cardData);

                if (serviceResult == RFID_OK && cardData.crc == expectedCrc)
                {
                    telnetPrintFmt("Success: Card data is consistent with API!\n");
                    printFullCardData(currentPerson, currentEmail, cardData);
                }
                else
                {
                    telnetPrintFmt("Warning: Data inconsistent! (CRC Mismatch)\n");

                    if (writeServiceData(apiServiceData) == RFID_OK)
                    {
                        telnetPrintFmt("Success: Card was successfully synchronized with API!\n");
                        printFullCardData(currentPerson, currentEmail, apiServiceData);
                    }
                    else
                    {
                        telnetPrintFmt("Error: Failed to update card service data.\n");
                    }
                }
            }

            stopComm();
            currentStep = RX_SHOW_MENU;
        }
        else if (lastChoice == "2")
        {
            RegisterRequest req = {cardUid, nameBuffer, surnameBuffer, emailBuffer};
            MemberDataResponse outData;
            ApiResult res = registerMember(req, outData);
            if (res != ApiResult::API_OK)
            {
                telnetPrintFmt("Error: Registration failed (API error)\n");
                stopComm();
                currentStep = RX_SHOW_MENU;
                return;
            }

            if (!saveDataToCard(outData))
            {
                telnetPrintFmt("Error: Failed to save data to card.\n");
                stopComm();
                currentStep = RX_SHOW_MENU;
                return;
            }
        }
        else if (lastChoice == "3")
        {
            int32_t newTotal = 0;

            ApiResult res = modifyPoints(currentPerson.userId, pointsBuffer, newTotal);

            if (res != ApiResult::API_OK)
            {
                telnetPrintFmt("Error: API error while modifying points.\n");
            }
            else
            {
                telnetPrintFmt("Info: Points updated. New total in API: %d\n", newTotal);

                ServiceData sData;
                if (readServiceData(sData) != RFID_OK)
                {
                    telnetPrintFmt("Error: Failed to read card service data.\n");
                }
                else
                {
                    sData.points = newTotal;
                    if (writeServiceData(sData) == RFID_OK)
                    {
                        telnetPrintFmt("Info: Card data updated successfully.\n");
                    }
                    else
                    {
                        telnetPrintFmt("Error: Failed to update card service data.\n");
                    }
                }
            }
        }
        else if (lastChoice == "4")
        {

            MembershipState actualState = INACTIVE;

            ApiResult res = changeMembershipState(currentPerson.userId, stateBuffer, actualState);

            if (res == ApiResult::API_OK)
            {
                telnetPrintFmt("Info: Successfully updated membership state: %s\n", (actualState == ACTIVE) ? "ACTIVE" : "INACTIVE");
            }
            else
            {
                telnetPrintFmt("Error: Błąd API podczas zmiany statusu (Kod: %d)\n", (int)res);
            }
        }
    }
    break;
    }
}

bool preAuthorize(const String &choice)
{
    return choice != "2";
}

bool saveDataToCard(const MemberDataResponse &data)
{
    PersonalData pData;
    memcpy(pData.userId, data.userId, 16);

    memset(pData.name, 0, sizeof(pData.name));
    strncpy(pData.name, nameBuffer.c_str(), sizeof(pData.name) - 1);

    memset(pData.surname, 0, sizeof(pData.surname));
    strncpy(pData.surname, surnameBuffer.c_str(), sizeof(pData.surname) - 1);

    if (writePersonalData(pData) != RFID_OK)
    {
        return false;
    }

    EmailData eData;
    memset(eData.email, 0, sizeof(eData.email));
    strncpy(eData.email, emailBuffer.c_str(), sizeof(eData.email) - 1);

    if (writeEmailData(eData) != RFID_OK)
    {
        return false;
    }

    ServiceData sData;
    sData.validUntil = data.validUntil;
    sData.points = data.points;
    sData.state = (data.state == ACTIVE) ? 1 : 0;

    if (writeServiceData(sData) != RFID_OK)
    {
        return false;
    }

    return true;
}

String uidToHexString(const uint8_t userId[16])
{
    String userIdHex = "";
    for (int i = 0; i < 16; i++)
    {
        if (userId[i] < 0x10)
            userIdHex += "0";
        userIdHex += String(userId[i], HEX);
    }
    userIdHex.toUpperCase();
    return userIdHex;
}

void printFullCardData(const PersonalData &person, const EmailData &email, const ServiceData &service)
{
    String userIdHex = uidToHexString(person.userId);

    telnetPrintFmt("\n========================================\n");
    telnetPrintFmt("          FULL CARD DATA              \n");
    telnetPrintFmt("========================================\n");
    telnetPrintFmt("User id : %s\n", userIdHex.c_str());
    telnetPrintFmt("Name           : %s\n", person.name);
    telnetPrintFmt("Surname        : %s\n", person.surname);
    telnetPrintFmt("Email          : %s\n", email.email);
    telnetPrintFmt("Points         : %d\n", service.points);
    telnetPrintFmt("Card validity  : %u (Unix Timestamp)\n", service.validUntil);
    telnetPrintFmt("========================================\n\n");
}