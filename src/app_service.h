#pragma once
#include <Arduino.h>

enum MembershipState { ACTIVE = 1, INACTIVE = 0};

bool registerMember(String name, String surname, String gymMembershipStarts, String gymMembershipEnds,String email, String coffeePoints);
bool checkMemberData(String cardUid);
bool changeMembershipState(String cardUid, MembershipState newState);
bool modifyPoints(String cardUid, int32_t amount);
bool logGymScan(String card_UID);