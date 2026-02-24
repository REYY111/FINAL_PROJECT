#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "emg.h"
#include "ppg.h"

// ================= CONFIGURATION =================
const char* ssid = "reyhannn";
const char* password = "12345678";
const char* mqtt_server = "10.170.222.19";
const char* mqtt_topic = "sensor/biosignal";

WiFiClient espClient;  
PubSubClient mqtt(espClient);

// ================= WIFI TASK (CORE 0) =================
void wifiTask(void *pv) {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        Serial.print(".");
    }
    Serial.println("\n✅ WiFi Connected");

    mqtt.setServer(mqtt_server, 1883);
    mqtt.setBufferSize(1024); // Buffer besar untuk JSON panjang

    while (true) {
        if (WiFi.status() == WL_CONNECTED) {
            if (!mqtt.connected()) {
                Serial.println("🔄 Attempting MQTT Connection...");
                // ID unik agar tidak ditendang broker
                String clientId = "ESP32_Client_" + String(random(0xffff), HEX);
                if (mqtt.connect(clientId.c_str())) {
                    Serial.println("✅ MQTT Broker Connected");
                } else {
                    vTaskDelay(2000 / portTICK_PERIOD_MS);
                    continue;
                }
            }
            
            mqtt.loop();

            // Local buffers untuk menghindari Race Condition
            char loc_emg[200] = {0};
            char loc_ppg[250] = {0};
            char loc_final[512] = {0};

            emg_getPayload(loc_emg);
            ppg_getPayload(loc_ppg);

            int len = snprintf(loc_final, sizeof(loc_final), "{%s,%s}", loc_emg, loc_ppg);

            if (len > 0 && len < 512) {
                bool result = mqtt.publish(mqtt_topic, loc_final);
                if (result) {
                    // Hanya print titik agar serial monitor tidak lag
                    Serial.print("."); 
                } else {
                    Serial.println("\n❌ MQTT Publish Fail");
                }
            }
        }
        // Jeda 50ms (20Hz) - Stabil untuk Windows Broker
        vTaskDelay(100 / portTICK_PERIOD_MS); 
    }
}

// ================= EMG TASK (CORE 1) =================
void emgTask(void *pv) {
    emg_init();
    TickType_t lastWake = xTaskGetTickCount();
    while (true) {
        emg_sample();
        vTaskDelayUntil(&lastWake, 1 / portTICK_PERIOD_MS);
    }
}

// ================= PPG TASK (CORE 1) =================
void ppgTask(void *pv) {
    ppg_init();
    TickType_t lastWake = xTaskGetTickCount();
    while (true) {
        ppg_sample(); 
        vTaskDelayUntil(&lastWake, 40 / portTICK_PERIOD_MS);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Task Sensor di Core 1 (High Priority)
    xTaskCreatePinnedToCore(emgTask, "EMG", 4096, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(ppgTask, "PPG", 4096, NULL, 2, NULL, 1);

    // Task WiFi di Core 0 (Stack besar)
    xTaskCreatePinnedToCore(wifiTask, "WIFI", 10000, NULL, 1, NULL, 0);
}

void loop() {}