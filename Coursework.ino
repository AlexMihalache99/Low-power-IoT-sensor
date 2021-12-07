#include "dotDevice.h"
#include "OneWire.h"
#include "DallasTemperature.h"
#include "ArduinoJson.h"
#include <WiFi.h>
#include <BluetoothSerial.h>
#include "driver/adc.h"
#include <esp_bt.h>

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  28        /* Time ESP32 will go to sleep (in seconds) */

const char *ssid = "TALKTALK027180";
const char *password = "JG9GDCFD";  
const char* server = "ws://ec2-52-15-138-171.us-east-2.compute.amazonaws.com:1234";
const char *gid = "6hxDLB69";

OneWire oneWire(26);
DallasTemperature sensors(&oneWire);
float temp_in_c;
int readings = 0;
float average;
int sum = 0;
bool sent = false;

int timestamps[17];
int values[17];
int i;

String json_str;


dotDevice server_con(ssid, password, server);

void setup() {
  Serial.begin(115200);
  sensors.begin();
  server_con.connect();
}

void loop() {

  sensors.requestTemperatures();
  delay(100);
  temp_in_c = sensors.getTempCByIndex(0);
  values[readings] = temp_in_c;
  timestamps[readings] = millis();
  sum += temp_in_c;

  if (readings == 15) {
    average = (float)sum / readings;
    sent = true;
    DynamicJsonDocument doc(1024);
    doc["device"] = String(gid);
    doc["average"] = average;


    JsonArray valuess = doc.createNestedArray("values");
    for (i = 0; i <= readings; i++) {
      JsonObject value = valuess.createNestedObject();
      value["timestamp"] = timestamps[i];
      value["value"] = values[i];
    }

    serializeJson(doc, json_str);
    server_con.sendJSON(json_str);
    delay(100);
    setCpuFrequencyMhz(40);
    digitalWrite(13, LOW);
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    esp_deep_sleep_start();
  }
  if (sent == false) {
    readings++;
  } else {
    sent = false;
    readings = 0;
    sum = 0;
    json_str = "";
    setCpuFrequencyMhz(240);
  }
  delay(100);
}

void disableWiFi() {
  adc_power_off();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}
void disableBluetooth() {
  btStop();
  esp_bt_controller_disable();
  delay(100);
}

void enableWiFi() {
  adc_power_on();
  delay(100);

  WiFi.disconnect(false);
  WiFi.mode(WIFI_STA);

  delay(100);
  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }
}

void setModemSleep() {
  disableWiFi();
  disableBluetooth();
  setCpuFrequencyMhz(40);

}

void wakeModemSleep() {
  setCpuFrequencyMhz(240);
  enableWiFi();
}
