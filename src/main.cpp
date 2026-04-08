#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "emg.h"
#include "ppg.h"
#include "ecg.h"

const char* ssid     = "Kamarr";
const char* password = "Kuningan123";
const char* mqtt_server = "192.168.0.102";
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

// CORE 0: WIFI & MQTT
void wifiTask(void *pv) {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) vTaskDelay(500 / portTICK_PERIOD_MS);
    
    mqtt.setServer(mqtt_server, 1883);
    mqtt.setBufferSize(1500); 

    while (true) {
        if (!mqtt.connected()) reconnectMQTT();
        mqtt.loop();

        char loc_emg[300] = {0};
        char loc_ppg[250] = {0};
        char loc_ecg[200] = {0};
        char loc_final[1200] = {0};

        emg_getPayload(loc_emg);
        ppg_getPayload(loc_ppg);
        ecg_getPayload(loc_ecg);

        snprintf(loc_final, sizeof(loc_final), "{%s,%s,%s}", loc_emg, loc_ppg, loc_ecg);

        if (mqtt.connected()) {
            mqtt.publish(mqtt_topic, loc_final);
        }

        // Kirim setiap 10ms (Pas dengan 10 sampel EMG)
        vTaskDelay(10 / portTICK_PERIOD_MS); 
    }
}

// CORE 1: SENSOR SAMPLING
void emgTask(void *pv) {
    emg_init();
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (true) {
        emg_sample(); // 1ms
        vTaskDelayUntil(&xLastWakeTime, 1 / portTICK_PERIOD_MS);
    }
}

// Task ECG dan PPG lainnya tetap sama...
void ecgTask(void *pv) {
    ecg_init();
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (true) {
        ecg_sample(); 
        vTaskDelayUntil(&xLastWakeTime, 2 / portTICK_PERIOD_MS);
    }
}

void ppgTask(void *pv) {
    ppg_init();
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (true) {
        ppg_sample(); 
        vTaskDelayUntil(&xLastWakeTime, 13 / portTICK_PERIOD_MS);
    }
}

void setup() {
    Serial.begin(115200);
    
    // Core 1 untuk Sensor (High Priority)
    xTaskCreatePinnedToCore(emgTask, "EMG", 4096, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(ecgTask, "ECG", 4096, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(ppgTask, "PPG", 4096, NULL, 2, NULL, 1);
    
    // Core 0 untuk WiFi
    xTaskCreatePinnedToCore(wifiTask, "WiFi", 10000, NULL, 1, NULL, 0);
}

void loop() { vTaskDelete(NULL); }