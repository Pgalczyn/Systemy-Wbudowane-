#include "app_service.h"
#include "api_client.h"
#include <ArduinoJson.h>

// Wewnętrzna funkcja pomocnicza: Konwersja tablicy bajtów na String HEX
static String userIdToHex(const uint8_t* userId) {
    String hex = "";
    for (int i = 0; i < 16; i++) {
        if (userId[i] < 0x10) hex += "0";
        hex += String(userId[i], HEX);
    }
    hex.toUpperCase();
    return hex;
}

// Wewnętrzna funkcja pomocnicza: Konwersja String HEX na tablicę bajtów
static void hexToUserId(const String& hexStr, uint8_t* outUserId) {
    for (size_t i = 0; i < 16; i++) {
        if (i * 2 + 2 <= hexStr.length()) {
            String byteStr = hexStr.substring(i * 2, i * 2 + 2);
            outUserId[i] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
        } else {
            outUserId[i] = 0;
        }
    }
}

ApiResult registerMember(const RegisterRequest &req, MemberDataResponse &outData)
{
    JsonDocument doc;
    doc["cardUid"] = req.userId;
    doc["name"] = req.name;
    doc["surname"] = req.surname;
    doc["email"] = req.email;

    String payload;
    serializeJson(doc, payload);

    String resp = apiCall("POST", "/register", payload);

    if (resp.startsWith("ERROR") || resp.length() == 0) {
        return ApiResult::API_ERROR;
    }

    if (deserializeJson(doc, resp)) {
        return ApiResult::API_ERROR;
    }

    if (doc["success"] == true) {
        outData.validUntil = doc["validUntil"] | 0;
        outData.points = doc["points"] | 0;

        // Odbieramy userId jako String HEX i przepisujemy do tablicy bajtów
        String apiUuid = doc["userId"] | "";
        hexToUserId(apiUuid, outData.userId);

        return ApiResult::API_OK;
    }

    return ApiResult::API_ERROR;
}

ApiResult checkMemberData(const uint8_t* userId, MemberDataResponse &outData)
{
    // Konwersja surowego ID na HEX przed wysyłką do API
    String userIdStr = userIdToHex(userId);
    String resp = apiCall("GET", "/member/" + userIdStr);

    if (resp.startsWith("ERROR") || resp.length() == 0) {
        return ApiResult::API_ERROR;
    }

    JsonDocument doc;
    if (deserializeJson(doc, resp)) {
        return ApiResult::API_ERROR;
    }

    if (doc["allowed"] == true || doc["success"] == true) {
        outData.validUntil = doc["validUntil"] | 0;
        outData.points = doc["points"] | 0;
        String stateStr = doc["status"] | "INACTIVE";
        outData.state = (stateStr == "ACTIVE") ? ACTIVE : INACTIVE;
        
        String apiUuid = doc["userId"] | userIdStr;
        hexToUserId(apiUuid, outData.userId);

        return ApiResult::API_OK;
    }

    return ApiResult::API_ERROR;
}

ApiResult modifyPoints(const uint8_t* userId, int32_t amount, int32_t &outNewTotal)
{
    String userIdStr = userIdToHex(userId);

    JsonDocument doc;
    doc["uid"] = userIdStr;
    doc["amount"] = amount;

    String payload;
    serializeJson(doc, payload);

    String resp = apiCall("POST", "/points", payload);

    if (resp.startsWith("ERROR") || resp.length() == 0) {
        return ApiResult::API_ERROR;
    }

    if (deserializeJson(doc, resp)) {
        return ApiResult::API_ERROR;
    }

    if (doc["success"] == true || resp.indexOf("ERROR") == -1) {
        outNewTotal = doc["new_total"] | 0;
        printf("Points updated via API. Current total: %d\n", outNewTotal);
        return ApiResult::API_OK;
    }

    return ApiResult::API_ERROR;
}

ApiResult changeMembershipState(const uint8_t* userId, MembershipState newState, MembershipState &outActualState)
{
    String userIdStr = userIdToHex(userId);

    JsonDocument doc;
    doc["uid"] = userIdStr;
    doc["status"] = (newState == ACTIVE) ? "ACTIVE" : "INACTIVE";

    String payload;
    serializeJson(doc, payload);

    String resp = apiCall("PUT", "/state", payload);

    if (resp.startsWith("ERROR") || resp.length() == 0) {
        return ApiResult::API_ERROR;
    }

    if (deserializeJson(doc, resp)) {
        return ApiResult::API_ERROR;
    }

    if (doc["success"] == true || resp.indexOf("updated") != -1) {
        String stateStr = doc["status"] | "INACTIVE";
        outActualState = (stateStr == "ACTIVE") ? ACTIVE : INACTIVE;
        return ApiResult::API_OK;
    }

    return ApiResult::API_ERROR;
}

ApiResult extendValidity(const uint8_t* userId, uint32_t &outNewValidUntil)
{
    String userIdStr = userIdToHex(userId);

    JsonDocument doc;
    doc["uid"] = userIdStr;

    String payload;
    serializeJson(doc, payload);

    String resp = apiCall("POST", "/extend", payload);

    if (resp.startsWith("ERROR") || resp.length() == 0) {
        return ApiResult::API_ERROR;
    }

    if (deserializeJson(doc, resp)) {
        return ApiResult::API_ERROR;
    }

    if (doc["success"] == true) {
        outNewValidUntil = doc["validUntil"] | 0;
        return ApiResult::API_OK;
    }

    return ApiResult::API_ERROR;
}