#pragma once
#include <Arduino.h>

String apiCall(String method, String endpoint, String payload = "");