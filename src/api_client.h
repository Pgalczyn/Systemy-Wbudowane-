#pragma once
#include <Arduino.h>

int apiCall(String method, String endpoint, String payload, String &outResponse);