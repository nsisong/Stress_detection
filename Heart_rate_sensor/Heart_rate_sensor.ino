#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME  "pulsesensor128"
#define AIO_KEY       "aio_xFfJ09No2GckFO4a7ya3UfbyuWCI"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#define ON_Board_LED 2  //--> Defining an On Board LED (GPIO2 = D4), used for indicators when the process of connecting to a wifi router and when there is a client request.



unsigned long previousMillisGetHR = 0; //--> will store the last time Millis (to get Heartbeat) was updated.
unsigned long previousMillisHR = 0; //--> will store the last time Millis (to get BPM) was updated.

const long intervalGetHR = 10; //--> Interval for reading heart rate (Heartbeat) = 10ms.
const long intervalHR = 10000; //--> Interval for obtaining the BPM value based on the sample is 10 seconds.

int PulseSensorHRVal;
const int PulseSensorHRWire = A0; //--> PulseSensor connected to ANALOG PIN 0 (A0 / ADC 0).
const int LED_D1 = D1; //--> LED to detect when the heart is beating. The LED is connected to PIN D1 (GPIO5) on the NodeMCU ESP12E.
int Threshold = 600; //--> Determine which Signal to "count as a beat" and which to ignore.

int cntHB = 0; //--> Variable for counting the number of heartbeats.
boolean ThresholdStat = true; //--> Variable for triggers in calculating heartbeats.
int BPMval = 0; //--> Variable to hold the result of heartbeats calculation.

WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Publish gauge = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Gauge");
Adafruit_MQTT_Publish line_chart = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Line_chart");
Adafruit_MQTT_Publish stream = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Stream");


void MQTT_connect();


void setup_wifi() {

    WiFi.mode(WIFI_STA); 
    Serial.begin(115200);
    
    WiFiManager wm;
    //wm.resetSettings();

    bool res;
    display.setCursor(0, 0);
    // Display static text
    display.clearDisplay();
    display.println("Connect to pulse Sensor wifi to set internet");
    display.display();
    res = wm.autoConnect("Pulse_Sensor","Pulse@12345"); // password protected ap
 

    if(!res) {
        Serial.println("Failed to connect");
        // ESP.restart();
        display.setCursor(0, 16);
        // Display static text
        display.clearDisplay();
        display.println("Failed to connect to Wifi");
        display.display(); 
    } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("connected...yeey :)");
        display.setCursor(0, 16);
        // Display static text
        display.clearDisplay();
        display.println("connected to Wifi");
        display.display();
    }
}
// To get a BPM value based on a heart rate reading for 10 seconds.
void GetHeartRate(){
  
  unsigned long currentMillisGetHR = millis();

  if (currentMillisGetHR - previousMillisGetHR >= intervalGetHR) {
    previousMillisGetHR = currentMillisGetHR;

    PulseSensorHRVal = analogRead(PulseSensorHRWire);

    if (PulseSensorHRVal > Threshold && ThresholdStat == true) {
      cntHB++;
      ThresholdStat = false;
      digitalWrite(LED_D1,HIGH);
    }

    if (PulseSensorHRVal < Threshold) {
      ThresholdStat = true;
      digitalWrite(LED_D1,LOW);
    }
  }
  

  //The process for getting the BPM value.
  unsigned long currentMillisHR = millis();

  if (currentMillisHR - previousMillisHR >= intervalHR) {
    previousMillisHR = currentMillisHR;

    BPMval = cntHB * 6; //--> The taken heart rate is for 10 seconds. So to get the BPM value, the total heart rate in 10 seconds x 6.
    Serial.print("BPM : ");
    Serial.println(BPMval);
    gauge.publish(BPMval);
    line_chart.publish(BPMval);
    stream.publish(PulseSensorHRVal);
    
    cntHB = 0;

    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("BPM: ");
    display.println(BPMval);
    //display.display();
    
    //display.clearDisplay();
    display.setCursor(0, 20);
    display.print("IBI: ");
    display.println(PulseSensorHRVal);
    display.display();
    

  }
  
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
    while(!Serial) delay(1);

      Serial.println("All state initialized");

      if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
      Serial.println(F("SSD1306 allocation failed"));
      for(;;);
    }
  //  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  setup_wifi();
  
  pinMode(ON_Board_LED,OUTPUT); //--> On Board LED port Direction output
  digitalWrite(ON_Board_LED, HIGH); //--> Turn off Led On Board

  pinMode(LED_D1,OUTPUT); //--> Set LED_3 PIN as Output.

  Serial.println();
  Serial.println("Please wait 10 seconds to get the BPM Value");
  MQTT_connect();
}
//--------------------------------------------------------------------------------

void loop() {
  

  GetHeartRate(); //--> Calling the GetHeartRate() subroutine

}
void MQTT_connect() {
    int8_t ret;

    // Stop if already connected.
    if (mqtt.connected()) {
        display.setCursor(0, 16);
        // Display static text
        display.clearDisplay();
        display.println("connected to mqtt server");
        display.display();
        return;
    }

    Serial.print("Connecting to MQTT... ");

    uint8_t retries = 3;
    while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
        Serial.println(mqtt.connectErrorString(ret));
        Serial.println("Retrying MQTT connection in 5 seconds...");
        mqtt.disconnect();
        delay(5000);  // wait 5 seconds
        retries--;
        if (retries == 0) {
            // basically die and wait for WDT to reset me
            while (1);
        }
    }
    Serial.println("MQTT Connected!");
    display.clearDisplay();
}

