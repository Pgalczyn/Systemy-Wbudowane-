#pragma once
#include <Arduino.h>

enum MembershipState { ACTIVE, INACTIVE };

bool registerMember(String cardUid, String name, String surname, String email);
bool checkMemberData(String cardUid);
bool changeMembershipState(String cardUid, MembershipState newState);
bool modifyPoints(String cardUid, int32_t amount);