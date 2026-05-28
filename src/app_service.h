#pragma once
#include <Arduino.h>

enum MembershipState { ACTIVE = 1, INACTIVE = 0};

struct RegisterResult {
    bool success;
    String errorMessage;
    String gymMembershipStarts;
    String gymMembershipEnds;
    int32_t coffeePoints;
};

struct MemberDataResult {
    bool success;
    String errorMessage;
    String name;
    String surname;
    int coffeePoints;
    String membershipEnds;
    int totalSessions;
    String lastEnterDate;
    bool isAtTheGym;
};

struct StateChangeResult {
    bool success;
    String errorMessage;
};

struct ModifyPointsResult {
    bool success;
    String errorMessage;
};

struct LogScanResult {
    bool success;
    String errorMessage;
};
struct ExtendMembershipResult {
    bool success;
    String errorMessage;
    String newMembershipStarts;
    String newMembershipEnds;
    int32_t coffeePoints;
};

RegisterResult registerMember(String cardUid, String name, String surname, String email);
MemberDataResult getMemberData(String cardUid);
StateChangeResult changeMembershipState(String cardUid, MembershipState newState);
ModifyPointsResult modifyPoints(String card_UID, int32_t amount);
LogScanResult logGymScan(String card_UID);
ExtendMembershipResult extendMembership(String cardUid, int months);