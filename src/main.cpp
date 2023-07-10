#include <Arduino.h>
#include <VL53L0X.h>
#include <neotimer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.hpp"
#include "func.hpp"

VL53L0X sensor_0;
VL53L0X sensor_1;
RTC_DS1307 rtc;
BluetoothSerial SerialBT;
OneWire oneWire(4);
DallasTemperature tempSensorBus(&oneWire);

WiFiUDP udp;
NTPClient timeClient(udp);
Neotimer mytimer;
Neotimer logtimer;
File logFile;

void setup() {
    Serial.begin(115200);
    SerialBT.begin("ESP32");
    Wire.begin();
    tempSensorBus.begin();
    mytimer.set(defaultTimer);
    logtimer.set(200);
    initializeLidarSensors(sensor_0, sensor_1);
    testWifi();
    initializeSDCardAndRTC(timeClient, rtc);
}

void loop() {
    // check if there is any data available on the Bluetooth serial port
    if (SerialBT.available()) {
        char command = SerialBT.read();

        if (command == 'c') {
            if (!logFile) {
                mytimer.set(logInterval);
                resetCounters();

                DateTime now = rtc.now();
                String fileName = "/" + String(now.year()) + String(now.month()) + String(now.day()) + "_" + String(now.hour()) + String(now.minute()) + String(now.second()) + ".txt";
                SerialBT.println(fileName);
                logFile = SD.open(fileName.c_str(), FILE_WRITE);
                SerialBT.println("Logging started.");
            }
            else {
                SerialBT.println("Error opening file for logging.");
            }
        }
        else if (command == 'p') {
            resetCounters();

            if (logFile) {
                logFile.close();
                SerialBT.println("Logging stopped.");
            }
            else {
                SerialBT.println("No file currently open for logging.");
            }
        }
        else if (command == 'r') {
            // Find the last file in chronological order
            mytimer.set(logInterval);
            resetCounters();
            File root = SD.open("/");
            File lastFile;
            uint32_t latestTimestamp = 0;

            while (File file = root.openNextFile()) {
                if (!file.isDirectory()) {
                    uint32_t timestamp = file.getLastWrite();
                    if (timestamp > latestTimestamp) {
                        latestTimestamp = timestamp;
                        lastFile = file;
                    }
                }
                file.close();
            }

            if (lastFile) {
                logFile = SD.open(lastFile.name(), FILE_APPEND);
                SerialBT.println("Opened the last file for appending data.");
            }
            else {
                SerialBT.println("No files found.");
            }
        }
        else if (command == 'i') {
            logInterval = (SerialBT.parseInt() * 1000);
            mytimer.set(logInterval);
            SerialBT.print("Logging interval set to ");
            SerialBT.print((logInterval / 1000));
            SerialBT.println(" seconds.");
        }
        else if (command == 'l') {
            SerialBT.println("Enter file name to read:");
            while (!SerialBT.available());
            String fileName = SerialBT.readString();
            fileName.trim();
            readFile(SD, fileName.c_str(), SerialBT);
        }
        else if (command == 'd') {
            SerialBT.println("Enter file name to delete:");
            while (!SerialBT.available());
            String fileName = SerialBT.readString();
            fileName.trim();
            deleteFile(SD, fileName.c_str(), SerialBT);
        }
        else if (command == 'n') {
            N = SerialBT.parseInt();
            SerialBT.print("Mean sample size set to ");
            SerialBT.print(N);
            SerialBT.println(" measurements.");
        }
        else if (command == 'a') {
            while (!SerialBT.available());
            listFiles(SerialBT);
        }
        else if (command == 'h') {
            while (!SerialBT.available());
            if (rtc.isrunning()) {
                SerialBT.println("RTC adjusted!");
                SerialBT.print("Current time: ");
                SerialBT.print(adjustRTC(timeClient, rtc));
            }
        }
        else if (command == 'f') {
            while (!SerialBT.available());
            deleteAllFiles(SD, SerialBT);
        }
    }

    if(logFile && logtimer.repeat()){

    DateTime now = rtc.now();
    reading_0 = sensor_0.readRangeSingleMillimeters();
    reading_1 = sensor_1.readRangeSingleMillimeters();

    // calculate mean and stdev of N measurements
    if(logEntryCount < N+1){
      logEntryCount++; // updates the log index
      if(logEntryCount > 1){

        // start from logEntryCount-1 because the first measurement is aways zero
        measurements_0[logEntryCount-2] = reading_0;
        measurements_1[logEntryCount-2] = reading_1;
        measureSum_0 += reading_0;
        measureSum_1 += reading_1;
        
        mean_0 = (float) measureSum_0 / (logEntryCount-1);
        mean_1 = (float) measureSum_1 / (logEntryCount-1);
         
        deviationSum_0 = 0;
        deviationSum_1 = 0;
        
        for (int i = 0; i < (logEntryCount-1); i++) {
          dev_0 = (float) measurements_0[i] - mean_0;
          deviationSum_0 += dev_0 * dev_0;
          dev_1 = (float) measurements_1[i] - mean_1;
          deviationSum_1 += dev_1 * dev_1;
        }
        stdev_0 = sqrt(deviationSum_0 / (logEntryCount-2));
        stdev_1 = sqrt(deviationSum_1 / (logEntryCount-2));
  
        String data1 = String(logEntryCount-1) + " " + String(reading_0) + " " + String(mean_0) + " " + String(stdev_0) + " " + String(reading_1) + " " + String(mean_1) + " " + String(stdev_1);
        String dataString = String(logEntryCount-1) + " " + String(millis() / 1000) + " " + String(now.day()) + "/" + String(now.month()) + "/" + String(now.year()) + " " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + " " + String(reading_0) + " " + String(reading_1);
        Serial.println(data1);
        //logFile.println(dataString);
        //logFile.flush();
    
        //SerialBT.println(dataString);
        //SerialBT.println(data1);
      }
    } else if(mytimer.repeat()){
    tempSensorBus.requestTemperatures(); 
    temperature = tempSensorBus.getTempCByIndex(0);
    logMeanCount++;
    String dataN = String(logMeanCount) + " " + String(millis() / 1000) + " " + String(now.day()) + "/" + String(now.month()) + "/" + String(now.year()) + " " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + " " + String(mean_0) + " " + String(stdev_0) + " " + String(mean_1) + " " + String(stdev_1) + " " + String(temperature);
    SerialBT.println(dataN);
    dataN.replace(".", ",");
    logFile.println(dataN);
    logFile.flush();
    dataN.replace(" ", ";");
    makeIFTTTRequest(dataN); 
    logEntryCount = 0;
    
    measureSum_0 = 0;
    mean_0 = 0; 
    stdev_0 = 0;
    deviationSum_0 = 0;

    measureSum_1 = 0;
    mean_1 = 0; 
    stdev_1 = 0;
    deviationSum_1 = 0;
    }
  }
}
