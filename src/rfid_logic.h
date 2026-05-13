#pragma once
#include <Arduino.h>

bool authenticateCard();
void stopComm();
String uidToHexString();