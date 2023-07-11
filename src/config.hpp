// config.hpp

#ifndef CONFIG_HPP
#define CONFIG_HPP
#pragma once

#include <VL53L0X.h>
#include <SD.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <RTClib.h>
#include <BluetoothSerial.h>
#include <Arduino.h>
#include <neotimer.h>
#include <OneWire.h>
#include <DallasTemperature.h>

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

#endif