#include "app_service.h"
#include "api_client.h"
#include <ArduinoJson.h>

bool registerMember(String name, String surname, String gymMembershipStarts, String gymMembershipEnds, String coffeePoints)
{
    JsonDocument doc;

    doc["name"] = name;
    doc["surname"] = surname;
    doc["email"] = "student@agh.edu.pl";
    doc["gymMembershipStarts"] = gymMembershipStarts;
    doc["gymMembershipEnds"] = gymMembershipEnds;

    doc["coffeePoints"] = coffeePoints.toInt();

    String payload;
    serializeJson(doc, payload);

    String resp = apiCall("POST", "/register", payload);

    return resp.indexOf("success") != -1;
}

bool checkMemberData(String uid)
{
    String resp = apiCall("GET", "/member/" + uid);

    if (resp.startsWith("ERROR"))
        return false;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, resp);
    if (error)
        return false;

    return doc["exists"] == true;
}

bool changeMembershipState(String uid, MembershipState newState)
{
    JsonDocument doc;
    doc["uid"] = uid;
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