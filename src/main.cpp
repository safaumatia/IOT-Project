#include <Arduino.h>
#include <HttpClient.h>
#include <WiFi.h>
#include <inttypes.h>
#include <stdio.h>
#include <Wire.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

// Default WiFi credentials
char ssid[50];  
char pass[50];  

const char kHostname[] = "18.219.216.245";
const int kPort = 5000;


const int kNetworkTimeout = 30 * 1000;
const int kNetworkDelay = 1000;

const int PULSE_INPUT = 33;  // GPIO for Pulse Sensor

int threshold = 2000;  // Threshold for detecting a heartbeat
int beat = 0;        
unsigned long timeBefore = 0; 
int bpm = 0;           
bool heartbeatDetected = false;


void nvs_access() {
 esp_err_t err = nvs_flash_init();
 if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
   ESP_ERROR_CHECK(nvs_flash_erase());
   err = nvs_flash_init();
 }
 ESP_ERROR_CHECK(err);


 Serial.println("Accessing NVS for WiFi credentials...");
 nvs_handle_t nvsHandle;
 err = nvs_open("storage", NVS_READWRITE, &nvsHandle);
 if (err != ESP_OK) {
   Serial.printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
   return;
 }


 size_t ssid_len = sizeof(ssid);
 size_t pass_len = sizeof(pass);
 err = nvs_get_str(nvsHandle, "ssid", ssid, &ssid_len);
 err |= nvs_get_str(nvsHandle, "pass", pass, &pass_len);


 if (err == ESP_OK) {
   Serial.printf("WiFi credentials loaded successfully: SSID=%s\n", ssid);
 } else if (err == ESP_ERR_NVS_NOT_FOUND) {
   Serial.println("WiFi credentials not found in NVS.");
 } else {
   Serial.printf("Error reading WiFi credentials (%s)\n", esp_err_to_name(err));
 }


 nvs_close(nvsHandle);
}


void setup() {
 Serial.begin(9600);
 delay(1000);


 // Retrieve WiFi credentials
 nvs_access();


 // Connect to WiFi
 Serial.print("Connecting to WiFi: ");
 Serial.println(ssid);
 WiFi.begin(ssid, pass);


 while (WiFi.status() != WL_CONNECTED) {
   delay(500);
   Serial.print(".");
 }
 Serial.println("\nWiFi connected.");
 Serial.print("IP Address: ");
 Serial.println(WiFi.localIP());


 // Initialize Pulse Detection
 Serial.println("Welcome to the Pulse Monitoring System");
 Serial.println("Monitoring pulse...");
}


void loop() {
  // Read Pulse Sensor Signal
  int sensorValue = analogRead(PULSE_INPUT);

  if (sensorValue > threshold) {
    beat++;
    Serial.println("Beat detected");
  }

  // Calculate BPM Every 15 Seconds
  if (millis() - timeBefore > 5000) {  
    bpm = beat * 12;  // Multiply by 12 for 5 seconds interval to 1 minute
    beat = 0;        
    timeBefore = millis();

    Serial.print("BPM: ");
    Serial.println(bpm);

    if (bpm != 0) {
    if (bpm >= 60 && bpm <= 100) {
      Serial.println("Status: Normal");
    } else {
      Serial.println("Status: Abnormal");
    }
    }

    // Send data to server
   WiFiClient c;
    HttpClient http(c);

    http.beginRequest();
    http.post(kHostname, kPort, "/data");
    http.sendHeader("Content-Type", "application/json");
    http.sendHeader("Connection", "close");

    String payload = String("{\"bpm\":") + bpm + "}";
    http.sendHeader("Content-Length", payload.length());
    http.print(payload);
    http.endRequest();

    int statusCode = http.responseStatusCode();
    Serial.printf("Response Code: %d\n", statusCode);

    if (statusCode == 200) {
      Serial.println("Data sent successfully.");
    } else {
      Serial.println("Failed to send data.");
    }

    http.stop();
  }
  delay(500);  // Stabilize readings
}

