#include <ESP8266WiFi.h> 
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>

#define LED_CONTROL_PIN 12
#define LED_COUNT  60
#define LEDS_USED 24
#define SECONDS_IN_DAY 43200

const int analogInPin = A0;
const long utcOffsetInSeconds = 3600;
bool SIMULATE = 0;

WiFiServer LEDserver(80); //Declare a server instance to allow some control over WiFi
WiFiUDP UDPInstance; //Communication protocol
NTPClient NTPClass(UDPInstance,utcOffsetInSeconds); //Network time protocol objecty to get time online
Adafruit_NeoPixel strip(LED_COUNT, LED_CONTROL_PIN, NEO_GRB + NEO_KHZ800); //The LED strip object to control the NeoPixels

int iSecond; //The next 3 variables hold time, either from the Interned or demo (Simulation)
int iMinute;
int iHour;
int iRedModifier = 1; //Colour modifiers for the NewP{ixel, allows change of the colour of the "clock face"
int iGreenModifier = 1;
int iBlueModifier = 1;


void setup() {
  
  Serial.begin(115200); //Just to help with debugging once plugged into a PC
  WiFi.begin("SSID", "pass");   // Connect to WiFi


  //Enire if block below simply waits for ESP8266 to connect to WiFi before continuing with the code
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());
  }

  // Initialise the objects we've declared 
  NTPClass.begin();
  LEDserver.begin();
  strip.begin();
  strip.setBrightness(255);
}

void loop() {  

  NTPClass.update(); //update time

  //Monitor battery voltage, if the voltage drops to ~6.4V (2 x 3.2V) 
  //ESP8266 will switch off the LEDs and go into deep sleep to minimise 
  //currnet consumption. This is also a visual que that it's time to replace the bateries
  if (analogRead(A0) <= 740) {
    strip.clear();
    strip.show();
    ESP.deepSleep(0);
  }

  //I use server to be able to show few features like differen colours or go throu
  //the motions showing the time. It will not react when in battery saving mode
  //as the ESP8266 will be in deep sleep for most of the time
  WiFiClient myClient = LEDserver.available();
  if (myClient && SIMULATE == 0) { //when in demo it will ignore commands until finished

    while (myClient.available())
    {
      if (myClient.available())
      {
        String line = myClient.readStringUntil('\n');
        //this one will simulate the clock hand movement looping throuh 12 hours ratrhet than taking onlie time
        if (line.indexOf("GET /demo") >= 0) {
          SIMULATE = 1;
          iSecond = 0;
          iMinute = 0;
          iHour = 0;
          myClient.flush();
        }
        //the next else ifs change the colour of the dial (LEDs)
        else if (line.indexOf("GET /red") >= 0) {
          //redint 
          iRedModifier = 1;
          iGreenModifier = 0;
          iBlueModifier = 0;
          myClient.flush();
        }
        else if (line.indexOf("GET /green") >= 0) {
          //green
          iRedModifier = 0;
          iGreenModifier = 1;
          iBlueModifier = 0;
          myClient.flush();
        }
        else if (line.indexOf("GET /blue") >= 0) {
          //blue
          iRedModifier = 0;
          iGreenModifier = 0;
          iBlueModifier = 1;
          myClient.flush();
        }
        else if (line.indexOf("GET /white") >= 0) {
          //white
          iRedModifier = 1;
          iGreenModifier = 1;
          iBlueModifier = 1;
          myClient.flush();
        }
      }
    }
            // send a response to client
            myClient.println("HTTP/1.1 200 OK");
            myClient.println("Content-type:text/html");
            myClient.println();
            myClient.print("<br>");
            myClient.println();
            myClient.stop();
  }


  //Below block puts the time to display into appropriate variables based on online or demo mode
  if (SIMULATE == 0) {
      iSecond = NTPClass.getSeconds();
      iMinute = NTPClass.getMinutes();
      if (NTPClass.getHours() >= 12){
        iHour = NTPClass.getHours()- 12;
      }
      else {
        iHour = NTPClass.getHours();
      }
      delay(1000);
    }
   // this is where we loop throug time if in demo mode (insde SimulateTime function)
    else {
      SimulateTime();  
      SerialPrintTime (iSecond,iMinute,iHour);
      if (iSecond == 59 && iMinute == 59 && iHour == 11) {
        SIMULATE = 0;
      }
    }

  LightUpPixels();

  //the line below is uncommented in my clock to save battery, it does disable the server 
  //without it the battery goes in about 24h from fully charged 4.2V to ~3V each.
  //ESP.deepSleep(60e6);
}

//Used for some debugging
void SerialPrintTime(int Sec, int Min, int Hour) {
  Serial.print(NTPClass.getEpochTime());
  Serial.print(" - ");
  Serial.print(Hour);
  Serial.print(":");
  Serial.print(Min);
  Serial.print(":");
  Serial.println(Sec);
}

void SimulateTime() { //Time loop for demo mode
  iSecond = iSecond + 1;
  if (iSecond == 60) {
    iSecond = 0;
    iMinute = iMinute+1;
  }
  if (iMinute == 60) {
    iMinute = 0;
    iHour = iHour+1;
  }
  if (iHour == 12) { 
    iHour = 0;
  }
}

void LightUpPixels() {
  int FirstPixel;
  int BrightnesDrop;

  //this line calculates which is the 1st pixel of the 2 to light up to show time 
  FirstPixel = (iHour*3600+iMinute*60+iSecond)/(SECONDS_IN_DAY/(LEDS_USED-1)); 
  //Here we calculate % brightnes based on the. For example, if we have 12 pixels at 12am 1st pixel will be fully lit,
  //but at 12:30 1st pixel will be at 50% brightness and the 2nd one at 50% as well
  BrightnesDrop = ((iHour*3600+iMinute*60+iSecond) - FirstPixel * SECONDS_IN_DAY/(LEDS_USED-1))*255/(SECONDS_IN_DAY/(LEDS_USED-1)); 
  //Setup the pixels based on the above calculations. We also have colour modifiers, when set they can adjust channels.
  strip.setPixelColor(FirstPixel, (255-BrightnesDrop) * iGreenModifier, (255-BrightnesDrop) * iRedModifier, (255-BrightnesDrop) * iBlueModifier);
  if (FirstPixel != LEDS_USED) {
      strip.setPixelColor(FirstPixel+1, BrightnesDrop * iGreenModifier, BrightnesDrop * iRedModifier, BrightnesDrop * iBlueModifier); 
  }
  strip.show(); //show the result
}
