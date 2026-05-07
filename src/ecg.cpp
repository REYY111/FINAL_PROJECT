#include "ecg.h"
#include <Arduino.h>

// ================= PIN =================
#define ECG_PIN   35
#define LO_PLUS   32
#define LO_MINUS  33
#define LED_TICK  LED_BUILTIN

#define THRESHOLD 2400

// ================= FILTER COEFF =================

// ===== NOTCH 50 Hz =====
float x_n_1 = 0, x_n_2 = 0;
float y_n_1 = 0, y_n_2 = 0;
const float b0 = 0.9391, b1 = -1.5195, b2 = 0.9391;
const float a1 = -1.5195, a2 = 0.8781;

// ===== HIGH PASS 0.5 Hz =====
float x_h_1 = 0, y_h_1 = 0;
const float a_hp = 0.9937;

// ===== LOW PASS ~100 Hz =====
float y_l_1 = 0;
const float a_lp = 0.6;

// ================= BPM =================
float BPM = 0;
int mavBPM = 0;
unsigned long lastBeatTime = 0;
int flagBPM = 0;

#define N_DATA 5
float bpm_buffer[N_DATA];
int idx = 0;

// ================= GLOBAL OUTPUT =================
volatile int ecg_raw_global = 0;
volatile float ecg_filtered_global = 0;
volatile int bpm_global = 0;
volatile int leadOff_global = 0;

static unsigned long previousMicros = 0;
static unsigned long t0;

// ================= FILTER PIPELINE =================
float apply_filters(float input) {

    // ===== NOTCH 50 Hz =====
    float notch_out =
        b0 * input +
        b1 * x_n_1 +
        b2 * x_n_2 -
        a1 * y_n_1 -
        a2 * y_n_2;

    x_n_2 = x_n_1;
    x_n_1 = input;
    y_n_2 = y_n_1;
    y_n_1 = notch_out;

    // ===== HIGH PASS 0.5 Hz =====
    float hp_out = a_hp * (y_h_1 + notch_out - x_h_1);
    x_h_1 = notch_out;
    y_h_1 = hp_out;

    // ===== LOW PASS ~100 Hz =====
    float lp_out = a_lp * hp_out + (1.0 - a_lp) * y_l_1;
    y_l_1 = lp_out;

    return lp_out;
}

// ================= MOVING AVG BPM =================
float moving_average(float in) {
    float sum = 0;
    bpm_buffer[idx] = in;

    for (int i = 0; i < N_DATA; i++) sum += bpm_buffer[i];

    idx = (idx + 1) % N_DATA;
    return sum / N_DATA;
}

// ================= INIT =================
void ecg_init() {
    analogReadResolution(12);

    pinMode(LO_PLUS, INPUT_PULLUP);
    pinMode(LO_MINUS, INPUT_PULLUP);
    pinMode(LED_TICK, OUTPUT);

    for (int i = 0; i < N_DATA; i++) bpm_buffer[i] = 0;

    x_n_1 = x_n_2 = 0;
    y_n_1 = y_n_2 = 0;
    x_h_1 = y_h_1 = 0;
    y_l_1 = 0;

    t0 = millis();

    Serial.println("✅ ECG READY (0.5–100Hz BANDPASS + 50Hz NOTCH)");
}

// ================= SAMPLE =================
void ecg_sample() {

    unsigned long now = micros();
    if (now - previousMicros < 2000) return; // 500 Hz sampling
    previousMicros = now;

    bool leadOff = digitalRead(LO_PLUS) || digitalRead(LO_MINUS);
    leadOff_global = leadOff;

    if (leadOff) {
        ecg_raw_global = 0;
        bpm_global = 0;
        digitalWrite(LED_TICK, HIGH);
        return;
    }

    // ===== RAW SIGNAL =====
    int raw_val = analogRead(ECG_PIN);
    ecg_raw_global = raw_val;

    // ===== FILTERED SIGNAL =====
    float filtered = apply_filters((float)raw_val);
    ecg_filtered_global = filtered;

    // ===== BPM DETECTION =====
    if (filtered > THRESHOLD && flagBPM == 0) {

        unsigned long duration = millis() - lastBeatTime;

        if (duration > 500) {
            BPM = 60000.0 / duration;
            mavBPM = (int)moving_average(BPM);
            bpm_global = mavBPM;
            lastBeatTime = millis();
            digitalWrite(LED_TICK, HIGH);
        }

        flagBPM = 1;
    }
    else if (filtered < (THRESHOLD - 150)) {
        flagBPM = 0;
        digitalWrite(LED_TICK, LOW);
    }
}

// ================= MQTT PAYLOAD =================
void ecg_getPayload(char* payload) {
    snprintf(payload, 200,
        "\"ecg_raw\":%d,"
        "\"ecg_filtered\":%.2f,"
        "\"bpm\":%d,"
        "\"leadOff\":%d,"
        "\"ecg_ts\":%lu",
        ecg_raw_global,
        ecg_filtered_global,
        bpm_global,
        leadOff_global,
        millis() - t0
    );
}