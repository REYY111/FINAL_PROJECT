#ifndef EMG_H
#define EMG_H

#include <Arduino.h>

#define EMG_BATCH_SIZE 20 // 20ms sampling (1ms/data) = 20 data

// Deklarasi fungsi
void emg_init();
void emg_sample();
void emg_getPayload(char* payload);

// Flags untuk sinkronisasi paralel
extern volatile bool emg_dataReady;

#endif