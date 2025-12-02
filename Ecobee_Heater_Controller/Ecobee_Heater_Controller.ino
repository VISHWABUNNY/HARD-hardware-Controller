#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include "Timer.h"

#define RELAY 16
#define TEMP_SENSOR_PIN A0  // Analog temperature sensor pin (or use digital sensor)

ESP8266WebServer server(80);

// WiFi Configuration - can be set via web interface
const char* ssid = "HeaterController";
const char* password = "heater12345";

Timer t;

// File paths
String indexHTML = "/index.html";
String logHTML = "/log.html";
String temperatureFile = "/temp.txt";
String timeFile = "/time.txt";
String logFile = "/log.txt";
String currentTimeFile = "/currenttime.txt";  // Store current time
String wifiConfigFile = "/wifi.txt";  // Store WiFi config

bool state = 0;
unsigned long startTime = 0;  // System start time in milliseconds
int currentHour = 0;
int currentMinute = 0;
int currentSecond = 0;

void setup()
{
  Serial.begin(115200);
  delay(100);

  pinMode(RELAY, OUTPUT);
  pinMode(TEMP_SENSOR_PIN, INPUT);
  
  SPIFFS.begin();
  
  // Load WiFi config if exists, otherwise use defaults
  LoadWiFiConfig();
  
  // Initialize WiFi (AP mode for offline access)
  SetupWiFi();
  
  // Load saved time or start from 00:00:00
  LoadTime();
  
  SetupServer();
  
  // Check temperature every 30 seconds
  t.every(30000, CheckTemperature, NULL);
  // Update time every second
  t.every(1000, UpdateTime, NULL);
  // Check time window every second
  t.every(1000, CheckTime, NULL);
  
  // Initial temperature check
  CheckTemperature(NULL);
  
  PrintLog("System started - Offline Mode");
}

void loop()
{
  t.update();
  server.handleClient();
  digitalWrite(RELAY, state);
}

void SetupServer()
{
  server.on("/", handle_OnConnect);
  server.on("/temp", HTTP_POST, handle_temperature);
  server.on("/time", HTTP_POST, handle_time);
  server.on("/settime", HTTP_POST, handle_settime);
  server.on("/check", HTTP_POST, handle_check);
  server.on("/clearLog", HTTP_POST, handle_clearLog);
  server.on("/log", handle_log);
  server.on("/wifi", HTTP_POST, handle_wifi);
  server.onNotFound(handle_NotFound);
  server.begin();
}

void SetupWiFi()
{
  // Try to connect to saved WiFi first
  String savedSSID = ReadFile(wifiConfigFile);
  if (savedSSID.length() > 0 && savedSSID.indexOf('\n') > 0) {
    int newlinePos = savedSSID.indexOf('\n');
    String ssidPart = savedSSID.substring(0, newlinePos);
    String passPart = savedSSID.substring(newlinePos + 1);
    ssidPart.trim();
    passPart.trim();
    
    if (ssidPart.length() > 0) {
      Serial.print("Connecting to WiFi: ");
      Serial.println(ssidPart);
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssidPart.c_str(), passPart.c_str());
      
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        PrintLog("Connected to WiFi: " + ssidPart);
        return;
      }
    }
  }
  
  // If connection failed, start in AP mode
  Serial.println("Starting Access Point mode");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  PrintLog("Access Point started: " + String(ssid));
}

void LoadWiFiConfig()
{
  // WiFi config loaded in SetupWiFi
}

void SetState(int val, String message = "")
{
  if (state == val)
    return;
  state = val;
  PrintLog(message + String("Changing State to ") + GetStateString());
}

String GetStateString()
{
  if (state)
    return "ON";
  else
    return "OFF";
}

void UpdateTime(void* context)
{
  unsigned long currentMillis = millis();
  unsigned long elapsedSeconds = (currentMillis - startTime) / 1000;
  
  int totalSeconds = (currentHour * 3600) + (currentMinute * 60) + currentSecond + elapsedSeconds;
  
  currentHour = (totalSeconds / 3600) % 24;
  currentMinute = (totalSeconds / 60) % 60;
  currentSecond = totalSeconds % 60;
  
  // Save time every minute
  if (currentSecond == 0) {
    SaveTime();
  }
}

void LoadTime()
{
  String timeStr = ReadFile(currentTimeFile);
  if (timeStr.length() > 0) {
    int colon1 = timeStr.indexOf(':');
    int colon2 = timeStr.indexOf(':', colon1 + 1);
    if (colon1 > 0 && colon2 > 0) {
      currentHour = timeStr.substring(0, colon1).toInt();
      currentMinute = timeStr.substring(colon1 + 1, colon2).toInt();
      currentSecond = timeStr.substring(colon2 + 1).toInt();
    }
  }
  startTime = millis();
}

