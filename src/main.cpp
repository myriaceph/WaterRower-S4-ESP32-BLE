#include <Arduino.h>
#include "waterrower.h"
#include "ble-srvice.h"

static WaterRower rower(Serial);
static BleFtmsService bleFtms("WaterRower-S4");

void handleRowerEvent(const WaterRowerEvent& event) {
    if (!bleFtms.isConnected()) {
        return;
    }

    if (event.type == "stroke_rate" && event.hasValue) {
        uint8_t strokeRate = static_cast<uint8_t>(event.value);
        bleFtms.setAverageStrokeRate(strokeRate);
        bleFtms.notifyRowerData();
        return;
    }

    if (event.type == "watts" && event.hasValue) {
        uint16_t power = static_cast<uint16_t>(event.value);
        bleFtms.setInstantaneousPower(power);
        bleFtms.notifyRowerData();
        return;
    }

    if (event.type == "total_distance_m" && event.hasValue) {
        uint32_t distance = static_cast<uint32_t>(event.value);
        bleFtms.setTotalDistance(distance);
        bleFtms.notifyRowerData();
        return;
    }
}

void setup() {
    Serial.begin(19200);
    delay(100);

    bleFtms.begin();

    rower.begin();
    rower.open();
    rower.registerCallback(handleRowerEvent);
}

void loop() {
    rower.update();
    delay(10);
}
