#include "rfid_logic.h"
#include "globals.h"
#include "telnet_tools.h"
#include "app_service.h"
String uidToHexString()
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