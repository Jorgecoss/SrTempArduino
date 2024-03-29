#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <string.h>
#include <Time.h>
#include <TimeLib.h>
#include <String.h>
#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>

// Data wire is plugged into port 8
#define ONE_WIRE_BUS D9
const int RX = 4;
const int TX = 3;



const char* ssid = "HUAWEI-B310-E360";
const char* password = "CASACOSS21";

//const char* ssid = "iPhone de coss ";
//const char* password = "123456789";

//const char* ssid = "LGHH";
//const char* password = "zebr@123";

const char* sensorId = "18111";
const char* serverName = "http://200.94.111.227:8530/API/sensor";
//const char* serverName = "http://192.168.0.4:8530/API/sensor";  //produccion
unsigned long lastTime = 0;
unsigned long timerDelay = 6000;  //30000 30 seg  Cada cuanto se va a leer el sensor 300000 5 min


OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
TinyGPSPlus gps;
SoftwareSerial ss(RX, TX);


char tempReadings[1000][5];
char hourReadings[1000][9];
char dateReadings[1000][10];
char latReadings[1000][10];
char lngReadings[1000][10];
int cantidadLecturas = 0;

void setup() {
  Serial.begin(115200);
  ss.begin(38400);
  pinMode(LED_BUILTIN, OUTPUT);
  sensors.begin();
  Serial.println("iniciando");
  WiFi.begin(ssid, password);
  Serial.print("Conectando a la red Wi-Fi...");
  delay(1000);
  setTime(12, 55, 00, 21, 4, 2022);
}

void loop() {
  time_t t = now();
  String fecha = String(year(t)) + "-" + String(month(t)) + "-" + String(day(t));
  String hora = String(hour(t)) + ":" + String(minute(t)) + ":" + String(second(t));

  while (ss.available() > 0)
    if (gps.encode(ss.read())) {
      Serial.print(F("Location: "));
      if (gps.location.isValid()) {
        dtostrf(gps.location.lat(),8,6,latReadings[cantidadLecturas]);
      dtostrf(gps.location.lng(),9,6,lngReadings[cantidadLecturas]);
      Serial.println(latReadings[cantidadLecturas]);
      Serial.println(lngReadings[cantidadLecturas]);
     

      } else {
        Serial.print(F("INVALID"));
      }
    }
  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println(F("No GPS detected: check wiring."));
    while (true)
      ;
  }


  if ((millis() - lastTime) >= timerDelay) {

    Serial.print("Requesting temperatures...");
    sensors.requestTemperatures();  // Send the command to get temperatures
    Serial.println("DONE");
    String(sensors.getTempCByIndex(0), 2).toCharArray(tempReadings[cantidadLecturas], 5);  //guarda la temperatura en un char array
    hora.toCharArray(hourReadings[cantidadLecturas], 9);                                   //guarda la hora en un char array
    fecha.toCharArray(dateReadings[cantidadLecturas], 9);
    if (gps.location.isValid()) {
      
      dtostrf(gps.location.lat(),8,6,latReadings[cantidadLecturas]);
      dtostrf(gps.location.lng(),10,6,lngReadings[cantidadLecturas]);
      Serial.println(latReadings[cantidadLecturas]);
      Serial.println(lngReadings[cantidadLecturas]);
     
    }
    //guarda la fecha en un char array
    cantidadLecturas = cantidadLecturas + 1;
    Serial.println("Temperatura guardada");
    Serial.println(tempReadings[cantidadLecturas - 1]);
    Serial.println(hourReadings[cantidadLecturas - 1]);
    Serial.println(dateReadings[cantidadLecturas - 1]);
    Serial.println("Cantidad de Lecturas");
    Serial.println(cantidadLecturas);
    lastTime = millis();
  }
  if (WiFi.status() == WL_CONNECTED) {

    digitalWrite(LED_BUILTIN, LOW);
    WiFiClient client;
    HTTPClient http;
    int lecturasAEnviar = cantidadLecturas;
    for (int i = lecturasAEnviar; i > 0; i--) {
      Serial.println("WiFi Connected");
      http.begin(client, serverName);
      http.addHeader("Content-Type", "application/json");
      DynamicJsonDocument json(200);
      Serial.println(i);
      json["sensorId"] = sensorId;
      json["Fecha"] = dateReadings[i - 1];  //
      JsonArray updates = json.createNestedArray("updates");
      JsonObject update = updates.createNestedObject();
      update["TimeStamp"] = hourReadings[i - 1];
      update["Temp"] = tempReadings[i - 1];
      update["Lat"] = latReadings[i - 1];
      update["Lng"] = lngReadings[i - 1];
      String requestBody;
      serializeJson(json, requestBody);
      Serial.println(requestBody);
      int httpResponseCode = http.POST(requestBody);
      if (httpResponseCode == 200) {
        Serial.println("Datos enviados correctamente.");
        Serial.println(tempReadings[i]);
        http.end();
        cantidadLecturas--;
      } else {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        i++;
        if (WiFi.status() != WL_CONNECTED) {
          break;
        }
      }
    }


  } else {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.print(".");
    delay(1000);
  }
}
