#include "app_service.h"
#include "api_client.h"
#include <ArduinoJson.h>

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
        MFRC522::PICC_CMD_MF_AUTH_KEY_A,
        blockAddress,
        &key,
        &(mfrc522.uid)
    );

    if (status != MFRC522::STATUS_OK) return false;

    status = mfrc522.MIFARE_Write(blockAddress, dataArray, 16);
    if (status != MFRC522::STATUS_OK) return false;

    return true;
}
bool readDataFromCardBlock(byte blockAddress, byte buffer[18])
{
    MFRC522::MIFARE_Key key;
    for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

    // 1. Autoryzacja sektora przed odczytem
    MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A,
        blockAddress,
        &key,
        &(mfrc522.uid)
    );

    if (status != MFRC522::STATUS_OK) return false;

    // 2. Odczyt danych z bloku
    // WAŻNE: Biblioteka MFRC522 wymaga bufora o rozmiarze minimum 18 bajtów,
    // ponieważ karta oprócz 16 bajtów danych przesyła też 2 bajty sumy kontrolnej CRC.
    byte size = 18;
    status = mfrc522.MIFARE_Read(blockAddress, buffer, &size);
    if (status != MFRC522::STATUS_OK) return false;

    return true;
}
bool registerMember(String cardUid, String name, String surname, String gymMembershipStarts, String gymMembershipEnds, String email, String coffeePoints)
{
    // --- 1. BUDOWANIE I WYSYŁANIE JSON DO API ---
    JsonDocument doc;

    doc["name"] = name;
    doc["surname"] = surname;
    doc["email"] = email;
    doc["card_UID"] = cardUid;
    // POPRAWKA: Przypisujemy zmienne przekazane do funkcji, a nie sztywne teksty
    doc["gymMembershipStarts"] = gymMembershipStarts;
    doc["gymMembershipEnds"] = gymMembershipEnds;
    doc["coffeePoints"] = coffeePoints.toInt();

    String payload;
    serializeJson(doc, payload);

    String resp = apiCall("POST", "/register", payload);

    // POPRAWKA: Zamiast "return" robimy warunek. Jeśli nie ma sukcesu, przerywamy.
    if (resp.indexOf("success") == -1) {
        return false;
    }

    // --- 2. PRZYGOTOWANIE BUFORÓW DANYCH DLA KARTY RFID ---
    byte dataName[16] = {0};
    byte dataSurname[16] = {0};
    byte dataPointsStatus[16] = {0};
    byte dataEmail[48] = {0};
    byte dataStart[16] = {0};
    byte dataEnd[16] = {0};

    // Kopiujemy teksty do bezpiecznych tablic bajtów
    name.getBytes(dataName, 16);
    surname.getBytes(dataSurname, 16);
    email.getBytes(dataEmail, 48);

    // Pakujemy punkty (4 bajty) oraz status (1 bajt) do jednego bloku
    int32_t points = coffeePoints.toInt();
    dataPointsStatus[0] = (points >> 24) & 0xFF;
    dataPointsStatus[1] = (points >> 16) & 0xFF;
    dataPointsStatus[2] = (points >> 8) & 0xFF;
    dataPointsStatus[3] = points & 0xFF;
    dataPointsStatus[4] = 1; // 1 = ACTIVE (zgodnie z naszym enumem)

    // Skracamy daty ISO do samych dni (YYYY-MM-DD, czyli pierwsze 10 znaków)
    String shortStart = gymMembershipStarts.substring(0, 10);
    String shortEnd = gymMembershipEnds.substring(0, 10);
    shortStart.getBytes(dataStart, 16);
    shortEnd.getBytes(dataEnd, 16);

    // --- 3. SEKWENCYJNY ZAPIS NA KARTĘ ---
    bool success = true;

    // Sektor 1
    success &= writeDataToCardBlock(B_NAME, dataName);
    success &= writeDataToCardBlock(B_SURNAME, dataSurname);
    success &= writeDataToCardBlock(B_POINTS_STAT, dataPointsStatus);

    // Sektor 2 (Email podzielony automatycznie na 3 bloki po 16 bajtów)
    success &= writeDataToCardBlock(B_EMAIL_1, dataEmail);
    success &= writeDataToCardBlock(B_EMAIL_2, dataEmail + 16);
    success &= writeDataToCardBlock(B_EMAIL_3, dataEmail + 32);

    // Sektor 3
    success &= writeDataToCardBlock(B_DATE_START, dataStart);
    success &= writeDataToCardBlock(B_DATE_END, dataEnd);

    if (!success) {
        printf("Ostrzeżenie: Dane zapisane w bazie MongoDB, ale nie udało się ich w pełni zdublować na karcie.\n");
    } else {
        printf("Sukces: Wszystkie dane usera bezpiecznie zdublowane na karcie RFID!\n");
    }

    return true;
}

