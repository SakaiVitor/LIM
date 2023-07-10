// config.hpp

#ifndef CONFIG_HPP
#define CONFIG_HPP

// config NTP server for RTC WiFi calibration
const char* ntpServer = "pool.ntp.org";

// Wifi connection credentials
const char* ssid = "CBPF_EVENTOS";
const char* password = "cbpf2019";

// IFTTT information for writing data on Google Sheets
const char* resource = "/trigger/Medidas_ESP32_LIDAR/with/key/ci6ljKlFCFmexcFFISDd41";
const char* server = "maker.ifttt.com";

// LIDAR config
// Define the XSHUT pin numbers
const int XSHUT_PIN_0 = 6;
const int XSHUT_PIN_1 = 15;
// Define the I2C addresses for the sensors
const uint8_t ADDRESS_0 = 0x30;
const uint8_t ADDRESS_1 = 0x31;

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
double reading_0 = 0;
double reading_1 = 0;

#endif