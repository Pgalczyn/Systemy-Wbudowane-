#include "api_client.h"
#include <HTTPClient.h>
#include "wifi_logic.h"
#include "globals.h"

String apiCall(String method, String endpoint, String payload) {
    if (!isWifiConnected()) {
        Serial.println("Cannot make API call: WiFi not connected.");
        return "???ERROR_WIFI_DISCONNECTED???";
    }

    if (serverAddress.length() == 0) {
        Serial.println("API Error: Server address is empty!");
        return "ERROR_EMPTY_ADDRESS";
    }

    HTTPClient http;

    String fullUrl = serverAddress;
    if (!fullUrl.endsWith("/") && !endpoint.startsWith("/")) {
        fullUrl += "/";
    }
    fullUrl += endpoint;

    http.begin(fullUrl);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(2000);

    int httpCode = 0;
    if (method == "GET") httpCode = http.GET();
    else if (method == "POST") httpCode = http.POST(payload);
    else if (method == "PUT") httpCode = http.PUT(payload);

    String response = "ERROR_HTTP";
    if (httpCode > 0) {
        response = http.getString();
    } else {
        Serial.printf("HTTP Error (%s): %s\n", fullUrl.c_str(), http.errorToString(httpCode).c_str());
    }

    http.end();
    return response;
}