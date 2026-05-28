#include "app_service.h"
#include "api_client.h"
#include <ArduinoJson.h>

bool registerMember(String cardUid, String name, String surname, String email)
{
    JsonDocument doc;

    doc["cardUid"] = cardUid;
    doc["name"] = name;
    doc["surname"] = surname;
    doc["email"] = email;

    String payload;
    serializeJson(doc, payload);

    String resp = apiCall("POST", "/register", payload);

    DeserializationError error = deserializeJson(doc, resp);
    if (error) {
        Serial.println("Failed to parse API response.");
        return false;
    }

    return resp.indexOf("success") != -1;
}

bool checkMemberData(String cardUid)
{
    String resp = apiCall("GET", "/member/" + cardUid);

    if (resp.startsWith("ERROR"))
        return false;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, resp);
    if (error)
        return false;

    return doc["exists"] == true;
}

bool changeMembershipState(String cardUid, MembershipState newState)
{
    JsonDocument doc;
    doc["uid"] = cardUid;
    doc["status"] = (newState == ACTIVE) ? "ACTIVE" : "INACTIVE";

    String payload;
    serializeJson(doc, payload);

    String resp = apiCall("PUT", "/state", payload);
    return resp.indexOf("updated") != -1;
}

bool modifyPoints(String uid, int32_t amount)
{
    JsonDocument doc;
    doc["uid"] = uid;
    doc["amount"] = amount;

    String payload;
    serializeJson(doc, payload);

    String resp = apiCall("POST", "/points", payload);

    if (resp.indexOf("ERROR") == -1)
    {
        JsonDocument resDoc;
        deserializeJson(resDoc, resp);
        int newTotal = resDoc["new_total"];
        printf("Punkty zaktualizowane. Obecnie: %d\n", newTotal);
        return true;
    }
    return false;
}