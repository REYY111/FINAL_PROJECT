#include "emg.h"
#include <Arduino.h>

// ================= EMG PIN =================
#define EMG_PIN   34
#define LO_PLUS   25
#define LO_MINUS  26

// ================= FILTER =================
const float ALPHA_DC = 0.99;
float last_raw = 0;
float last_dc  = 0;

const float b2 = -1.9021;
const float a2 = -1.8831;
const float a3 = 0.9801;

float x1_n=0, x2_n=0, y1_n=0, y2_n=0;

const float ALPHA_LP = 0.30;
float lp_emg = 0;

const float ALPHA_ENV = 0.05;
float envelope = 0;

static unsigned long t0;

// Gunakan volatile karena diakses antar Core/Task
volatile int rawADC_global;
volatile float clean_global;
volatile float envelope_global;

// ================= INIT =================
void emg_init() {
    analogReadResolution(12);
    pinMode(LO_PLUS, INPUT);
    pinMode(LO_MINUS, INPUT);
    t0 = millis();
}

// ================= 1kHz SAMPLING =================
void emg_sample() {
    // Leads-off Detection
    if (digitalRead(LO_PLUS) || digitalRead(LO_MINUS)) {
        rawADC_global = 0;
        clean_global = 0;
        envelope_global = 0;
        return;
    }

    // Oversampling simple
    int rawADC = 0;
    for(int i=0; i<4; i++) rawADC += analogRead(EMG_PIN);
    rawADC /= 4;

    float x0 = (float)rawADC;

    // 1. DC Removal
    float dc_filtered = x0 - last_raw + (ALPHA_DC * last_dc);
    last_raw = x0;
    last_dc  = dc_filtered;

    // 2. Notch Filter (50Hz @1kHz sampling)
    float notch_out = dc_filtered + (b2 * x1_n) + x2_n - (a2 * y1_n) - (a3 * y2_n);
    x2_n = x1_n;
    x1_n = dc_filtered;
    y2_n = y1_n;
    y1_n = notch_out;

    // 3. Low Pass Filter (Smoothing)
    lp_emg = (ALPHA_LP * notch_out) + ((1.0 - ALPHA_LP) * lp_emg);

    // 4. Rectification & Envelope
    float rectified = abs(lp_emg);
    envelope = (ALPHA_ENV * rectified) + ((1.0 - ALPHA_ENV) * envelope);

    // Simpan ke global
    rawADC_global = rawADC;
    clean_global = lp_emg;
    envelope_global = envelope;
}

// ================= PAYLOAD (FIXED) =================
void emg_getPayload(char* payload) {
    // Kita hilangkan kurung kurawal {} di sini supaya bisa digabung rapi
    // Format: "raw":...,"clean":...,"envelope":...,"emg_ts":...
    
    snprintf(payload, 200,
        "\"raw\":%d,"
        "\"clean\":%.2f,"
        "\"envelope\":%.2f,"
        "\"emg_ts\":%lu",
        rawADC_global,
        clean_global,
        envelope_global,
        millis() - t0
    );
}