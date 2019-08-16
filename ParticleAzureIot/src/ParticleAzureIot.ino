#include <stdio.h>
#include <stdlib.h>

// These are external libraries for JSON parsing
#include "Arduino.h"
#include "ArduinoJson.h"

void sendAzure();
void workLeds(int level);
void myHandler(const char *event, const char *data);

// Define your Button & LED pin configuration here
int button = D2;
int RED = D3;
int BLUE = D4;
int GREEN = D5;

static int brightness = 0;
static int hour = 0;
static int levels[24];
bool arrayTime = false;

// Change this value to how often you want the Particle to check for the brightness
const int INTERVAL = 3600;  // Default is 1 hr (3600s)

// This variable is used so that your lights aren't set to 0% on the 1st day b/c there is no training data yet
bool firstDay = true;

Timer timer(INTERVAL, sendAzure);
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
        firstDay = false;

        // Send the telemtry data to the Azure integration
        Serial.printf("Sending to Particle Console: %s\n", str);
        Particle.publish("dataSetFull", str, PRIVATE);
        strcpy(str, "{\"data\":[");
      }

      char hourToString[10];
      sprintf(hourToString, "%d", hour);

      // If it's not the first day, the machine learning model can give the Particle an accurate prediction as
      // to what the brightness should be at the current time
      if(!firstDay)
        Particle.publish("getPrediction", hourToString, PRIVATE);
    }

    // There are 4 brightness modes, representing 0, 25, 50, 75, 100% brightness
    if(digitalRead(button) == HIGH){
      brightness++;
      if(brightness == 5)
        brightness = 0;

      workLeds(brightness);
      delay(200);
    }
}

// Callback function for brightness prediction
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

  // If the brightness is greater than 1 increment, it means the user has significantly changed the brightness preference
  // for the current time - so we keep the current brightness level instead of overriding it.
  // This new value will be saved for retraining after the 24 time period.
  if(abs(brightness-level) > 1){}
  else{
    brightness = level;
    workLeds(brightness);
  }
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
