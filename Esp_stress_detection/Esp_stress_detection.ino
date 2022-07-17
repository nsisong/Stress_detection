#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Ticker.h>
#include <SPI.h>


#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME  "pulsesensor128"
#define AIO_KEY       "aio_zgos808A5YLkkgKGVhg81cPsfmff"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// The Ticker/flipper routine
Ticker flipper;

//  VARIABLES
int blinkPin = 2;                 // pin to blink led at each beat
int fadePin = 12;                 // pin to do fancy classy fading blink at each beat
int fadeRate = 0;                 // used to fade LED on with PWM on fadePin


// these variables are volatile because they are used during the interrupt service routine!
volatile int BPM;                   // used to hold the pulse rate
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // holds the time between beats, must be seeded! 
volatile boolean Pulse = false;     // true when pulse wave is high, false when it's low
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.

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

void setup(){
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


    pinMode(blinkPin,OUTPUT);         // pin that will blink to your heartbeat!
    pinMode(fadePin,OUTPUT);          // pin that will fade to your heartbeat!


}

void loop(){
  
  // MQTT_connect();
  
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("BPM: ");
  display.println(BPM);
  //  display.display();
  
  //  display.clearDisplay();
  display.setCursor(0, 20);
  display.print("IBI: ");
  display.println(IBI);
  display.display();
  
  gauge.publish(BPM);
  line_chart.publish(BPM);
  stream.publish(IBI);
  
  Serial.print(" BPM = ");
  Serial.print(BPM);
  Serial.print("  IBI = ");
  Serial.println(IBI);

  // delay(2000);
  
  
  sendDataToProcessing('S', Signal);     // send Processing the raw Pulse Sensor data
  if (QS == true){                       // Quantified Self flag is true when arduino finds a heartbeat
    fadeRate = 255;                  // Set 'fadeRate' Variable to 255 to fade LED with pulse
    sendDataToProcessing('B',BPM);   // send heart rate with a 'B' prefix
    sendDataToProcessing('Q',IBI);   // send time between beats with a 'Q' prefix
    QS = false;                      // reset the Quantified Self flag for next time    
  
  }
  
  //  Cayenne.run();
  ledFadeToBeat();
  
  delay(10);                             //  take a break
}

void ledFadeToBeat(){
    fadeRate -= 15;                         //  set LED fade value
    fadeRate = constrain(fadeRate,0,255);   //  keep LED fade value from going into negative numbers!
    analogWrite(fadePin,fadeRate);          //  fade LED
}


void sendDataToProcessing(char symbol, int data ){
    Serial.print(symbol);                // symbol prefix tells Processing what type of data is coming
    Serial.println(data);                // the data to send culminating in a carriage return
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
}
