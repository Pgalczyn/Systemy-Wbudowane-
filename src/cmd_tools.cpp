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

    if (telnetClient && telnetClient.available()) {
        c = telnetClient.read();
    } else {
        return false;
    }

    if (c == 0) return false;

    // OBSŁUGA TELNETU: rozpoznaj IAC (255) i skonsumuj całe sekwencje negocjacyjne
    unsigned char uc = (unsigned char)c;
    if (uc > 127) {
        if (uc == 255) { // IAC - początek komendy Telnet
            if (telnetClient && telnetClient.available()) {
                unsigned char cmd = (unsigned char)telnetClient.read();
                // WILL(251)/WONT(252)/DO(253)/DONT(254) -> następny bajt to opcja
                if (cmd >= 251 && cmd <= 254) {
                    if (telnetClient.available()) telnetClient.read();
                }
                // SB (250) - subnegocjacja aż do IAC(255) SE(240)
                else if (cmd == 250) {
                    while (telnetClient && telnetClient.available()) {
                        unsigned char b = (unsigned char)telnetClient.read();
                        if (b == 255) {
                            if (telnetClient.available()) {
                                unsigned char b2 = (unsigned char)telnetClient.read();
                                if (b2 == 240) break; // SE
                            } else break;
                        }
                    }
                }
            }
        }
        // ignoruj pozostałe bajty poza ASCII
        return false;
    }

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