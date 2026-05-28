#pragma once
#include <Arduino.h>

enum MembershipState { ACTIVE = 1, INACTIVE = 0};

struct RegisterResult {
    bool success;
    String errorMessage;
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

// ZAKTUALIZOWANE DEKLARACJE FUNKCJI
RegisterResult registerMember(String cardUid, String name, String surname, String gymMembershipStarts, String gymMembershipEnds, String email, String coffeePoints);
MemberDataResult getMemberData(String cardUid);
StateChangeResult changeMembershipState(String cardUid, MembershipState newState);
ModifyPointsResult modifyPoints(String card_UID, int32_t amount);
LogScanResult logGymScan(String card_UID);