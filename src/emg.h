#ifndef EMG_H
#define EMG_H

void emg_init();
void emg_sample();     // dipanggil RTOS tiap 1ms
void emg_getPayload(char* buffer);

#endif