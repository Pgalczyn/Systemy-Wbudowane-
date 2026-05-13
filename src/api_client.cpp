#include "api_client.h"
#include "globals.h"
#include <HTTPClient.h>
#include <WiFi.h>

const String SERVER_URL = "http://192.168.1.100:5000/api";

String apiCall(String method, String endpoint, String payload) {
    if (WiFi.status() != WL_CONNECTED) {
        consolePrintf("API: No WiFi connection!\n");
        return "ERROR_WIFI";
    }

    HTTPClient http;
    http.begin(SERVER_URL + endpoint);
    http.addHeader("Content-Type", "application/json");
    // Tu możesz dodać API-KEY jeśli serwer go wymaga:
    // http.addHeader("X-API-Key", "twoj-klucz");

    int httpCode = 0;
    if (method == "GET") httpCode = http.GET();
    else if (method == "POST") httpCode = http.POST(payload);
    else if (method == "PUT") httpCode = http.PUT(payload);

    String response = "ERROR_HTTP";
    if (httpCode > 0) {
        response = http.getString();
    } else {
        consolePrintf("HTTP Error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
    return response;
}