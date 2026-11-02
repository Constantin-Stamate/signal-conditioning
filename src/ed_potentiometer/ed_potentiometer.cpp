#include <Arduino.h>
#include "ed_potentiometer.h"

static constexpr int POTENTIOMETER_PIN = A0;
static constexpr int ADC_MAX_VALUE = 1023;
static constexpr int REFERENCE_MILLIVOLTS = 5000;

static int raw_value = 0;
static int voltage_millivolts = 0;

void ed_potentiometer_setup() {
    pinMode(POTENTIOMETER_PIN, INPUT);
}

void ed_potentiometer_update() {
    raw_value = analogRead(POTENTIOMETER_PIN);

    voltage_millivolts =
        (raw_value * REFERENCE_MILLIVOLTS + ADC_MAX_VALUE / 2)
        / ADC_MAX_VALUE;
}

int ed_potentiometer_get_raw() {
    return raw_value;
}

int ed_potentiometer_get_voltage() {
    return voltage_millivolts;
}