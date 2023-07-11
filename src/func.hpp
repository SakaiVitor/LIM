// func.hpp

#ifndef FUNC_HPP
#define FUNC_HPP
#pragma once

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
    initWifi();
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
        Serial.println("Failed to connect");
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
/*void initializeLidarSensors(VL53L0X sensor_0, VL53L0X sensor_1){
    pinMode(XSHUT_PIN_0, OUTPUT);
    pinMode(XSHUT_PIN_1, OUTPUT);

    // all reset
    digitalWrite(XSHUT_PIN_0, LOW);    
    digitalWrite(XSHUT_PIN_1, LOW);
    delay(10);

    // all unreset
    digitalWrite(XSHUT_PIN_0, HIGH);
    digitalWrite(XSHUT_PIN_1, HIGH);
    delay(10);

    // activating LOX0 and resetting LOX1
    digitalWrite(XSHUT_PIN_1, LOW);

    // initing LOX1
    if(!sensor_0.init()) {
        Serial.println(F("Failed to boot VL53L0X_0"));
     }else {
        sensor_0.setAddress(ADDRESS_0);
        Serial.println(F("VL53L0X_0 booted successfully"));
        sensor_0.setMeasurementTimingBudget(200000);
        isLidaron_0 = true;
    }
    delay(10);

    // activating LOX1
    digitalWrite(XSHUT_PIN_1, HIGH);
    delay(10);

    //initing LOX1
    if(!sensor_1.init()) {
        Serial.println(F("Failed to boot VL53L0X_1"));
    }else {
        Serial.println(F("VL53L0X_1 booted successfully"));
        sensor_1.setMeasurementTimingBudget(200000);
        isLidaron_1 = true;
    }
}*/
void initializeLidarSensors(VL53L0X sensor_0, VL53L0X sensor_1){
    pinMode(17, OUTPUT);
    pinMode(16, OUTPUT);

    digitalWrite(16, LOW); 
    digitalWrite(17, LOW);
    
    digitalWrite(17, HIGH);
    digitalWrite(16, HIGH);
    digitalWrite(17, LOW); 
    
    delay(10); 
    sensor_1.setAddress(0x30); 
    digitalWrite(17, HIGH); 

    if (!sensor_0.init())
    {
        Serial.println("Failed to detect and initialize sensor_0!");
    }else isLidaron_0 = true;
        
    if (!sensor_1.init())
    {
        Serial.println("Failed to detect and initialize sensor_1!");
    }else isLidaron_1 = true;

    if(isLidaron_0) sensor_0.setMeasurementTimingBudget(200000); sensor_0.setTimeout(500);
    if(isLidaron_1) sensor_1.setMeasurementTimingBudget(200000); sensor_1.setTimeout(500);
}

void testWifi(){
    Serial.println("Initializing Wifi test");
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
    }
}

void i2cScan(){
    
    byte error, address;
    int deviceCount = 0;

    Serial.println("Scanning I2C bus...");

    for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
        Serial.print("Device found at address 0x");
        if (address < 16)
            Serial.print("0");
        Serial.print(address, HEX);
        Serial.println();

        deviceCount++;
        }
    }

    if (deviceCount == 0)
        Serial.println("No devices found.");
}


#endif