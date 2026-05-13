#include <Arduino.h>
#include "cmd_prompt.h"
#include "globals.h"

String readStringWithEcho(const char* prompt) {
    Serial.print(prompt);
    String input = "";
    
    // Czyścimy bufor, żeby stare znaki nie "wskoczyły" do odpowiedzi
    while(Serial.available()) Serial.read();

    while (true) {
        if (Serial.available()) {
            char c = Serial.read();

            if (c == '\r' || c == '\n') { // Enter
                Serial.println();
                input.trim();
                return input;
            } 
            else if (c == 8 || c == 127) { // Backspace
                if (input.length() > 0) {
                    input.remove(input.length() - 1);
                    Serial.print("\b \b"); // Kasowanie znaku na ekranie
                }
            } 
            else if (isprint(c)) { // Zwykły znak
                input += c;
                Serial.print(c); // ECHO
            }
        }
        yield(); // Żeby Wi-Fi nie zdechło w tle
    }
}

void showPrompt()
{
    switch (currentMode)
    {
    case MODE_NONE:
        Serial.print(F("> "));
        break;
    case MODE_RECEPTION:
        Serial.print(F("[RECEPTION] > "));
        break;
    case MODE_GATE:
        Serial.print(F("[GATE] > "));
        break;
    }
}