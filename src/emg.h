#ifndef EMG_H
#define EMG_H

#include <Arduino.h>

// Deklarasi fungsi agar bisa diakses dari file lain
void emg_init();
void emg_sample();
void emg_getPayload(char* payload);

// Eksternalisasi variabel global jika ingin diakses langsung di main.cpp
extern volatile int rawADC_global;
extern volatile float clean_global;
extern volatile float envelope_global;

#endif