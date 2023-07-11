#include <Arduino.h>
#include <VL53L0X.h>
#include <SD.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <RTClib.h>
#include <BluetoothSerial.h>
#include <neotimer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT_U.h>

// config NTP server for RTC WiFi calibration
const char* ntpServer = "pool.ntp.org";

// Wifi connection credentials
const char* ssid = "FUX_2G";//"CBPF_EVENTOS";
const char* password = "fux081100";//"cbpf2019";

// IFTTT information for writing data on Google Sheets
const char* resource = "/trigger/Medidas_ESP32_LIDAR/with/key/ci6ljKlFCFmexcFFISDd41";
const char* server = "maker.ifttt.com";

// LIDAR config
bool isLidaron_0 = false;
bool isLidaron_1 = false;

// Neotimer send data interval
const int defaultTimer = 900000;

// Counters
int logInterval = 0; // set default logging interval
int N = 100; // set default number of measurements for each mean
int logEntryCount = 0; // counter for each "sub"-measurement
int logMeanCount = 0; // counter for each "full" measurement (mean of n "sub"-measurements)

int measurements_0[200]; // vector to store measurements
int measureSum_0 = 0; // current sum of measurements
float stdev_0 = 0; // standard deviation of measurements
float deviationSum_0 = 0; // sum of standard deviation of measurements
float dev_0 = 0; // standard deviation of measurements
double mean_0 = 0; // mean of measurements

int measurements_1[200]; // vector to store measurements
int measureSum_1 = 0; // currenst sum of measurements
float stdev_1 = 0; // standard deviation of measurements
float deviationSum_1 = 0; // sum of standard deviation of measurements
float dev_1 = 0;
double mean_1 = 0;

float temperature = 0;
uint16_t reading_0 = 0;
uint16_t reading_1 = 0;

VL53L0X sensor_0;
VL53L0X sensor_1;
RTC_DS1307 rtc;
BluetoothSerial SerialBT;
OneWire oneWire(4);
DallasTemperature tempSensorBus(&oneWire);
DHT_Unified dht(32, DHT11); 
sensor_t sensor;
sensors_event_t event;

WiFiUDP udp;
NTPClient timeClient(udp);
Neotimer mytimer;
Neotimer logtimer;
File logFile;

void readFile(fs::FS &fs, const char * path, BluetoothSerial& SerialBT);
void listFiles(BluetoothSerial& SerialBT);
void deleteFile(fs::FS &fs, const char *path, BluetoothSerial& SerialBT);
void deleteAllFiles(fs::FS &fs, BluetoothSerial& SerialBT);
void initWifi();
bool isWifiConnected();
void closeWifiConnection();
void makeIFTTTRequest(String data);
String adjustRTC();
void resetCounters();
void initializeLidarSensors();
void testWifi();
void initializeSDCardAndRTC();
void i2cScan();


void setup() {
    Serial.begin(115200);
    SerialBT.begin("ESP32");
    Wire.begin();
    tempSensorBus.begin();
    dht.begin();
    mytimer.set(defaultTimer);
    logtimer.set(200);
    initializeLidarSensors();
    testWifi();
    initializeSDCardAndRTC();
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
            if (!logFile) {
                // Find the latest file based on file name
                mytimer.set(logInterval);
                resetCounters();
                File root = SD.open("/");
                File lastFile;
                String latestFileName = "";

                while (File file = root.openNextFile()) {
                    if (!file.isDirectory()) {
                        String fileName = file.name();
                        if (fileName > latestFileName) {
                            latestFileName = fileName;
                            lastFile = file;
                        }
                    }
                    file.close();
                    Serial.println(lastFile.name());
                }
                if (lastFile.available()) {
                    String lastFilename = "/" + latestFileName;
                    logFile = SD.open(lastFilename.c_str(), FILE_APPEND);
                    SerialBT.println("Opened the last file for appending data.");
                }
                else {
                    SerialBT.println("No files found.");
                }
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
                SerialBT.print(adjustRTC());
            }
        }
        else if (command == 'f') {
            while (!SerialBT.available());
            deleteAllFiles(SD, SerialBT);
        }
        else if (command == 's') {
            while (!SerialBT.available());
            i2cScan();
        }
    }

    if(logFile && logtimer.repeat()){
        DateTime now = rtc.now();
        if(isLidaron_0){reading_0 = sensor_0.readRangeSingleMillimeters();}
        if(isLidaron_1){reading_1 = sensor_1.readRangeSingleMillimeters();}
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
        dht.temperature().getEvent(&event);
        dht.humidity().getEvent(&event);
        logMeanCount++;
        String dataN = String(logMeanCount) + " " + String(millis() / 1000) + " " + String(now.day()) + "/" + String(now.month()) + "/" + String(now.year()) + " " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + " " + String(mean_0) + " " + String(stdev_0) + " " + String(mean_1) + " " + String(stdev_1) + " " + String(temperature) + " " + String(event.temperature) + " " + String(event.relative_humidity);
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
        Serial.println("Wi-Fi not connected.");
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
    String jsonObject = String("{\"value1\":\"") + data + "\"}";
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

String adjustRTC(){  
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
void initializeLidarSensors(){
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

void initializeSDCardAndRTC() {
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
