#include "rfid_logic.h"
#include "globals.h"

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
