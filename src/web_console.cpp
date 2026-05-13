#include "globals.h"
#include "cmd_prompt.h"
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>
#include <WiFi.h>

AsyncWebServer server(80);

static void handleNotFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

void consolePrintf(const char* format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Serial.print(buffer);
    WebSerial.print(buffer);
}

void setupWebConsole()
{
    server.onNotFound(handleNotFound);
    WebSerial.onMessage(handleWebSerialInput);
    WebSerial.setBuffer(0); // disable internal buffering — send messages immediately
    WebSerial.begin(&server, "/");
    server.begin();
}