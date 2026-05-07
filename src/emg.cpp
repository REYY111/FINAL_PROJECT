#include "emg.h"
#include "filters.h"
#include <Arduino.h>

#define EMG_PIN   34
#define LO_PLUS   25
#define LO_MINUS  26

// ============================
// LOCAL NOTCH CLASS (Bypass filters.h)
// ============================
class CustomNotch {
    float b0, b1, b2, a0, a1, a2;
    float x1, x2, y1, y2;
public:
    CustomNotch(float f_notch, float bw, float f_s) {
        float wo = 2.0 * PI * f_notch / f_s;
        float alpha = sin(2.0 * PI * bw / f_s) / 2.0;
        b0 = 1.0;
        b1 = -2.0 * cos(wo);
        b2 = 1.0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cos(wo);
        a2 = 1.0 - alpha;
        x1 = x2 = y1 = y2 = 0;
    }
    float process(float x) {
        float y = (b0/a0)*x + (b1/a0)*x1 + (b2/a0)*x2 - (a1/a0)*y1 - (a2/a0)*y2;
        x2 = x1; x1 = x; y2 = y1; y1 = y;
        return y;
    }
};

// ============================
// PARAMETER & INSTANCE
// ============================
const float kFs = 1000.0;
CustomNotch notch_emg(50.0, 2.0, kFs); // Notch 50Hz
HighPassFilter hp_emg(20.0, kFs);      // Pakai yang ada di filters.h
LowPassFilter lp_emg(450.0, kFs);      // Pakai yang ada di filters.h

// Moving Average Params
#define MA_WINDOW_SIZE 50 
float ma_buffer[MA_WINDOW_SIZE];
int ma_idx = 0;
float ma_sum = 0;

static unsigned long t0;
volatile int emg_buffer[EMG_BATCH_SIZE];
volatile int emg_idx = 0;
volatile bool emg_dataReady = false;

// ============================
// INIT
// ============================
void emg_init() {
    analogReadResolution(12);
    pinMode(LO_PLUS, INPUT);
    pinMode(LO_MINUS, INPUT);
    t0 = millis();

    for(int i = 0; i < EMG_BATCH_SIZE; i++) emg_buffer[i] = 0;
    for(int i = 0; i < MA_WINDOW_SIZE; i++) ma_buffer[i] = 0;
    ma_sum = 0;
}

// ============================
// SAMPLING + PROCESSING
// ============================
void emg_sample() {
    int val = 0;

    if (digitalRead(LO_PLUS) || digitalRead(LO_MINUS)) {
        val = 0; 
    } else {
        float raw = (float)analogRead(EMG_PIN);

        // 1. Notch (Custom Class di atas)
        float nt = notch_emg.process(raw);

        // 2. Bandpass (Class dari filters.h)
        float hp = hp_emg.process(nt);
        float bp = lp_emg.process(hp);

        // 3. Rectify
        float rectified = abs(bp);

        // 4. Moving Average (Envelope)
        ma_sum -= ma_buffer[ma_idx];
        ma_buffer[ma_idx] = rectified;
        ma_sum += rectified;
        ma_idx = (ma_idx + 1) % MA_WINDOW_SIZE;

        val = (int)(ma_sum / MA_WINDOW_SIZE);
    }

    emg_buffer[emg_idx] = val;
    emg_idx = (emg_idx + 1) % EMG_BATCH_SIZE;
    if (emg_idx == 0) emg_dataReady = true;
}

// ============================
// PAYLOAD
// ============================
void emg_getPayload(char* payload) {
    int pos = sprintf(payload, "\"emg_raw\":[");
    for (int i = 0; i < EMG_BATCH_SIZE; i++) {
        int read_idx = (emg_idx + i) % EMG_BATCH_SIZE;
        pos += sprintf(payload + pos, "%d%s", emg_buffer[read_idx], (i < EMG_BATCH_SIZE - 1 ? "," : ""));
    }
    sprintf(payload + pos, "],\"emg_ts\":%lu", millis() - t0);
}