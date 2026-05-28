#include "app_service.h"
#include "api_client.h"
#include <ArduinoJson.h>

RegisterResult registerMember(String cardUid, String name, String surname, String email)
{
    // Inicjalizacja struktury zerami/pustymi stringami
    RegisterResult result = {false, "", "", "", 0};
    
    JsonDocument doc;
    doc["name"] = name;
    doc["surname"] = surname;
    doc["email"] = email;
    doc["card_UID"] = cardUid;

    String payload;
    serializeJson(doc, payload);

    String resp = apiCall("POST", "/register", payload);

    // Wstępne sprawdzenie, czy odpowiedź nie jest błędem HTTP
    if (resp.indexOf("success") == -1) {
        result.errorMessage = "Server rejected registration or connection failed.";
        return result;
    }

    // --- PARSOWANIE ODPOWIEDZI ---
    JsonDocument respDoc;
    DeserializationError error = deserializeJson(respDoc, resp);
    
    if (!error && respDoc["status"] == "success") {
        // Dobieramy się do obiektu "user", który odesłał backend
        JsonObject user = respDoc["user"];
        
        // Zapisujemy wygenerowane przez backend dane do naszej struktury
        result.gymMembershipStarts = user["gymMembershipStarts"].as<String>();
        result.gymMembershipEnds = user["gymMembershipEnds"].as<String>();
        result.coffeePoints = user["coffeePoints"];
        
        result.success = true;
    } else {
        result.errorMessage = "Failed to parse user data from server.";
    }

    return result; 
}

MemberDataResult getMemberData(String cardUid)
{
    MemberDataResult result = {false, "", "", "", 0, "", 0, "", false};
    String resp = apiCall("GET", "/getUserData/" + cardUid);

    if (resp.startsWith("ERROR")) {
        result.errorMessage = "API connection error.";
        return result;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, resp);
    if (error) {
        result.errorMessage = "JSON parse error.";
        return result;
    }

    const char* status = doc["status"];
    if (strcmp(status, "success") != 0) {
        result.errorMessage = "User not found.";
        return result;
    }

    JsonObject user = doc["user"];

    result.name = user["name"].as<String>();
    result.surname = user["surname"].as<String>();
    result.coffeePoints = user["coffeePoints"];
    result.membershipEnds = user["gymMembershipEnds"].as<String>();

    JsonArray sessions = doc["sessions"];
    result.totalSessions = sessions.size();

    if (result.totalSessions > 0) {
        JsonObject latestSession = sessions[0];
        result.lastEnterDate = latestSession["enterDate"].as<String>();
        result.isAtTheGym = latestSession["isAtTheGym"];
    }

    result.success = true;
    return result;
}

StateChangeResult changeMembershipState(String cardUid, MembershipState newState)
{
    StateChangeResult result = {false, ""};
    String resp = apiCall("POST", "/change/membershipState/" + cardUid + "/" + String(newState));

    if (resp.startsWith("ERROR") || resp.indexOf("error") != -1) {
        result.errorMessage = "Failed to change membership state.";
        return result;
    }

    result.success = true;
    return result;
}

ModifyPointsResult modifyPoints(String card_UID, int32_t amount)
{
    ModifyPointsResult result = {false, ""};
    if (amount == 0) {
        result.success = true;
        return result;
    }

    String resp;

    if (amount > 0) {
        resp = apiCall("POST", "/add/coffee/points/" + card_UID + "/" + String(amount));
    }
    else {
        resp = apiCall("POST", "/subtract/coffee/points/" + card_UID + "/" + String(abs(amount)));
    }

    if (resp.startsWith("ERROR") || resp.indexOf("error") != -1) {
        result.errorMessage = "Failed to modify points.";
        return result;
    }

    result.success = true;
    return result;
}

LogScanResult logGymScan(String card_UID){
    LogScanResult result = {false, ""};
    String resp = apiCall("POST", "/enter/exit/gym/" + card_UID);

    if (resp.startsWith("ERROR") || resp.indexOf("error") != -1) {
        result.errorMessage = "Failed to log gym scan.";
        return result;
    }

    result.success = true;
    return result;
}

ExtendMembershipResult extendMembership(String cardUid, int months)
{
    ExtendMembershipResult result = {false, "", ""};

    String endpoint = "/extend/membership/" + cardUid + "/" + String(months);
    String resp = apiCall("POST", endpoint);

    if (resp.startsWith("ERROR") || resp.indexOf("error") != -1 || resp.indexOf("success") == -1) {
        result.errorMessage = "Server rejected extension or connection failed.";
        return result;
    }

    JsonDocument respDoc;
    DeserializationError error = deserializeJson(respDoc, resp);

    if (!error && respDoc["status"] == "success") {
        JsonObject user = respDoc["user"];
        result.newMembershipStarts = user["gymMembershipStarts"].as<String>();
        result.newMembershipEnds = user["gymMembershipEnds"].as<String>();
        result.coffeePoints = user["coffeePoints"];
        result.success = true;
    } else {
        result.errorMessage = "Failed to parse updated user data from server.";
    }

    return result;
}