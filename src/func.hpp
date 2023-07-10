// func.hpp

#ifndef FUNC_HPP
#define FUNC_HPP

#include <iostream>
#include <SD.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <RTClib.h>
#include <BluetoothSerial.h>
#include "config.hpp"

void readFile(fs::FS &fs, const char * path, BluetoothSerial& SerialBT){
    SerialBT.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        SerialBT.println("Failed to open file for reading");
        return;
    }

    SerialBT.println("Read from file:");
    while(file.available()){
        SerialBT.write(file.read());
    }
    file.close();
}

void listFiles(BluetoothSerial& SerialBT) {
    File root = SD.open("/");
    File file = root.openNextFile();
    
    while (file) {
        SerialBT.println(file.name());
        file = root.openNextFile();
    }
    file.close();
    root.close();
}

void deleteFile(fs::FS &fs, const char *path, BluetoothSerial& SerialBT) {
    SerialBT.printf("Deleting file: %s\n", path);
    if (fs.remove(path)) {
        SerialBT.println("File deleted");
    } else {
        SerialBT.println("Failed to delete file");
    }
}

void deleteAllFiles(fs::FS &fs, BluetoothSerial& SerialBT) {
    File root = fs.open("/");
    if (!root) {
        SerialBT.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        SerialBT.println("Not a directory");
        return;
    }
    File file = root.openNextFile();
    while (file) {
        String fileName = "/" + String(file.name());
        deleteFile(fs, fileName.c_str(), SerialBT);
        file = root.openNextFile();
    }
    root.close();
    file.close();
}

void initWifi() {
    Serial.print("Connecting to: "); 
    Serial.print(ssid);
    WiFi.begin(ssid, password);  

    int timeout = 10 * 4; // 10 seconds
    while(WiFi.status() != WL_CONNECTED  && (timeout-- > 0)) {
        delay(250);
        Serial.print(".");
    }
    Serial.println("");

    if(WiFi.status() != WL_CONNECTED) {
        Serial.println("Failed to connect");
    }

    Serial.print("WiFi connected in: "); 
    Serial.print(millis());
    Serial.print(", IP address: "); 
    Serial.println(WiFi.localIP());
}

bool isWifiConnected() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Wi-Fi not connected. Initializing Wi-Fi connection...");
        initWifi();
        return false;
    }
    return true;
}

void closeWifiConnection() {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Closing Wi-Fi connection...");
        WiFi.disconnect();
        Serial.println("Wi-Fi connection closed");
    }
}

void makeIFTTTRequest(String data) {
    if(!isWifiConnected()){
        Serial.println("Failed to estabilish WiFi connection! Could NOT send data");
        return;
        }
    String jsonObject = String("{\"value1\":\"") + data;
    jsonObject.replace(".", ",");
    
    Serial.print("Connecting to "); 
    Serial.print(server);
    
    WiFiClient client;
    int retries = 5;
    while(!!!client.connect(server, 80) && (retries-- > 0)) {
        Serial.print(".");
    }
    Serial.println();
    if(!!!client.connected()) {
        Serial.println("Failed to connect...");
    }
    
    Serial.print("Request resource: "); 
    Serial.println(resource);                        
                        
    client.println(String("POST ") + resource + " HTTP/1.1");
    client.println(String("Host: ") + server); 
    client.println("Connection: close\r\nContent-Type: application/json");
    client.print("Content-Length: ");
    client.println(jsonObject.length());
    client.println();
    client.println(jsonObject);
            
    int timeout = 5 * 10; // 5 seconds             
    while(!!!client.available() && (timeout-- > 0)){
        delay(100);
    }
    if(!!!client.available()) {
        Serial.println("No response...");
    }
    while(client.available()){
        Serial.write(client.read());
    }
    
    Serial.println("\nclosing connection");
    client.stop(); 
}

String adjustRTC(NTPClient& timeClient, RTC_DS1307& rtc){  
    isWifiConnected();
    if(WiFi.status() != WL_CONNECTED) {
        Serial.println("Failed to connect, going back to sleep");
    }
    timeClient.begin();
    timeClient.setPoolServerName(ntpServer); // Set NTP server
    timeClient.setTimeOffset(-3 * 3600);     // Set GMT offset for Brasilia, Brazil (-3 hours in seconds)
    timeClient.update(); 
    DateTime current = DateTime(timeClient.getEpochTime());
    rtc.adjust(current);
    DateTime now = rtc.now();
    timeClient.end();
    closeWifiConnection();
    return String(now.day()) + "/" + String(now.month()) + "/" + String(now.year()) + "_" + String(now.hour()) + String(now.minute()) + String(now.second());
}

void resetCounters(){
    logEntryCount = 0;

    measureSum_0 = 0;
    mean_0 = 0; 
    stdev_0 = 0;
    deviationSum_0 = 0;

    measureSum_1 = 0;   
    mean_1 = 0; 
    stdev_1 = 0;
    deviationSum_1 = 0;

    logMeanCount = 0;
}

void initializeLidarSensors(VL53L0X sensor_0, VL53L0X sensor_1)
{
    // Initialize the XSHUT pins
    pinMode(XSHUT_PIN_0, OUTPUT);
    pinMode(XSHUT_PIN_1, OUTPUT);

    // Reset the sensors by setting XSHUT pins low
    digitalWrite(XSHUT_PIN_0, LOW);
    digitalWrite(XSHUT_PIN_1, LOW);
    delay(10);

    // Bring the sensors out of reset by setting XSHUT pins high
    digitalWrite(XSHUT_PIN_0, HIGH);
    digitalWrite(XSHUT_PIN_1, HIGH);
    delay(10);

    // Keep sensor 0 awake and disable sensor 1
    digitalWrite(XSHUT_PIN_1, LOW);
    delay(10);
    if(sensor_0.init()){
    sensor_0.setAddress(ADDRESS_0);
    Serial.println("Sensor 0 initialized correctly");
    sensor_1.setMeasurementTimingBudget(200000);
    }else Serial.println("Failed to detect and initialize sensor_0!");

    // Wake up sensor 1
    digitalWrite(XSHUT_PIN_1, HIGH);
    if(sensor_1.init()){
    Serial.println("Sensor 1 initialized correctly");
    sensor_1.setMeasurementTimingBudget(200000);
    }else Serial.println("Failed to detect and initialize sensor_1!");
    delay(10);
}

void testWifi(){
    initWifi();
    Serial.println("Wifi connection tested successfully");
    closeWifiConnection();
}

void initializeSDCardAndRTC(NTPClient& timeClient, RTC_DS1307& rtc) {
  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    while(1);
  }
  else {
    Serial.println("SD Card Mounted");
  }

  if (!rtc.begin()) {
    Serial.println("RTC not found");
    while(1);
  }
  else {
    Serial.println("RTC Connected");
    adjustRTC(timeClient, rtc);
  }
}


#endif