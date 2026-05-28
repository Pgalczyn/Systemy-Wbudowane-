#include "app_service.h"
#include "api_client.h"
#include <ArduinoJson.h>
#include "hex_util.h"

String userIdBytesToHexString(const uint8_t* userId) {
    return bytesToHex(userId, 16);
}

static void userIdHexStringToBytes(const String& hexStr, uint8_t* outUserId) {
    hexToBytes(hexStr, outUserId, 16);
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
    userIdHexStringToBytes(doc["userId"].as<String>(), outData.userId);

    return ApiResult::API_OK;
}

ApiResult checkMemberData(const uint8_t* userId, MemberDataResponse &outData, bool isGate)
{
    String userIdStr = userIdBytesToHexString(userId);
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
    
    userIdHexStringToBytes(doc["userId"].as<String>(), outData.userId);

    return ApiResult::API_OK;
}

ApiResult modifyPoints(const uint8_t* userId, int32_t amount, int32_t &outNewTotal)
{
    String userIdStr = userIdBytesToHexString(userId);

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

    return ApiResult::API_OK;
}

ApiResult changeMembershipState(const uint8_t* userId, MembershipState newState, MembershipState &outActualState)
{
    String userIdStr = userIdBytesToHexString(userId);

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
    String userIdStr = userIdBytesToHexString(userId);

    String resp;
    int httpCode = apiCall("PUT", "/member/" + userIdStr + "/extend-validity", "", resp);

    JsonDocument doc;
    if (httpCode != 200 || deserializeJson(doc, resp)) {
        return ApiResult::API_ERROR;
    }

    outNewValidUntil = doc["validUntil"].as<uint32_t>();
    
    return ApiResult::API_OK;
}

ApiResult rollbackMember(const uint8_t* userId)
{
    String userIdStr = userIdBytesToHexString(userId);

    String resp;
    int httpCode = apiCall("POST", "/member/" + userIdStr + "/rollback", "", resp);

    if (httpCode != 200) {
        return ApiResult::API_ERROR;
    }
    
    return ApiResult::API_OK;
}