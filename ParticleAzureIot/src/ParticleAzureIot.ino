#include <stdio.h>
#include <stdlib.h>

#include "Arduino.h"
#include "ArduinoJson.h"

void sendAzure();
void workLeds(int level);
void myHandler(const char *event, const char *data);

int button = D0;
int RED = D1;
int BLUE = D2;
int GREEN = D3;

static int brightness = 0;
static int hour = 0;
static int levels[24];
bool arrayTime = false;

Timer timer(1000, sendAzure);
void sendAzure(){
    arrayTime = true;
}

void setup() {
    Serial.begin(9600);
    pinMode(RED, OUTPUT);
    pinMode(BLUE, OUTPUT);
    pinMode(GREEN, OUTPUT);
    pinMode(button, INPUT_PULLDOWN);
    timer.start();
    Particle.subscribe("hook-response/getPrediction", myHandler, MY_DEVICES);
}

void loop() {
    static char str[50] = "{\"data\":[";
    if(arrayTime){
      arrayTime = false;
      levels[hour] = brightness;
      //Serial.println(levels[i]);

      char temp[50];
      sprintf(temp, "%d", levels[hour]);
      strcat(str, temp);
      //Serial.printf("Level set to: %s\n", temp);

      hour++;
      if(hour != 24){
        strcat(str, ",");
      } else {
        strcat(str, "]}");
      }

      if(hour == 24){
        hour = 0;

        Serial.printf("Sending to Particle Console: %s\n", str);
        Particle.publish("dataSetFull", str, PRIVATE);
        strcpy(str, "{\"data\":[");
      }

      char hourToString[10];
      sprintf(hourToString, "%d", hour);
      Particle.publish("getPrediction", hourToString, PRIVATE);
    }

    if(digitalRead(button) == HIGH){
      brightness++;
      if(brightness == 5)
        brightness = 0;

      workLeds(brightness);
      delay(200);
    }
}

void myHandler(const char *event, const char *data) {
  char json[500] = "";
  strcpy(json, data);

  //Serial.println(data);

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(json);
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }

  JsonObject& first = root["Results"];
  JsonObject& second = first["output1"][0];
  const char* result = second["Scored Labels"];

  int level = atoi(result);

  Serial.printf("[%d] - Brightness Predict: %d\n", hour, level);
  //workLeds(brightness);
}

void workLeds(int level){
  digitalWrite(RED, LOW);
  digitalWrite(BLUE, LOW);
  digitalWrite(GREEN, LOW);

  switch(level) {
    case 0:
      break;
    case 1:
      digitalWrite(RED, HIGH);
      break;
    case 2:
      digitalWrite(BLUE, HIGH);
      break;
    case 3:
      digitalWrite(GREEN, HIGH);
      break;
    case 4:
      digitalWrite(RED, HIGH);
      digitalWrite(BLUE, HIGH);
      digitalWrite(GREEN, HIGH);
      break;
  }
}
