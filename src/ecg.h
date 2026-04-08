#ifndef ECG_H
#define ECG_H

void ecg_init();
void ecg_sample();
void ecg_getPayload(char* buffer);

#endif