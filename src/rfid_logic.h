#pragma once
#include <Arduino.h>
#include "globals.h"

bool authenticateCard();
void stopComm();
String uidToHexString();

bool writeDataToCardBlock(byte blockAddress, byte dataArray[16]);
bool readDataFromCardBlock(byte blockAddress, byte buffer[18]);

bool writeRegistrationToCard(String name, String surname, String email, String gymMembershipStarts, String gymMembershipEnds, int32_t points);
bool writeStateToCard(int32_t points, String gymMembershipStarts, String gymMembershipEnds, MembershipState newState);
bool writePointsToCard(int32_t newPoints);