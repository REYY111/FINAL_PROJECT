#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "emg.h"
#include "ppg.h" // Pastikan ppg.h juga menggunakan pola BATCH_SIZE & snprintf
#include "ecg.h" // Pastikan ecg.h juga menggunakan pola BATCH_SIZE & snprintf

const char* ssid     = "reyhandhata";
const char* password = "ffffffff";
const char* mqtt_server = "172.20.10.4";
const char* mqtt_topic  = "sensor/biosignal";

WiFiClient espClient;  
PubSubClient mqtt(espClient);

void reconnectMQTT() {
    while (!mqtt.connected()) {
        Serial.println("🔄 Menghubungkan MQTT...");
        String clientId = "ESP32_Bio_" + String(random(0xffff), HEX);
        if (mqtt.connect(clientId.c_str())) {
            Serial.println("✅ Terhubung!");
        } else {
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }
}

void wifiTask(void *pv) {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) vTaskDelay(500 / portTICK_PERIOD_MS);
    
    mqtt.setServer(mqtt_server, 1883);
    mqtt.setBufferSize(2000); 

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = 20 / portTICK_PERIOD_MS; // Pengiriman Fixed 50Hz

    while (true) {
        if (!mqtt.connected()) reconnectMQTT();
        mqtt.loop();

        char loc_emg[450] = {0};
        char loc_ppg[250] = {0};
        char loc_ecg[200] = {0};
        char loc_final[1500] = {0};

        // Ambil snapshot data terbaru dari semua sensor
        emg_getPayload(loc_emg);
        ppg_getPayload(loc_ppg);
        ecg_getPayload(loc_ecg);

        snprintf(loc_final, sizeof(loc_final), "{%s,%s,%s}", loc_emg, loc_ppg, loc_ecg);

        if (mqtt.connected()) {
            mqtt.publish(mqtt_topic, loc_final);
        }

        // Reset flags (opsional dalam mode circular, tapi baik untuk tracking)
        emg_dataReady = false; 

        // Jamin interval pengiriman stabil (Mencegah Garis Diagonal)
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void emgTask(void *pv) {
    emg_init();
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (true) {
        emg_sample(); // 1000Hz
        vTaskDelayUntil(&xLastWakeTime, 1 / portTICK_PERIOD_MS);
    }
}

void ecgTask(void *pv) {
    ecg_init();
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (true) {
        ecg_sample(); // 500Hz
        vTaskDelayUntil(&xLastWakeTime, 2 / portTICK_PERIOD_MS);
    }
}

void ppgTask(void *pv) {
    ppg_init();
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (true) {
        ppg_sample(); // 50Hz (Sekarang tepat 20ms per data)
        vTaskDelayUntil(&xLastWakeTime, 20 / portTICK_PERIOD_MS);
    }
}

void setup() {
    Serial.begin(115200);
    
    // Core 1: Khusus High Speed Sampling
    xTaskCreatePinnedToCore(emgTask, "EMG", 4096, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(ecgTask, "ECG", 4096, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(ppgTask, "PPG", 4096, NULL, 2, NULL, 1);
    
    // Core 0: Khusus WiFi/Network (Agar tidak mengganggu sampling)
    xTaskCreatePinnedToCore(wifiTask, "WiFi", 10000, NULL, 1, NULL, 0);
}

void loop() { vTaskDelete(NULL); }