#ifndef PPG_H
#define PPG_H

#include <Arduino.h>

// Deklarasi fungsi
bool ppg_init();
void ppg_sample();
void ppg_getPayload(char* payload);

// Variabel global agar bisa dipantau dari main jika perlu
extern float red_raw_global;
extern float ir_raw_global;
extern float red_final_global;
extern float ir_final_global;

#endif