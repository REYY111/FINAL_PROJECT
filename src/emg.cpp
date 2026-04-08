#include "emg.h"
#include <Arduino.h>

#define EMG_PIN   34
#define LO_PLUS   25
#define LO_MINUS  26
#define BATCH_SIZE 10 

static unsigned long t0;
volatile int emg_buffer[BATCH_SIZE];
volatile int emg_idx = 0;

void emg_init() {
    analogReadResolution(12);
    pinMode(LO_PLUS, INPUT);
    pinMode(LO_MINUS, INPUT);
    t0 = millis();
    
    // Inisialisasi buffer dengan 0 agar tidak ada nilai sampah
    for(int i=0; i<BATCH_SIZE; i++) emg_buffer[i] = 0;
}

// JALAN DI CORE 1 (1000Hz)
void emg_sample() {
    int val = 0;
    // Cek Leads-off (Sensor terpasang atau tidak)
    if (digitalRead(LO_PLUS) || digitalRead(LO_MINUS)) {
        val = 0;
    } else {
        val = analogRead(EMG_PIN);
    }

    // SISTEM CIRCULAR: Isi data lalu geser index. 
    // Jika sampai 10, balik ke 0. Tidak akan pernah "stuck".
    emg_buffer[emg_idx] = val;
    emg_idx = (emg_idx + 1) % BATCH_SIZE;
}

// JALAN DI CORE 0 (Dipanggil tiap 10ms)
void emg_getPayload(char* payload) {
    // Kita ambil snapshot data saat ini
    String s = "\"emg_raw\":[";
    for (int i = 0; i < BATCH_SIZE; i++) {
        s += String(emg_buffer[i]);
        if (i < BATCH_SIZE - 1) s += ",";
    }
    s += "],\"emg_ts\":" + String(millis() - t0);
    
    s.toCharArray(payload, 250);
    // Note: Tidak perlu reset emg_idx karena sudah diatur oleh emg_sample (%)
}