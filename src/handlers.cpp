#include <handlers.h>
#include "globals.h"
#include "rfid_logic.h"
#include "telnet_tools.h"
#include "app_service.h"

// =========================================================
// --- LOGIKA ZAPISU/ODCZYTU DANYCH BEZPOŚREDNIO NA KARTĘ ---
// =========================================================

// Sztywna mapa bloków pamięci (Sektory 1, 2 i 3)
const byte B_NAME         = 4;
const byte B_SURNAME      = 5;
const byte B_POINTS_STAT  = 6;
const byte B_EMAIL_1      = 8;
const byte B_EMAIL_2      = 9;
const byte B_EMAIL_3      = 10;
const byte B_DATE_START   = 12;
const byte B_DATE_END     = 13;

// Funkcja pomocnicza wykonująca autoryzację i fizyczny zapis 16 bajtów
bool writeDataToCardBlock(byte blockAddress, byte dataArray[16])
{
    MFRC522::MIFARE_Key key;
    for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

    MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddress, &key, &(mfrc522.uid));

    if (status != MFRC522::STATUS_OK) return false;

    status = mfrc522.MIFARE_Write(blockAddress, dataArray, 16);
    if (status != MFRC522::STATUS_OK) return false;

    return true;
}

bool readDataFromCardBlock(byte blockAddress, byte buffer[18])
{
    MFRC522::MIFARE_Key key;
    for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

    MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddress, &key, &(mfrc522.uid));

    if (status != MFRC522::STATUS_OK) return false;

    byte size = 18;
    status = mfrc522.MIFARE_Read(blockAddress, buffer, &size);
    if (status != MFRC522::STATUS_OK) return false;

    return true;
}

bool writeRegistrationToCard(String name, String surname, String email, String gymMembershipStarts, String gymMembershipEnds, int32_t points)
{
    byte dataName[16] = {0};
    byte dataSurname[16] = {0};
    byte dataPointsStatus[16] = {0};
    byte dataEmail[48] = {0};
    byte dataStart[16] = {0};
    byte dataEnd[16] = {0};

    name.getBytes(dataName, 16);
    surname.getBytes(dataSurname, 16);
    email.getBytes(dataEmail, 48);

    dataPointsStatus[0] = (points >> 24) & 0xFF;
    dataPointsStatus[1] = (points >> 16) & 0xFF;
    dataPointsStatus[2] = (points >> 8) & 0xFF;
    dataPointsStatus[3] = points & 0xFF;
    dataPointsStatus[4] = 1; // 1 = ACTIVE

    String shortStart = gymMembershipStarts.substring(0, 10);
    String shortEnd = gymMembershipEnds.substring(0, 10);
    shortStart.getBytes(dataStart, 16);
    shortEnd.getBytes(dataEnd, 16);

    bool success = true;
    success &= writeDataToCardBlock(B_NAME, dataName);
    success &= writeDataToCardBlock(B_SURNAME, dataSurname);
    success &= writeDataToCardBlock(B_POINTS_STAT, dataPointsStatus);
    success &= writeDataToCardBlock(B_EMAIL_1, dataEmail);
    success &= writeDataToCardBlock(B_EMAIL_2, dataEmail + 16);
    success &= writeDataToCardBlock(B_EMAIL_3, dataEmail + 32);
    success &= writeDataToCardBlock(B_DATE_START, dataStart);
    success &= writeDataToCardBlock(B_DATE_END, dataEnd);

    if (!success) telnetPrintFmt("Ostrzeżenie: Błąd zapisu na karcie RFID (Rejestracja).\n");
    else telnetPrintFmt("Sukces: Wszystkie dane usera bezpiecznie zdublowane na karcie RFID!\n");

    return success;
}

bool writeStateToCard(int32_t points, String gymMembershipStarts, String gymMembershipEnds, MembershipState newState)
{
    byte dataPointsStatus[16] = {0};
    byte dataStart[16] = {0};
    byte dataEnd[16] = {0};

    dataPointsStatus[0] = (points >> 24) & 0xFF;
    dataPointsStatus[1] = (points >> 16) & 0xFF;
    dataPointsStatus[2] = (points >> 8) & 0xFF;
    dataPointsStatus[3] = points & 0xFF;
    dataPointsStatus[4] = (newState == 0) ? 1 : 0;

    String shortStart = gymMembershipStarts.substring(0, 10);
    String shortEnd = gymMembershipEnds.substring(0, 10);
    shortStart.getBytes(dataStart, 16);
    shortEnd.getBytes(dataEnd, 16);

    bool success = true;
    success &= writeDataToCardBlock(B_POINTS_STAT, dataPointsStatus);
    success &= writeDataToCardBlock(B_DATE_START, dataStart);
    success &= writeDataToCardBlock(B_DATE_END, dataEnd);

    if (!success) telnetPrintFmt("Ostrzeżenie: Błąd zapisu nowej daty/stanu na karcie RFID.\n");
    else telnetPrintFmt("Sukces: Nowy status i zaktualizowane daty ważności zapisane na karcie RFID!\n");

    return success;
}

