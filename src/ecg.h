#ifndef ECG_H
#define ECG_H

#include <Arduino.h>

// Deklarasi fungsi utama
void ecg_init();
void ecg_sample();
void ecg_getPayload(char* buffer);

// Opsional: Jika Anda ingin mengakses nilai ECG/BPM di file main.cpp, 
// tambahkan keyword 'extern' agar compiler tahu variabel ini ada di ecg.cpp
extern volatile int ecg_raw_global;
extern volatile int bpm_global;
extern volatile int leadOff_global;

#endif