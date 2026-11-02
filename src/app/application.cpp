#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <LiquidCrystal.h>
#include <stdio.h>
#include <string.h>

#include "application.h"
#include "../ed_potentiometer/ed_potentiometer.h"
#include "../lib_cond/lib_cond.h"
#include "../srv_stdio_serial/srv_stdio_serial.h"

static LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

static constexpr float REFERENCE_VOLTAGE = 5.0f;
static constexpr float MAX_ANGLE = 270.0f;

static constexpr TickType_t ACQUISITION_PERIOD =
    pdMS_TO_TICKS(20);

static constexpr TickType_t PROCESSING_PERIOD =
    pdMS_TO_TICKS(20);

static constexpr TickType_t REPORT_PERIOD =
    pdMS_TO_TICKS(500);

static QueueHandle_t raw_queue = nullptr;

static Median5 median_filter;
static WeightedAvg4 average_filter;

struct Measurement {
    int raw;
    int millivolts;
    float median_voltage;
    float average_voltage;
    float angle;
};

static volatile int current_raw = 0;
static volatile int current_millivolts = 0;
static volatile float current_median = 0.0f;
static volatile float current_average = 0.0f;
static volatile float current_angle = 0.0f;

static Measurement get_measurement() {
    Measurement m;

    taskENTER_CRITICAL();

    m.raw = current_raw;
    m.millivolts = current_millivolts;
    m.median_voltage = current_median;
    m.average_voltage = current_average;
    m.angle = current_angle;

    taskEXIT_CRITICAL();

    return m;
}

static void report_serial(const Measurement& m) {
    int median_mV =
        static_cast<int>(m.median_voltage * 1000.0f + 0.5f);

    int average_mV =
        static_cast<int>(m.average_voltage * 1000.0f + 0.5f);

    int angle_tenths =
        static_cast<int>(m.angle * 10.0f + 0.5f);

    printf(
        "RAW=%d | Input=%d mV | "
        "Median=%d mV | Average=%d.%03d V | "
        "Angle=%d.%d deg\r\n",
        m.raw,
        m.millivolts,
        median_mV,
        average_mV / 1000,
        average_mV % 1000,
        angle_tenths / 10,
        angle_tenths % 10
    );
}

static void report_lcd(const Measurement& m) {
    char line1[17];
    char line2[17];

    int median_mV =
        static_cast<int>(m.median_voltage * 1000.0f + 0.5f);

    int average_mV =
        static_cast<int>(m.average_voltage * 1000.0f + 0.5f);

    int angle_tenths =
        static_cast<int>(m.angle * 10.0f + 0.5f);

    snprintf(
        line1,
        sizeof(line1),
        "In:%4d Med:%4d",
        m.millivolts,
        median_mV
    );

    snprintf(
        line2,
        sizeof(line2),
        "Avg:%d.%02d A:%d.%d",
        average_mV / 1000,
        (average_mV % 1000) / 10,
        angle_tenths / 10,
        angle_tenths % 10
    );

    lcd.setCursor(0, 0);
    lcd.print(line1);

    for (int i = strlen(line1); i < 16; ++i) {
        lcd.print(' ');
    }

    lcd.setCursor(0, 1);
    lcd.print(line2);

    for (int i = strlen(line2); i < 16; ++i) {
        lcd.print(' ');
    }
}

static void task_acquisition(void*) {
    ed_potentiometer_setup();

    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        ed_potentiometer_update();

        int raw = ed_potentiometer_get_raw();
        xQueueOverwrite(raw_queue, &raw);

        vTaskDelayUntil(&last_wake, ACQUISITION_PERIOD);
    }
}

static void task_processing(void*) {
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        int raw = 0;

        if (xQueueReceive(raw_queue, &raw, 0) == pdTRUE) {
            int millivolts = ed_potentiometer_get_voltage();

            float voltage = millivolts / 1000.0f;

            voltage = lib_cond_saturate(
                voltage, 0.0f, REFERENCE_VOLTAGE
            );

            float median = median_filter.push(voltage);
            float average = average_filter.push(median);

            median = lib_cond_saturate(
                median, 0.0f, REFERENCE_VOLTAGE
            );

            average = lib_cond_saturate(
                average, 0.0f, REFERENCE_VOLTAGE
            );

            float angle =
                average * MAX_ANGLE / REFERENCE_VOLTAGE;

            angle = lib_cond_saturate(
                angle, 0.0f, MAX_ANGLE
            );

            taskENTER_CRITICAL();

            current_raw = raw;
            current_millivolts = millivolts;
            current_median = median;
            current_average = average;
            current_angle = angle;

            taskEXIT_CRITICAL();
        }

        vTaskDelayUntil(&last_wake, PROCESSING_PERIOD);
    }
}

static void task_reporting(void*) {
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        if (median_filter.ready() && average_filter.ready()) {
            Measurement m = get_measurement();

            report_serial(m);
            report_lcd(m);
        }

        vTaskDelayUntil(&last_wake, REPORT_PERIOD);
    }
}

void application_init() {
    srv_stdio_serial_setup();

    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Signal Processing");
    lcd.setCursor(0, 1);
    lcd.print("System Starting...");

    printf("\r\n");
    printf("================================\r\n");
    printf(" Potentiometer Signal Processing\r\n");
    printf("================================\r\n");

    raw_queue = xQueueCreate(1, sizeof(int));

    if (raw_queue == nullptr) {
        printf("ERROR: Failed to create data queue\r\n");

        for (;;) {
        }
    }

    if (xTaskCreate(
            task_acquisition, "acquisition", 192,
            nullptr, 2, nullptr) != pdPASS ||
        xTaskCreate(
            task_processing, "processing", 256,
            nullptr, 2, nullptr) != pdPASS ||
        xTaskCreate(
            task_reporting, "reporting", 256,
            nullptr, 1, nullptr) != pdPASS) {

        printf("ERROR: Failed to create FreeRTOS tasks\r\n");

        for (;;) {
        }
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Signal Processing");
    lcd.setCursor(0, 1);
    lcd.print("System Ready");

    printf("System initialized successfully\r\n");
    printf("Signal processing started\r\n");
    printf("--------------------------------\r\n");

    vTaskStartScheduler();

    for (;;) {
    }
}

void application_execute() {
}