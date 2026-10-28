#include "DHT.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

const char* ssid = "";
const char* password = "";

ESP8266WebServer server(80);

Servo servo;

float temperature = NAN;
float humidity = NAN;

unsigned long lastReadTime;
const unsigned short readInterval = 1000;

String webpage = R"rawliteral(

<!DOCTYPE html>
<html lang="uk">
  <head>

    <meta charset='UTF-8'>
    <title> ESP8266 </title>

    <style>

      body {
        background-color: grey;
        background-size: cover;
      }
      .temperature {
        font-size: 75px;
        color: red;
        display: flex;
        justify-content: center;
        text-shadow: 4px 4px 10px pink;
        position: relative;
        top: 25vh;
      }
      .humidity {
        font-size: 75px;
        color: blue;
        display: flex;
        justify-content: center;
        position: relative;
        top: 25vh;
        text-shadow: 4px 4px 10px aqua;
      }
      .button {
        height: 60px;
        width: 100px;
        border-radius: 10px;
        position: relative;
        top: 22vh;
        color: white;
        align-item: center;
        border: 3px solid black;
        font-size: 40px;
      }
      .button:hover {
        transform: scale(1.01);
      }
      .button:active {
        transform: scale(0.96);
      }
      div {
        display: grid;
        justify-content: center;
      }

    </style>

  </head>

  <body>

    <p class=temperature>Temperature:&nbsp;<span id=Temperature>...</span>С</p>
    <p class=humidity>Humidity:&nbsp;<span id=Humidity>...</span>%</p>

    <div>
      <button class=button id=button onclick=buttonFunc()>...</button>
    </div>

    <script defer>

      let rotate = false;

      async function buttonFunc() {
        const button = document.getElementById("button");
      
        rotate = !rotate;

        button.textContent = rotate ? "ON" : "OFF";
        button.style.backgroundColor = rotate ? "green" : "red";

        const response = await fetch(`api/servo?rotate=${rotate}`);
      }

      async function updateSensorData() {
        try {
          const response = await fetch("/api/sensor");
          if (!response.ok) {
            throw new Error("HTTP: " + response.status);
          }
          const data = await response.json();

          let temperature = document.getElementById("Temperature");
          let humidity = document.getElementById("Humidity");

          temperature.textContent = data.temperature === null ? "---" : data.temperature.toFixed(1);
          humidity.textContent = data.humidity === null ? "---" : data.humidity.toFixed(1);
        } catch (error) {
        console.error("ERROR:" + error);
        }
      } 
      updateSensorData();
      setInterval(updateSensorData, 1000);

    </script>

  </body>

</html>

)rawliteral";

#define DHTPIN D5
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

void handleRoot() {
  server.send(200, "text/html; charset=utf-8", webpage);
}

void handleSensorAPI() {
  String json = "{\"temperature\":";

  if (isnan(temperature)) {
    json += "null";
  } else {
    json += String(temperature, 1);
  }

  json += ",\"humidity\":";

  if (isnan(humidity)) {
    json += "null";
  } else {
    json += String(humidity, 1);
  }

  json += "}";

  server.send(200, "application/json; charset=utf-8", json);
}

void readSensor() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
}

void handleServoAPI() {
  bool rotate = server.arg("rotate") == "true";
  servo.write(rotate ? 60 : 90);
  server.send(200, "text/plain", "Received: " + String(rotate));
 }

void setup() {
  Serial.begin(115200);
  dht.begin();

  servo.attach(D6);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(333);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/sensor", HTTP_GET, handleSensorAPI);
  server.on("/api/servo", HTTP_GET, handleServoAPI);
  server.begin();

  Serial.println("HTTP server started");

  readSensor();
  lastReadTime = millis();

  delay(100);
}

void loop() {
  server.handleClient();
  unsigned long currentTime = millis();

  if (currentTime - lastReadTime >= readInterval) {
    lastReadTime = currentTime;
    readSensor();
  }
}
