#include "app_service.h"
#include "api_client.h"
#include <ArduinoJson.h>
bool registerMember(String name, String surname, String gymMembershipStarts, String gymMembershipEnds,String email, String coffeePoints)
{
    JsonDocument doc;

    doc["name"] = name;
    doc["surname"] = surname;
    doc["email"] = email;
    doc["card_UID"] = cardUid;
    doc["gymMembershipStarts"] = "2026-11-31T23:59:59.999Z";
    doc["gymMembershipEnds"] = "2026-12-31T23:59:59.999Z";
    doc["coffeePoints"] = 1000;
    
    String payload;
    serializeJson(doc, payload);

    String resp = apiCall("POST", "/register", payload);

    return resp.indexOf("success") != -1;
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
    String resp = apiCall("POST", "/change/membershipState/" + card_UID + "/" + newState);

    if (resp.startsWith("ERROR") || resp.indexOf("error") != -1) {
        return false;
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
    return true;
}

bool logGymScan(String card_UID){

    String resp = apiCall("POST", "/enter/exit/gym/" + cardUid);

    if (resp.startsWith("ERROR") || resp.indexOf("error") != -1) {
        return false;
    }

    return true;
}