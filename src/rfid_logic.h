#pragma once
#include <Arduino.h>

constexpr byte BLOCK_PERSONAL = 60; // Sector 15
constexpr byte BLOCK_EMAIL    = 56; // Sector 14
constexpr byte BLOCK_SERVICE  = 52; // Sector 13

// Sector 15: Personal data (Sum: 16 + 12 + 16 = 48 bytes -> 3 blocks: 60, 61, 62)
struct __attribute__((packed)) PersonalData {
    uint8_t userId[16];   // 16 bytes for raw UUID (128-bit)
    char name[12];        // 12 bytes
    char surname[16];     // 16 bytes
    uint32_t crc;         // 4 bytes CRC32 for the entire structure (excluding this field)
};

// Sector 14: Contact data (Exactly 48 bytes = 3 blocks: 56, 57 and 58)
struct __attribute__((packed)) EmailData {
    char email[44];
    uint32_t crc;         // CRC32
};

// Sector 13: Service and contact data (Exactly 48 bytes = 3 blocks: 52, 53 and 54)
struct __attribute__((packed)) ServiceData {
    uint32_t validUntil;  // 4 bytes
    int32_t points;       // 4 bytes
    uint32_t crc;         // 4 bytes CRC32 for the entire structure (excluding this field)
};

// Result codes for RFID operations
enum RfidResult {
    RFID_OK = 0,
    RFID_AUTH_FAIL,
    RFID_READ_FAIL,
    RFID_WRITE_FAIL,
    RFID_CRC_INVALID,
    RFID_NOT_PRESENT,
    RFID_OTHER_ERROR
};

bool isCardPresent();
void stopComm();
String uidToHexString();

RfidResult writePersonalData(PersonalData &data);
RfidResult readPersonalData(PersonalData &data);

RfidResult writeServiceData(ServiceData &data);
RfidResult readServiceData(ServiceData &data);

RfidResult writeEmailData(EmailData &data);
RfidResult readEmailData(EmailData &data);