bool getMemberData(String cardUid)
{
    String resp = apiCall("GET", "/getUserData/" + cardUid);

    if (resp.startsWith("ERROR"))
        return false;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, resp);
    if (error) {
        printf("JSON Deserialization failed!\n");
        return false;
    }

    const char* status = doc["status"];
    if (strcmp(status, "success") != 0) {
        printf("User not found or server error.\n");
        return false;
    }

    JsonObject user = doc["user"];
    const char* name = user["name"];
    const char* surname = user["surname"];
    int coffeePoints = user["coffeePoints"];
    const char* membershipEnds = user["gymMembershipEnds"];

    printf("\n=============================\n");
    printf("     MEMBER INFO CARD        \n");
    printf("=============================\n");
    printf("Name:    %s %s\n", name, surname);
    printf("Points:  %d\n", coffeePoints);
    printf("Expires: %s\n", membershipEnds);

    JsonArray sessions = doc["sessions"];
    printf("Total sessions registered: %d\n", sessions.size());

    if (sessions.size() > 0) {
        JsonObject latestSession = sessions[0]; // Indeks 0 to najnowsza sesja dzięki .sort() na serwerze
        const char* enterDate = latestSession["enterDate"];
        bool isAtTheGym = latestSession["isAtTheGym"];

        printf("-----------------------------\n");
        printf("Last activity:\n");
        printf(" - Date:   %s\n", enterDate);
        printf(" - Status: %s\n", isAtTheGym ? "Still inside the gym" : "Completed");
    }
    printf("=============================\n\n");

    return true;
}
bool changeMembershipState(String cardUid, MembershipState newState)
{
    String resp = apiCall("POST", "/change/membershipState/" + cardUid + "/" + String(newState));

    if (resp.startsWith("ERROR") || resp.indexOf("error") != -1) {
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, resp);
    if (error) {
        printf("Błąd: Serwer zmienił stan, ale nie udało się sparsować odpowiedzi JSON dla karty.\n");
        return true;
    }

    JsonObject user = doc["user"];
    int32_t points = user["coffeePoints"];
    String gymMembershipStarts = user["gymMembershipStarts"];
    String gymMembershipEnds = user["gymMembershipEnds"];

    // Przygotowujemy czyste bufory na bloki pamięci
    byte dataPointsStatus[16] = {0};
    byte dataStart[16] = {0};
    byte dataEnd[16] = {0};

    // Pakujemy aktualne punkty (biorąc je z serwera, żeby ich nie wyzerować!)
    dataPointsStatus[0] = (points >> 24) & 0xFF;
    dataPointsStatus[1] = (points >> 16) & 0xFF;
    dataPointsStatus[2] = (points >> 8) & 0xFF;
    dataPointsStatus[3] = points & 0xFF;

    // Zapisujemy nowy status: jeśli newState == 0 (ACTIVE), na karcie zapisujemy 1
    dataPointsStatus[4] = (newState == 0) ? 1 : 0;

    // Docinamy daty zwrócone przez serwer do formatu YYYY-MM-DD
    String shortStart = gymMembershipStarts.substring(0, 10);
    String shortEnd = gymMembershipEnds.substring(0, 10);
    shortStart.getBytes(dataStart, 16);
    shortEnd.getBytes(dataEnd, 16);

    // Zapisujemy zaktualizowane bloki na kartę przy użyciu funkcji pomocniczej
    bool success = true;
    success &= writeDataToCardBlock(B_POINTS_STAT, dataPointsStatus); // Blok 6
    success &= writeDataToCardBlock(B_DATE_START, dataStart);         // Blok 12
    success &= writeDataToCardBlock(B_DATE_END, dataEnd);           // Blok 13

    if (!success) {
        printf("Ostrzeżenie: Stan zmieniony w bazie, ale błąd zapisu nowej daty na karcie RFID.\n");
    } else {
        printf("Sukces: Nowy status i zaktualizowane daty ważności zapisane na karcie RFID!\n");
    }

    return true;
}

bool modifyPoints(String card_UID, int32_t amount)
{
    if (amount == 0) return true;

    String resp;

    if (amount > 0) {
        resp = apiCall("POST", "/add/coffee/points/" + card_UID + "/" + String(amount));
    }
    else {
        resp = apiCall("POST", "/subtract/coffee/points/" + card_UID + "/" + String(abs(amount)));
    }

    if (resp.startsWith("ERROR") || resp.indexOf("error") != -1) {
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, resp);
    if (error) {
        printf("Błąd: Serwer zmienił punkty, ale wystąpił błąd parsowania odpowiedzi JSON dla karty.\n");
        return true;
    }

    int32_t newPoints = doc["user"]["coffeePoints"];

    // 2. KROK "READ": Odczytujemy aktualny stan Bloku 6 z karty
    byte blockBuffer[18] = {0};
    if (!readDataFromCardBlock(B_POINTS_STAT, blockBuffer)) {
        printf("Błąd: Nie udało się odczytać Bloku 6. Przerywam zapis na karcie, aby nie uszkodzić danych.\n");
        return true;
    }

    // W tym momencie w blockBuffer[4] mamy bezpiecznie zapisaną flagę statusu (0 lub 1). Nie ruszamy jej!

    // 3. KROK "MODIFY": Wstrzykujemy nowe punkty w bajty 0-3 naszego bufora
    blockBuffer[0] = (newPoints >> 24) & 0xFF;
    blockBuffer[1] = (newPoints >> 16) & 0xFF;
    blockBuffer[2] = (newPoints >> 8) & 0xFF;
    blockBuffer[3] = newPoints & 0xFF;

    // 4. KROK "WRITE": Zapisujemy zmodyfikowany bufor z powrotem do Bloku 6
    bool success = writeDataToCardBlock(B_POINTS_STAT, blockBuffer);

    if (!success) {
        printf("Ostrzeżenie: Punkty zmienione w bazie, ale wystąpił błąd zapisu na karcie RFID.\n");
    } else {
        printf("Sukces: Punkty na karcie zaktualizowane do stanu: %d\n", newPoints);
    }

    return true;
}

bool logGymScan(String card_UID){

    String resp = apiCall("POST", "/enter/exit/gym/" + cardUid);

    if (resp.startsWith("ERROR") || resp.indexOf("error") != -1) {
        return false;
    }

    return true;
}