bool writePointsToCard(int32_t newPoints)
{
    byte blockBuffer[18] = {0};

    if (!readDataFromCardBlock(B_POINTS_STAT, blockBuffer)) {
        telnetPrintFmt("Błąd: Nie udało się odczytać Bloku 6. Przerywam zapis na karcie.\n");
        return false;
    }

    blockBuffer[0] = (newPoints >> 24) & 0xFF;
    blockBuffer[1] = (newPoints >> 16) & 0xFF;
    blockBuffer[2] = (newPoints >> 8) & 0xFF;
    blockBuffer[3] = newPoints & 0xFF;

    bool success = writeDataToCardBlock(B_POINTS_STAT, blockBuffer);

    if (!success) telnetPrintFmt("Ostrzeżenie: Wystąpił błąd zapisu punktów na karcie RFID.\n");
    else telnetPrintFmt("Sukces: Punkty na karcie zaktualizowane do stanu: %d\n", newPoints);

    return success;
}
// =========================================================

// GATE MODE VARIABLES
GateState gateState = GATE_WAITING_CARD;
unsigned long gateStateChangeTime = 0;
String gateCurrentUid = "";

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
        // Always waiting for a card
        if (authenticateCard())
        {
            gateCurrentUid = uidToHexString();
            printf("Card detected: %s\n", gateCurrentUid.c_str());

            gateState = GATE_PROCESSING;
            gateStateChangeTime = millis();
        }
        break;

    case GATE_PROCESSING:
        {
            // Send API request to check/update member
            bool success = logGymScan(gateCurrentUid);

            if (success)
            {
                // Access granted - turn on green LED
                printf("Access granted for: %s\n", gateCurrentUid.c_str());
                digitalWrite(LED_GREEN, LOW);   // Active Low - LOW = ON
                digitalWrite(LED_RED, HIGH);    // Active Low - HIGH = OFF
                gateState = GATE_SUCCESS;
            }
            else
            {
                // Access denied or error - turn on red LED
                printf("Access denied or error for: %s\n", gateCurrentUid.c_str());
                digitalWrite(LED_RED, LOW);     // Active Low - LOW = ON
                digitalWrite(LED_GREEN, HIGH);  // Active Low - HIGH = OFF
                gateState = GATE_FAILURE;
            }
            gateStateChangeTime = millis();
        }
        break;

    case GATE_SUCCESS:
    case GATE_FAILURE:
        // Keep LED on for 2 seconds
        if (millis() - gateStateChangeTime > 2000)
        {
            digitalWrite(LED_GREEN, HIGH);  // Active Low - HIGH = OFF
            digitalWrite(LED_RED, HIGH);    // Active Low - HIGH = OFF
            stopComm();
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

            if (input == "1") {
                lastChoice = input;
                currentStep = RX_WAITING_FOR_CARD_MSG;
            }
            else if (input == "2") {
                lastChoice = "2";
                telnetPrintFmt("Enter name: ");
                currentStep = RX_WAIT_FOR_NAME;
            }
            else if (input == "3") {
                lastChoice = "3";
                telnetPrintFmt("How many points?: ");
                currentStep = RX_WAIT_FOR_POINTS;
            }
            else if (input == "4") {
                lastChoice = "4";
                telnetPrintFmt("Status (1=ACT, 0=INA): ");
                currentStep = RX_WAIT_FOR_STATE;
            }
            else {
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
        else if (authenticateCard())
        {
            String uid = uidToHexString();
            telnetPrintFmt("Handling card: %s\n", uid.c_str());

            if (lastChoice == "1") {
                checkMemberData(uid);
            }
            else if (lastChoice == "2") {
                registerMember(uid, nameBuffer, surnameBuffer, emailBuffer);
                // Wywołanie zapisu na kartę z użyciem pobranych buforów
                // Z braku pobierania dat przez Telnet, podaję format "YYYY-MM-DD" jako placeholder
                writeRegistrationToCard(nameBuffer, surnameBuffer, emailBuffer, "2026-05-28", "2027-05-28", 0);
            }
            else if (lastChoice == "3") {
                modifyPoints(uid, pointsBuffer);
                // Wywołanie zapisu z bufora punktów
                writePointsToCard(pointsBuffer);
            }
            else if (lastChoice == "4") {
                changeMembershipState(uid, stateBuffer);
                // Ponownie, jako że maszyna stanów nie zbiera punktów/dat dla opcji nr 4, przekazuję zera/placeholdery
                writeStateToCard(0, "2026-05-28", "2027-05-28", stateBuffer);
            }

            stopComm();
            currentStep = RX_SHOW_MENU;
        }
        break;
    }
}