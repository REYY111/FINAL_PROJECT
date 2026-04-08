#include "ecg.h"
#include <Arduino.h>

// ================= PIN =================
#define ECG_PIN   35
#define LO_PLUS   32
#define LO_MINUS  33
#define LED_TICK  LED_BUILTIN

#define THRESHOLD 3000

// ================= BPM =================
float BPM = 0;
int mavBPM = 0;
unsigned long lastBeatTime = 0;
int flagBPM = 0;

// ================= MOVING AVG =================
#define N_DATA 5
float bpm_buffer[N_DATA];
int idx = 0;

// ================= GLOBAL SHARE =================
volatile int ecg_raw_global = 0;
volatile int bpm_global = 0;
volatile int leadOff_global = 0;

// ================= TIMER =================
static unsigned long previousMicros = 0;
static unsigned long t0;

// ------------------------------------------------
float moving_average(float in) {
    float sum = 0;
    bpm_buffer[idx] = in;

    for(int i = 0; i < N_DATA; i++)
        sum += bpm_buffer[i];

    idx = (idx + 1) % N_DATA;
    return sum / N_DATA;
}

// ------------------------------------------------
void ecg_init() {

    analogReadResolution(12);

    pinMode(LO_PLUS, INPUT_PULLUP);
    pinMode(LO_MINUS, INPUT_PULLUP);
    pinMode(LED_TICK, OUTPUT);

    for(int i = 0; i < N_DATA; i++)
        bpm_buffer[i] = 0;

    t0 = millis();

    Serial.println("✅ ECG READY (500 Hz)");
}

// ------------------------------------------------
void ecg_sample() {

    // ===== 500 Hz SAMPLING =====
    unsigned long now = micros();
    if(now - previousMicros < 2000) return; // 2000 µs = 2 ms → 500 Hz
    previousMicros = now;

    // ===== LEAD OFF DETECTION =====
    bool leadOff =
        digitalRead(LO_PLUS) ||
        digitalRead(LO_MINUS);

    leadOff_global = leadOff;

    if(leadOff) {
        ecg_raw_global = 0;
        bpm_global = 0;
        digitalWrite(LED_TICK, HIGH);
        return;
    }

    // ===== READ ECG =====
    int val = analogRead(ECG_PIN);
    ecg_raw_global = val;

    // ===== BPM DETECTION =====
    if(val > THRESHOLD && flagBPM == 0) {

        unsigned long duration = millis() - lastBeatTime;

        if(duration > 500) { // anti noise
            BPM = 60000.0 / duration;
            mavBPM = (int)moving_average(BPM);

            bpm_global = mavBPM;
            lastBeatTime = millis();

            digitalWrite(LED_TICK, HIGH);
        }

        flagBPM = 1;
    }
    else if(val < (THRESHOLD - 200)) {
        flagBPM = 0;
        digitalWrite(LED_TICK, LOW);
    }
}

// ------------------------------------------------
void ecg_getPayload(char* payload) {

    snprintf(payload, 200,
        "\"ecg\":%d,"
        "\"bpm\":%d,"
        "\"leadOff\":%d,"
        "\"ecg_ts\":%lu",
        ecg_raw_global,
        bpm_global,
        leadOff_global,
        millis() - t0
    );
}