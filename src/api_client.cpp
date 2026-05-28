#include "api_client.h"
#include <HTTPClient.h>
#include "wifi_logic.h"
#include "globals.h"

int apiCall(String method, String endpoint, String payload, String &outResponse) {
    outResponse = ""; // Czyszczenie bufora na nową odpowiedź

    if (!isWifiConnected()) {
        Serial.println("Cannot make API call: WiFi not connected.");
        return -1; // Własny kod błędu dla braku sieci
    }

    if (serverAddress.length() == 0) {
        Serial.println("API Error: Server address is empty!");
        return -2; // Własny kod błędu dla braku konfiguracji URL
    }

    HTTPClient http;

    // Budowanie pełnego adresu URL i zabezpieczenie przed brakiem/podwójnym "/"
    String fullUrl = serverAddress;
    if (!fullUrl.endsWith("/") && !endpoint.startsWith("/")) {
        fullUrl += "/";
    }
    fullUrl += endpoint;

    http.begin(fullUrl);
    http.addHeader("Content-Type", "application/json");
    http.setConnectTimeout(500);
    http.setTimeout(500);

    int httpCode = 0;
    if (method == "GET") {
        httpCode = http.GET();
    } else if (method == "POST") {
        httpCode = http.POST(payload);
    } else if (method == "PUT") {
        httpCode = http.PUT(payload);
    }

    // Jeśli serwer odpowiedział (kod dodatni, np. 200, 404, 500), pobieramy body
    if (httpCode > 0) {
        outResponse = http.getString();
    } else {
        Serial.printf("HTTP Error (%s): %s\n", fullUrl.c_str(), http.errorToString(httpCode).c_str());
    }

    http.end();
    return httpCode; // Zwraca kod HTTP bazy danych (lub ujemny kod błędu biblioteki)
}