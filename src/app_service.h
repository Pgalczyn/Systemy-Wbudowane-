#pragma once
#include <Arduino.h>

enum MembershipState { ACTIVE, INACTIVE };

bool registerMember();
bool checkMemberData(String uid);
bool changeMembershipState(String uid, MembershipState newState);
bool modifyPoints(String uid, int32_t amount);