#include <Arduino.h>
#include <Wire.h>
#include <MAX30105.h>
#include "filters.h"
#include "ppg.h"

// ================= I2C PIN =================
#define SDA_PIN 21
#define SCL_PIN 22

// ================= SENSOR =================
MAX30105 sensor;

// ================= FILTER PARAM =================
const float kFs = 25.0;
const float kLowPassCutoff = 5.0;

// ================= FILTER =================
LowPassFilter lp_red(kLowPassCutoff, kFs);
LowPassFilter lp_ir(kLowPassCutoff, kFs);

HighPassFilter hp_red(0.5, kFs);
HighPassFilter hp_ir(0.5, kFs);

// ================= GLOBAL DATA =================
float red_raw_g = 0;
float ir_raw_g  = 0;
float red_final_g = 0;
float ir_final_g  = 0;

static unsigned long t0;
static bool sensor_ok = false;

// ================= INIT =================
void ppg_init() {

    Serial.println("Init I2C...");

    // WAJIB untuk ESP32
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(400000);

    delay(100);

    // ⭐ versi library kamu TANPA parameter
    if (!sensor.begin()) {
        Serial.println("❌ MAX30105 not found");
        sensor_ok = false;
        return;
    }

    sensor.setSamplingRate(MAX30105::SAMPLING_RATE_400SPS);

    sensor_ok = true;
    t0 = millis();

    Serial.println("✅ PPG READY");
}

// ================= SAMPLING =================
void ppg_sample() {

    if (!sensor_ok) return;

    // ⭐ fungsi yang DIDUKUNG library kamu
    auto sample = sensor.readSample(10); // timeout kecil (RTOS safe)

    red_raw_g = (float)sample.red;
    ir_raw_g  = (float)sample.ir;

    // ===== FILTER =====
    float red_lpf = lp_red.process(red_raw_g);
    float ir_lpf  = lp_ir.process(ir_raw_g);

    red_final_g = hp_red.process(red_lpf);
    ir_final_g  = hp_ir.process(ir_lpf);
}

// ================= PAYLOAD =================
void ppg_getPayload(char* payload) {

    snprintf(payload,250,
      "\"ppg\":{"
      "\"red_raw\":%.0f,"
      "\"ir_raw\":%.0f,"
      "\"red\":%.2f,"
      "\"ir\":%.2f,"
      "\"ts\":%lu}",
      red_raw_g,
      ir_raw_g,
      red_final_g,
      ir_final_g,
      millis() - t0
    );
}