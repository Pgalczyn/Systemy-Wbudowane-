#include "rfid_logic.h"
#include "globals.h"
#include "hex_util.h"

static MFRC522::MIFARE_Key keyPersonal = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static MFRC522::MIFARE_Key keyService = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static MFRC522::MIFARE_Key keyEmail = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

uint32_t calculateCRC32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320;
            else crc >>= 1;
        }
    }
    return ~crc;
}

String getCardUid() {
    return bytesToHex(mfrc522.uid.uidByte, mfrc522.uid.size);
}

static bool authenticateBlock(byte blockAddr, MFRC522::MIFARE_Key &key)
{
    MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(mfrc522.uid));

    return (status == MFRC522::STATUS_OK);
}

bool isCardPresent()
{
    if (!mfrc522.PICC_IsNewCardPresent())
        return false;
    if (!mfrc522.PICC_ReadCardSerial())
        return false;
    return true;
}

void stopComm()
{
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
}

RfidResult writePersonalData(PersonalData &data) {
    if (!authenticateBlock(BLOCK_PERSONAL, keyPersonal)) return RFID_AUTH_FAIL;

    data.crc = calculateCRC32((uint8_t*)&data, sizeof(PersonalData) - 4);

    byte buffer[48] = {0};
    memcpy(buffer, &data, sizeof(PersonalData));

    for (byte i = 0; i < 3; i++) {
        if (mfrc522.MIFARE_Write(BLOCK_PERSONAL + i, &buffer[i * 16], 16) != MFRC522::STATUS_OK)
            return RFID_WRITE_FAIL;
    }
    return RFID_OK;
}

RfidResult readPersonalData(PersonalData &data) {
    if (!authenticateBlock(BLOCK_PERSONAL, keyPersonal)) return RFID_AUTH_FAIL;

    byte buffer[48] = {0};
    byte blockBuf[18];

    for (byte i = 0; i < 3; i++) {
        byte size = sizeof(blockBuf);
        if (mfrc522.MIFARE_Read(BLOCK_PERSONAL + i, blockBuf, &size) != MFRC522::STATUS_OK)
            return RFID_READ_FAIL;
        memcpy(&buffer[i * 16], blockBuf, 16);
    }

    PersonalData tempData;
    memcpy(&tempData, buffer, sizeof(PersonalData));

    uint32_t computedCrc = calculateCRC32((uint8_t*)&tempData, sizeof(PersonalData) - 4);
    if (computedCrc != tempData.crc) {
        return RFID_CRC_INVALID;
    }

    data = tempData;
    return RFID_OK;
}

RfidResult writeServiceData(ServiceData &data) {
    if (!authenticateBlock(BLOCK_SERVICE, keyService)) return RFID_AUTH_FAIL;

    data.crc = calculateCRC32((uint8_t*)&data, sizeof(ServiceData) - 4);

    byte buffer[16] = {0};
    memcpy(buffer, &data, sizeof(ServiceData));
    if (mfrc522.MIFARE_Write(BLOCK_SERVICE, buffer, 16) != MFRC522::STATUS_OK) return RFID_WRITE_FAIL;
    return RFID_OK;
}

RfidResult readServiceData(ServiceData &data) {
    if (!authenticateBlock(BLOCK_SERVICE, keyService)) return RFID_AUTH_FAIL;

    byte blockBuf[18];
    byte size = sizeof(blockBuf);
    if (mfrc522.MIFARE_Read(BLOCK_SERVICE, blockBuf, &size) != MFRC522::STATUS_OK) return RFID_READ_FAIL;

    ServiceData tempData;
    memcpy(&tempData, blockBuf, sizeof(ServiceData));

    uint32_t computedCrc = calculateCRC32((uint8_t*)&tempData, sizeof(ServiceData) - 4);
    if (computedCrc != tempData.crc) {
        return RFID_CRC_INVALID;
    }

    data = tempData;
    return RFID_OK;
}

RfidResult writeEmailData(EmailData &data) {
    if (!authenticateBlock(BLOCK_EMAIL, keyEmail)) return RFID_AUTH_FAIL;

    data.crc = calculateCRC32((uint8_t*)&data, sizeof(EmailData) - 4);

    byte buffer[48] = {0};
    memcpy(buffer, &data, sizeof(EmailData));

    for (byte i = 0; i < 3; i++) {
        if (mfrc522.MIFARE_Write(BLOCK_EMAIL + i, &buffer[i * 16], 16) != MFRC522::STATUS_OK)
            return RFID_WRITE_FAIL;
    }
    return RFID_OK;
}

RfidResult readEmailData(EmailData &data) {
    if (!authenticateBlock(BLOCK_EMAIL, keyEmail)) return RFID_AUTH_FAIL;

    byte buffer[48] = {0};
    byte blockBuf[18];

    for (byte i = 0; i < 3; i++) {
        byte size = sizeof(blockBuf);
        if (mfrc522.MIFARE_Read(BLOCK_EMAIL + i, blockBuf, &size) != MFRC522::STATUS_OK) 
            return RFID_READ_FAIL;
        memcpy(&buffer[i * 16], blockBuf, 16);
    }

    EmailData tempData;
    memcpy(&tempData, buffer, sizeof(EmailData));

    uint32_t computedCrc = calculateCRC32((uint8_t*)&tempData, sizeof(EmailData) - 4);
    if (computedCrc != tempData.crc) {
        return RFID_CRC_INVALID;
    }

    data = tempData;
    return RFID_OK;
}