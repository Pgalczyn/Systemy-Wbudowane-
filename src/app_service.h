#pragma once
#include <Arduino.h>

enum class ApiResult {
    API_OK,
    API_ERROR
};

enum MembershipState { ACTIVE, INACTIVE };

struct RegisterRequest {
    const uint8_t* userId;
    String name;
    String surname;
    String email;
};

struct MemberDataResponse {
    uint8_t userId[16];
    uint32_t validUntil;
    int32_t points;
    MembershipState state;
};

ApiResult registerMember(const RegisterRequest &req, MemberDataResponse &outData);
ApiResult checkMemberData(const uint8_t* userId, MemberDataResponse &outData);
ApiResult extendValidity(const uint8_t* userId, uint32_t &outNewValidUntil);
ApiResult modifyPoints(const uint8_t* userId, int32_t amount, int32_t &outNewTotal);
ApiResult changeMembershipState(const uint8_t* userId, MembershipState newState, MembershipState &outActualState);