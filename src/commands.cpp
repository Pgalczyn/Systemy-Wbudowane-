#include "wifi_logic.h"
#include "globals.h"
#include <WiFi.h>
#include "cmd_prompt.h"
#include "commands.h"

int cmdReception(int argc, char** argv)
{
    if (!isWifiConnected())
    {
        printf("Error: WiFi not connected. Cannot enter Reception mode.\n");
        return 1; // Po prostu wychodzimy z funkcji
    }

    currentMode = MODE_RECEPTION;
    printf("Mode: RECEPTION active.\n");
    showPrompt();
    return 0;
}

int cmdGate(int argc, char** argv)
{
    if (!isWifiConnected())
    {
        printf("Error: WiFi not connected. Cannot enter Gate mode.\n");
        return 1    ;
    }

    currentMode = MODE_GATE;
    printf("Mode: GATE active.\n");
    showPrompt();
    return 0;
}

int cmdWifi(int argc, char** argv)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        printf("Już połączono z: %s (IP: %s)\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
        return 0;
    }

    printf("Skanowanie sieci...\n");
    int n = WiFi.scanNetworks();

    if (n <= 0)
    {
        printf("Nie znaleziono żadnych sieci.\n");
        return 1;
    }

    for (int i = 0; i < n; ++i)
    {
        printf("%2d. %-20s (%d dBm)\n", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
    }

    String selection = readStringWithEcho("\nWybierz numer sieci: ");
    selection.trim();
    if (selection.length() == 0)
    {
        WiFi.scanDelete();
        return 1;
    }

    int sel = selection.toInt();

    if (sel < 1 || sel > n)
    {
        printf("Błąd: Nieprawidłowy numer.\n");
        WiFi.scanDelete();
        return 1;
    }

    String ssid = WiFi.SSID(sel - 1);

    char promptMsg[64];
    snprintf(promptMsg, sizeof(promptMsg), "Hasło dla %s: ", ssid.c_str());

    String pass = readStringWithEcho(promptMsg);

    WiFi.begin(ssid.c_str(), pass.c_str());
    printf("Łączenie");

    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout++ < 40)
    {
        delay(500);
        printf(".");
        fflush(stdout);
    }

    WiFi.scanDelete();

    if (WiFi.status() == WL_CONNECTED)
    {
        printf("\nPOŁĄCZONO! IP: %s\n", WiFi.localIP().toString().c_str());
    }
    else
    {
        printf("\nBŁĄD: Nie udało się nawiązać połączenia.\n");
    }
    return (WiFi.status() == WL_CONNECTED) ? 0 : 1;
}

int cmdWifiDisconnect(int argc, char** argv)
{
    WiFi.disconnect(true, true);
    printf("Disconnected. Wifi credentials cleared.\n");
    return 0;
}