#include <Arduino.h>
#include "app/application.h"

void setup() {
    application_init();
}

void loop() {
    application_execute();
}