
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <PulseSensorPlayground.h>

#define pulse_sensor A0;

#define USE_ARDUINO_INTERRUPTS true 

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME  "incubator"
#define AIO_KEY       "aio_oDxb76WO6b2rICbGh9HVbySsQ1mC"



#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/*Determine which Signal to "count as a beat" and which to ignore.
Use the "Gettting Started Project" to fine-tune Threshold Value beyond default setting.
Otherwise leave the default "550" value. 
*/
int Threshold = 550;           
                               

// Creates an instance of the PulseSensorPlayground object called "pulseSensor"                               
PulseSensorPlayground pulseSensor;  


WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Publish Temperature = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Temperature");


void MQTT_connect();


void setup_wifi() {

    WiFi.mode(WIFI_STA); 
    Serial.begin(115200);
    
    WiFiManager wm;
    //wm.resetSettings();

    bool res;
    display.setCursor(0, 16);
    // Display static text
    display.clearDisplay();
    display.println("Connect to Incubator wifi to set internet");
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

    mqtt.subscribe(&Temperature_set);

    // Configure the PulseSensor object, by assigning our variables to it. 
    pulseSensor.analogInput(pulse_sensor);

    pulseSensor.setThreshold(Threshold);   

    // Double-check the "pulseSensor" object was created and "began" seeing a signal. 
    if (pulseSensor.begin()) {
    Serial.println("We created a pulseSensor Object !");  //This prints one time at Arduino power-up,  or on Arduino reset.  
    }


}

void loop(){

    MQTT_connect();

    // Calls function on our pulseSensor object that returns BPM as an "int" into myBPM.
    int myBPM = pulseSensor.getBeatsPerMinute();  
                                               

    // Constantly test to see if "a beat happened".
    if (pulseSensor.sawStartOfBeat()) {             
        Serial.println("♥  A HeartBeat Happened ! "); 
        Serial.print("BPM: ");                        
        Serial.println(myBPM);       
        //publish to cloud                 
        Data_stream.publish(sound_val);
        //print to screen
        display.setCursor(0, 16);
        // Display static text
        display.clearDisplay();
        display.print("BPM: ");
        display.println(myBPM);
        display.display();
    }

    delay(20);
 

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