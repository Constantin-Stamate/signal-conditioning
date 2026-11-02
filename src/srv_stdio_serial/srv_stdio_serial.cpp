#include <Arduino.h>
#include <stdio.h>

#include "srv_stdio_serial.h"

static int serial_putchar(char character, FILE*) {
    Serial.write(static_cast<uint8_t>(character));
    return 0;
}

static int serial_getchar(FILE*) {
    while (!Serial.available()) {
    }

    return Serial.read();
}

static FILE serial_stream;

void srv_stdio_serial_setup() {
    Serial.begin(9600);

    fdev_setup_stream(
        &serial_stream,
        serial_putchar,
        serial_getchar,
        _FDEV_SETUP_RW
    );

    stdout = &serial_stream;
    stdin = &serial_stream;
}