#include "ppg.h"
#include <MAX30105.h>
#include "filters.h"

// ================= SENSOR & FILTER SETTINGS =================
MAX30105 sensor;
const float kFs = 75.0;
const float kLowPassCutoff = 5.0;
const float kHighPassCutoff = 0.5;

// Instances Filter
LowPassFilter lp_red(kLowPassCutoff, kFs);
LowPassFilter lp_ir(kLowPassCutoff, kFs);
HighPassFilter hp_red(kHighPassCutoff, kFs); 
HighPassFilter hp_ir(kHighPassCutoff, kFs);

// Data Global
float red_raw_global = 0;
float ir_raw_global = 0;
float red_final_global = 0;
float ir_final_global = 0;

static unsigned long t0_ppg;

// ================= INIT =================
bool ppg_init() {
    if (!sensor.begin()) {
        return false;
    }
    // Set sampling rate sensor ke 400sps (internal sensor)
    // Tapi kita akan tarik data di software pada 50Hz (kFs)
    sensor.setSamplingRate(MAX30105::SAMPLING_RATE_400SPS);
    t0_ppg = millis();
    return true;
}

// ================= PROCESS =================
void ppg_sample() {
    auto sample = sensor.readSample(1000);

    float red_raw = (float)sample.red;
    float ir_raw  = (float)sample.ir;

    // 1. Low Pass (Buang noise frekuensi tinggi)
    float red_lpf = lp_red.process(red_raw);
    float ir_lpf  = lp_ir.process(ir_raw);

    // 2. High Pass (Buang DC Offset / Baseline Drift)
    float red_final = hp_red.process(red_lpf);
    float ir_final  = hp_ir.process(ir_lpf);

    // Simpan ke global
    red_raw_global = red_raw;
    ir_raw_global = ir_raw;
    red_final_global = red_final;
    ir_final_global = ir_final;
}

// ================= PAYLOAD =================
void ppg_getPayload(char* payload) {
    // Format: "red_raw":...,"ir_raw":...,"red_f":...,"ir_f":...,"ppg_ts":...
    snprintf(payload, 250,
        "\"red_raw\":%.0f,"
        "\"ir_raw\":%.0f,"
        "\"red_filtered\":%.2f,"
        "\"ir_filtered\":%.2f,"
        "\"ppg_ts\":%lu",
        red_raw_global,
        ir_raw_global,
        red_final_global,
        ir_final_global,
        millis() - t0_ppg
    );
}