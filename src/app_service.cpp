#include "app_service.h"
#include "api_client.h"
#include <ArduinoJson.h>

static String userIdToHex(const uint8_t* userId) {
    String hex = "";
    for (int i = 0; i < 16; i++) {
        if (userId[i] < 0x10) hex += "0";
        hex += String(userId[i], HEX);
    }
    hex.toUpperCase();
    return hex;
}

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
    doc["cardUid"] = req.cardUid;
    doc["name"] = req.name;
    doc["surname"] = req.surname;
    doc["email"] = req.email;

    String payload;
    serializeJson(doc, payload);

    String resp;
    int httpCode = apiCall("POST", "/register", payload, resp);

    // HTTP 200 oznacza sukces. Jeśli jest inny lub JSON jest uszkodzony -> błąd.
    if (httpCode != 200 || deserializeJson(doc, resp)) {
        return ApiResult::API_ERROR;
    }

    outData.validUntil = doc["validUntil"].as<uint32_t>();
    outData.points = doc["points"].as<int32_t>();

    hexToUserId(doc["userId"].as<String>(), outData.userId);

    return ApiResult::API_OK;
}

ApiResult checkMemberData(const uint8_t* userId, MemberDataResponse &outData, bool isGate)
{
    String userIdStr = userIdToHex(userId);
    String url = "/member/" + userIdStr;
    if (isGate) {
        url += "?gate=true";
    }
    
    String resp;
    int httpCode = apiCall("GET", url, "", resp);

    JsonDocument doc;
    if (httpCode != 200 || deserializeJson(doc, resp)) {
        return ApiResult::API_ERROR;
    }

    outData.validUntil = doc["validUntil"].as<uint32_t>();
    outData.points = doc["points"].as<int32_t>();
    
    String stateStr = doc["status"].as<String>();
    if (stateStr.length() == 0) {
        stateStr = "INACTIVE";
    }
    outData.state = (stateStr == "ACTIVE") ? ACTIVE : INACTIVE;
    
    hexToUserId(doc["userId"].as<String>(), outData.userId);

    return ApiResult::API_OK;
}

ApiResult modifyPoints(const uint8_t* userId, int32_t amount, int32_t &outNewTotal)
{
    String userIdStr = userIdToHex(userId);

    JsonDocument doc;
    doc["amount"] = amount;

    String payload;
    serializeJson(doc, payload);

    String resp;
    String url = "/member/" + userIdStr + "/points";
    int httpCode = apiCall("PUT", url, payload, resp);

    if (httpCode != 200 || deserializeJson(doc, resp)) {
        return ApiResult::API_ERROR;
    }

    outNewTotal = doc["newTotal"].as<int32_t>();
    printf("Points updated via API. Current total: %d\n", outNewTotal);
    
    return ApiResult::API_OK;
}

ApiResult changeMembershipState(const uint8_t* userId, MembershipState newState, MembershipState &outActualState)
{
    String userIdStr = userIdToHex(userId);

    JsonDocument doc;
    doc["status"] = (newState == ACTIVE) ? "ACTIVE" : "INACTIVE";

    String payload;
    serializeJson(doc, payload);

    String resp;
    String url = "/member/" + userIdStr + "/state";
    int httpCode = apiCall("PUT", url, payload, resp);

    if (httpCode != 200 || deserializeJson(doc, resp)) {
        return ApiResult::API_ERROR;
    }

    String stateStr = doc["status"].as<String>();
    if (stateStr.length() == 0) {
        stateStr = "INACTIVE";
    }
    outActualState = (stateStr == "ACTIVE") ? ACTIVE : INACTIVE;
    
    return ApiResult::API_OK;
}

ApiResult extendValidity(const uint8_t* userId, uint32_t &outNewValidUntil)
{
    String userIdStr = userIdToHex(userId);

    String resp;
    int httpCode = apiCall("PUT", "/member/" + userIdStr + "/extend-validity", "", resp);

    JsonDocument doc;
    if (httpCode != 200 || deserializeJson(doc, resp)) {
        return ApiResult::API_ERROR;
    }

    outNewValidUntil = doc["validUntil"].as<uint32_t>();
    
    return ApiResult::API_OK;
}