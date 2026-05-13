#pragma once
#include <Arduino.h>

enum MembershipState { ACTIVE, INACTIVE };

bool registerMember(String uid);
bool checkMemberData(String uid); // Zwraca np. czy istnieje
bool changeMembershipState(String uid, MembershipState newState);
bool modifyPoints(String uid, int32_t amount);