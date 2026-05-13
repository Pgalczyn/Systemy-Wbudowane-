#pragma once
#include <Arduino.h>

bool authenticateCard();
void stopComm();

void handleGateLogic();
void handleReceptionLogic();