void SaveTime()
{
  String timeStr = String(currentHour) + ":" + 
                   String(currentMinute) + ":" + 
                   String(currentSecond);
  OverwriteFile(currentTimeFile, timeStr);
}

void CheckTime(void* context)
{
  if (!WithinTime())
    SetState(0, "Outside Timer Range. ");
}

float ReadTemperature()
{
  // Read analog temperature sensor (LM35 or similar)
  // LM35: 10mV per degree C, 0V = 0°C
  // For ESP8266 ADC: 0-1V range, 1024 steps
  // Formula: (reading / 1024.0) * 100.0 = temperature in Celsius
  
  int sensorValue = analogRead(TEMP_SENSOR_PIN);
  float voltage = (sensorValue / 1024.0) * 3.3;  // ESP8266 ADC reference is 3.3V
  float temperatureC = voltage * 100.0;  // LM35: 10mV/°C
  
  // Convert to Fahrenheit
  float temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;
  
  return temperatureF;
}

void CheckTemperature(void* context)
{
  if (!WithinTime())
  {
    return;
  }
  
  float temp = ReadTemperature();
  
  if (state == 0)
  {
    if (BelowTemperature(temp))
      SetState(1, String("Temperature is ") + String(temp, 1) + "°F. ");
  }
  else
  {
    if (AboveTemperature(temp))
      SetState(0, String("Temperature is ") + String(temp, 1) + "°F. ");
  }
}

bool WithinTemperature(float temp)
{
  File f = SPIFFS.open(temperatureFile, "r");
  if (!f)
  {
    PrintLog(String("Failed to Read File: ") + temperatureFile);
    return false;
  }
  String tempMinString = f.readStringUntil('\n');
  String tempMaxString = f.readString();
  f.close();
  float tempMin = tempMinString.toFloat();
  float tempMax = tempMaxString.toFloat();
  if (temp > tempMin && temp < tempMax)
    return true;
  else
    return false;
}

bool AboveTemperature(float temp)
{
  File f = SPIFFS.open(temperatureFile, "r");
  if (!f)
  {
    PrintLog(String("Failed to Read File: ") + temperatureFile);
    return false;
  }
  f.readStringUntil('\n');
  String tempMaxString = f.readString();
  f.close();
  float tempMax = tempMaxString.toFloat();
  if (temp > tempMax)
    return true;
  else
    return false;
}

bool BelowTemperature(float temp)
{
  File f = SPIFFS.open(temperatureFile, "r");
  if (!f)
  {
    PrintLog(String("Failed to Read File: ") + temperatureFile);
    return false;
  }
  String tempMinString = f.readStringUntil('\n');
  f.close();
  float tempMin = tempMinString.toFloat();
  if (temp < tempMin)
    return true;
  else
    return false;
}

bool WithinTime()
{
  float currentTime = (float)currentHour + ((float)currentMinute / 60.0f);
  File f = SPIFFS.open(timeFile, "r");
  if (!f)
  {
    PrintLog(String("Failed to Read File: ") + timeFile);
    return false;
  }
  String timeOnString = f.readStringUntil('\n');
  String timeOffString = f.readString();
  f.close();
  int colonInd = timeOnString.indexOf(':');
  float timeOnHours = timeOnString.substring(0, colonInd).toFloat();
  float timeOnMinutes = timeOnString.substring(colonInd + 1).toFloat();
  colonInd = timeOffString.indexOf(':');
  float timeOffHours = timeOffString.substring(0, colonInd).toFloat();
  float timeOffMinutes = timeOffString.substring(colonInd + 1).toFloat();
  float timeOn = timeOnHours + (timeOnMinutes / 60.0f);
  float timeOff = timeOffHours + (timeOffMinutes / 60.0f);
  if (timeOff < timeOn)
  {
    if (currentTime > timeOff && currentTime < timeOn)
      return false;
    else
      return true;
  }
  else
  {
    if (currentTime > timeOn && currentTime < timeOff)
      return true;
    else
      return false;
  }
}

String ReadFile(String filename)
{
  File f = SPIFFS.open(filename, "r");
  if (!f)
  {
    PrintLog(String("Failed to Read File: ") + filename);
    return "";
  }
  String contents = f.readString();
  f.close();
  return contents;
}

void OverwriteFile(String filename, String contents)
{
  File f = SPIFFS.open(filename, "w");
  if (!f)
  {
    PrintLog(String("Failed to Overwrite File: ") + filename);
    return;
  }
  f.print(contents);
  f.close();
}

void AppendFile(String filename, String contents)
{
  File f = SPIFFS.open(filename, "a");
  if (!f)
  {
    PrintLog(String("Failed to Append File: ") + filename);
    return;
  }
  f.print(contents);
  f.close();
}

void handle_NotFound()
{
  server.send(404, "text/plain", "Not found");
}

