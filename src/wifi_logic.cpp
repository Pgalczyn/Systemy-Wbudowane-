#include <WiFi.h>
#include "globals.h"

bool isWifiConnected() {
    if (WiFi.status() != WL_CONNECTED) {
        printf("Device is offline: Use wifi command to configure wifi access.\n");
        return false;
    }
    return true;
}