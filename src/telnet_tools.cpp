#include <Arduino.h>
#include "telnet_tools.h"
#include "globals.h"
#include <stdarg.h>
#include <stdio.h>

String commandBuffer = "";

void telnetPrintFmt(const char *format, ...)
{
    if (!isTelnetConnected())
        return;

    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    String msg(buffer);
    msg.replace("\n", "\r\n");
    telnetClient.print(msg);
}

bool getCommand(String &result)
{
    // Read only when data is available to avoid -1 / EOF being misinterpreted
    if (!isTelnetConnected())
        return false;

    if (!telnetClient.available())
        return false;

    int r = telnetClient.read();
    if (r < 0)
        return false;
    char c = (char)r;

    // OBSŁUGA TELNETU: rozpoznaj IAC (255) i skonsumuj całe sekwencje negocjacyjne
    unsigned char uc = (unsigned char)c;
    if (uc > 127)
    {
        if (uc == 255)
        { // IAC - początek komendy Telnet
            if (isTelnetConnected())
            {
                unsigned char cmd = (unsigned char)telnetClient.read();
                // WILL(251)/WONT(252)/DO(253)/DONT(254) -> następny bajt to opcja
                if (cmd >= 251 && cmd <= 254)
                {
                    if (isTelnetConnected())
                        telnetClient.read();
                }
                // SB (250) - subnegocjacja aż do IAC(255) SE(240)
                else if (cmd == 250)
                {
                    while (telnetClient && telnetClient.available())
                    {
                        unsigned char b = (unsigned char)telnetClient.read();
                        if (b == 255)
                        {
                            if (telnetClient.available())
                            {
                                unsigned char b2 = (unsigned char)telnetClient.read();
                                if (b2 == 240)
                                    break; // SE
                            }
                            else
                                break;
                        }
                    }
                }
            }
        }
        // ignoruj pozostałe bajty poza ASCII
        return false;
    }

    // Handle CR and LF. If LF follows CR (common CRLF), ignore the second empty terminator.
    if (c == '\r')
    {
        result = commandBuffer;
        commandBuffer = "";
        // move client cursor to new line so subsequent menu prints start on next line
        if (isTelnetConnected())
            telnetClient.print("\r\n");
        return true;
    }
    else if (c == '\n')
    {
        if (commandBuffer.length() == 0)
            return false; // stray LF after CR or empty line, ignore
        result = commandBuffer;
        commandBuffer = "";
        if (isTelnetConnected())
            telnetClient.print("\r\n");
        return true;
    }
    else if (c == 8 || c == 127)
    { // Backspace
        if (commandBuffer.length() > 0)
        {
            commandBuffer.remove(commandBuffer.length() - 1);
            telnetPrintFmt("\b \b");
        }
    }
    else
    {
        commandBuffer += c;

        // ECHO: Odsyłaj znak tylko jeśli nie jest to "pusty" znak
        if (c >= 32)
        {
            char echo[2] = {c, 0};
            telnetPrintFmt(echo);
        }
    }
    return false;
}

void handleTelnet()
{
  if (telnetClient && !telnetClient.connected())
  {
    telnetClient.stop();
  }

  if (telnetServer.hasClient())
  {
    WiFiClient newClient = telnetServer.available();
    if (!telnetClient || !telnetClient.connected())
    {
      telnetClient = newClient;
      telnetPrintFmt("=== ESP32 REMOTE TERMINAL ===\n");
    }
    else
    {
      newClient.stop();
    }
  }
}

bool isTelnetConnected()
{
    return telnetClient && telnetClient.connected();
}