void handle_OnConnect()
{
  Serial.println("CONNECTED!");
  String html = ReadFile(indexHTML);
  html.replace("%STATE", GetStateString());
  
  // Get current temperature
  float currentTemp = ReadTemperature();
  html.replace("%CURRENTTEMP", String(currentTemp, 1));
  
  // Get current time
  String currentTimeStr = String(currentHour) + ":" + 
                         (currentMinute < 10 ? "0" : "") + String(currentMinute);
  html.replace("%CURRENTTIME", currentTimeStr);
  html.replace("%CURRENTHOUR%", String(currentHour));
  html.replace("%CURRENTMINUTE%", String(currentMinute));
  
  File f = SPIFFS.open(temperatureFile, "r");
  if (!f)
  {
    PrintLog(String("Failed to Read File: ") + temperatureFile);
  }
  String tempMinString = f.readStringUntil('\n');
  String tempMaxString = f.readString();
  tempMinString.trim();
  tempMaxString.trim();
  f.close();
  html.replace("%TEMPMIN", tempMinString);
  html.replace("%TEMPMAX", tempMaxString);
  
  f = SPIFFS.open(timeFile, "r");
  if (!f)
  {
    PrintLog(String("Failed to Read File: ") + timeFile);
  }
  String timeOnString = f.readStringUntil('\n');
  String timeOffString = f.readString();
  timeOnString.trim();
  timeOffString.trim();
  f.close();
  html.replace("%TIMEON", timeOnString);
  html.replace("%TIMEOFF", timeOffString);
  
  server.send(200, "text/html", html);
}

void handle_log()
{
  Serial.println("CONNECTED TO LOG!");
  String html = ReadFile(logHTML);
  String log = ReadFile(logFile);
  html.replace("%CONTENT", log);
  server.send(200, "text/html", html);
}

void handle_temperature()
{
  if (server.hasArg("tempMin") && server.hasArg("tempMax"))
  {
    String tempMin = server.arg("tempMin");
    String tempMax = server.arg("tempMax");
    PrintLog(String("Updating Temperature Range to: (") + tempMin + ", " + tempMax + ").");
    OverwriteFile(temperatureFile, tempMin + "\n");
    AppendFile(temperatureFile, tempMax);
  }
  else
    PrintLog(String("Temperature Request did not contain required parameters"));

  RefreshPage();
}

void handle_time()
{
  if (server.hasArg("timeOff") && server.hasArg("timeOn"))
  {
    String timeOff = server.arg("timeOff");
    String timeOn = server.arg("timeOn");
    PrintLog(String("Updating Time Range to: (") + timeOn + ", " + timeOff + ").");
    OverwriteFile(timeFile, timeOn + "\n");
    AppendFile(timeFile, timeOff);
  }
  else
    PrintLog(String("Time Request did not contain required parameters"));
  RefreshPage();
}

void handle_settime()
{
  if (server.hasArg("hour") && server.hasArg("minute"))
  {
    currentHour = server.arg("hour").toInt();
    currentMinute = server.arg("minute").toInt();
    currentSecond = 0;
    startTime = millis();
    SaveTime();
    PrintLog(String("Time set to: ") + String(currentHour) + ":" + String(currentMinute));
  }
  RefreshPage();
}

void handle_wifi()
{
  if (server.hasArg("ssid") && server.hasArg("password"))
  {
    String ssidNew = server.arg("ssid");
    String passNew = server.arg("password");
    OverwriteFile(wifiConfigFile, ssidNew + "\n" + passNew);
    PrintLog("WiFi config updated. Restart to apply.");
    server.send(200, "text/html", "<html><body>WiFi config saved. Please restart the device.</body></html>");
    delay(1000);
    ESP.restart();
  }
  RefreshPage();
}

void handle_check()
{
  RefreshPage();
  CheckTemperature(NULL);
}

void handle_clearLog()
{
  ClearLog();
  RefreshPage();
}

void RefreshPage()
{
  server.sendHeader("Location", "/");
  server.send(303);
}

void PrintLog(String message)
{
  String timeStr = String(currentHour) + ":" + 
                   (currentMinute < 10 ? "0" : "") + String(currentMinute) + ":" +
                   (currentSecond < 10 ? "0" : "") + String(currentSecond);
  message = String("(") + timeStr + ") " + message;
  Serial.println(message);
  File f = SPIFFS.open(logFile, "r");
  int lineCount = 0;
  while (f.available())
  {
    lineCount++;
    f.readStringUntil('\n');
  }
  f.close();
  if (lineCount > 100)
  {
    f = SPIFFS.open(logFile, "r");
    f.readStringUntil('\n');
    String s = f.readString();
    f.close();
    OverwriteFile(logFile, s);
  }
  f = SPIFFS.open(logFile, "a");
  f.println(message);
  f.close();
}

void ClearLog()
{
  OverwriteFile(logFile, "");
  PrintLog("Log Cleared Manually");
}
