#include <Arduino.h>
#include "cmd_tools.h"
#include "globals.h"

String commandBuffer = "";

void logMsg(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    String msg = String(buffer);
    // Zamień każde "\n" na "\r\n" tylko dla Telnetu, jeśli trzeba
    // ale najpierw wyślij czysty tekst na Serial
    Serial.print(msg);

    if (telnetClient && telnetClient.connected()) {
        // PuTTY potrzebuje \r, żeby wrócić na początek linii
        msg.replace("\n", "\r\n"); 
        telnetClient.print(msg);
    }
}

bool getCommand(String &result) {
    char c = 0;
    bool isTelnet = false;

    if (Serial.available()) {
        c = Serial.read();
    } else if (telnetClient && telnetClient.available()) {
        c = telnetClient.read();
        isTelnet = true;
    }

    if (c == 0) return false;

    // FILTR TELNETU: Ignoruj bajty negocjacji (wszystko powyżej 127)
    // To usunie te "krzaki" ␟
    if ((unsigned char)c > 127) return false;

    if (c == '\r' || c == '\n') {
        result = commandBuffer;
        commandBuffer = "";
        return true;
    } 
    else if (c == 8 || c == 127) { // Backspace
        if (commandBuffer.length() > 0) {
            commandBuffer.remove(commandBuffer.length() - 1);
            logMsg("\b \b");
        }
    } 
    else {
        commandBuffer += c;
        
        // ECHO: Odsyłaj znak tylko jeśli nie jest to "pusty" znak
        if (c >= 32) { 
            char echo[2] = {c, 0};
            logMsg(echo); 
        }
    }
    return false;
}