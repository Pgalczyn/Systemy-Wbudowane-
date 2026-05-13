#include <Arduino.h>
#include <WebSerial.h>
#include "cmd_prompt.h"
#include "globals.h"

namespace
{
    String pendingWebSerialInput;
    bool waitingForWebSerialInput = false;
}

void handleWebSerialInput(const String &msg)
{
    if (waitingForWebSerialInput)
    {
        pendingWebSerialInput = msg;
    }
}

String readStringWithEcho(const char *prompt, bool echo)
{
    pendingWebSerialInput = "";
    waitingForWebSerialInput = true;
    WebSerial.print(prompt);

    while (pendingWebSerialInput.isEmpty())
    {
        WebSerial.loop();
        yield();
        delay(1);
    }

    waitingForWebSerialInput = false;
    String input = pendingWebSerialInput;
    pendingWebSerialInput = "";
    input.trim();

    if (echo)
    {
        WebSerial.println(input);
    }
    else
    {
        WebSerial.println();
    }

    return input;
}