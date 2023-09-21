#include <Arduino.h>

#include <ESP8266WiFi.h> 
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>

#include <DNSServer.h>
#include <WiFiManager.h>

#define LED_CONTROL_PIN 12
#define LED_COUNT  60
#define LEDS_USED 24
#define SECONDS_IN_DAY 43200

const long utcOffsetInSeconds = -25200;

WiFiServer LEDserver(80); //Declare a server instance to allow some control over WiFi
WiFiUDP UDPInstance; //Communication protocol
NTPClient NTPClass(UDPInstance,utcOffsetInSeconds); //Network time protocol objecty to get time online
Adafruit_NeoPixel strip(LED_COUNT, LED_CONTROL_PIN, NEO_RGB + NEO_KHZ800); //The LED strip object to control the NeoPixels

int iSecond; //The next 3 variables hold time, either from the Interned or demo (Simulation)
int iMinute;
int iHour;

float iRedModifier = 1; //Colour modifiers for the NewP{ixel, allows change of the colour of the "clock face"
float iGreenModifier = 1;
float iBlueModifier = 1;

int mHour = 0;
int mMins = 0;

bool SIMULATE = 0;
bool sDebug = 1;  //send debug into to serial port if enabled

void SerialPrintTime(int,int,int);
void SimulateTime();
void LightUpPixels();
void pDebug(String);

void pDebug(String mesg){

  if (sDebug != 0 ){

    Serial.print(mesg);
    Serial.print("\n");

  }

}

void setup() {

  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();
  
  Serial.begin(115200); //Just to help with debugging once plugged into a PC

      WiFiManager wifiManager;
        
      wifiManager.autoConnect("SundialAP");

      Serial.print("Connected");
      Serial.print("\n");
      Serial.println(WiFi.localIP());


  // Initialise the objects we've declared 
  NTPClass.begin();
  LEDserver.begin();
  strip.begin();
  strip.setBrightness(255);

  NTPClass.update(); //update time
  SerialPrintTime (iSecond,iMinute,iHour);

}

void loop() {  

  NTPClass.update(); //update time
  //SerialPrintTime (iSecond,iMinute,iHour);
 

  //I use server to be able to show few features like differen colors or go throu
  //the motions showing the time. It will not react when in battery saving mode
  //as the ESP8266 will be in deep sleep for most of the time
  WiFiClient myClient = LEDserver.accept();

// if (myClient && SIMULATE == 0) { //when in demo it will ignore commands until finished
  if (myClient) { //If client has sent get request

    while (myClient.available())
    {
      if (myClient.available())
      pDebug("Client Connected...");
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
          pDebug(line);


          line.remove(11);  //remove all but the first part of the request including options 
          line.remove(0,8);  //remove the first part of the request leaving behind only the vlaue

          iRedModifier = line.toFloat();

          if (iRedModifier > 1){
            iRedModifier = 1;
          }

          if (iRedModifier < 0){
            iRedModifier = 0;
          }

    
          pDebug("Red:");
          
          pDebug(String(iRedModifier));
          
          myClient.flush();
        }
        else if (line.indexOf("GET /green") >= 0) {
          //green
          line.remove(13);
          line.remove(0,10);

          
          iGreenModifier = line.toFloat();

          if (iGreenModifier > 1){
            iGreenModifier = 1;
          }

          if (iGreenModifier < 0){
            iGreenModifier = 0;
          }
    
          
          Serial.print("green:");
          
          Serial.print(iGreenModifier);

          Serial.print('\n');
          
          myClient.flush();
          
        }
        else if (line.indexOf("GET /blue") >= 0) {
          //blue


          line.remove(12);
          line.remove(0,9);

          
          iBlueModifier = line.toFloat();

          if (iBlueModifier > 1){
            iBlueModifier = 1;
          }

          if (iBlueModifier < 0){
            iBlueModifier = 0;
          }
    
          
          Serial.print("blue:");
          Serial.print(iBlueModifier);
          Serial.print('\n');          
          myClient.flush();

        }

        else if (line.indexOf("GET /hour") >= 0) {
          //time
          
          line.remove(11);          
          line.remove(0,9);

          
          mHour = line.toInt();

              if (mHour > 12){
                mHour = 12;
              }
          
          Serial.print("Manual Hour:");
          Serial.print(mHour);
          Serial.print('\n');          
          myClient.flush();

        }

                else if (line.indexOf("GET /mins") >= 0) {
          //time
          
          line.remove(11);          
          line.remove(0,9);

          
          mMins = line.toInt();

              if (mMins > 60){
                mMins = 60;
              }
          
          Serial.print("Manual Mins:");
          Serial.print(mMins);
          Serial.print('\n');
          myClient.flush();

        }
        
        else if (line.indexOf("GET /local") >= 0) {
          //white
          iRedModifier = 1;
          iGreenModifier = 1;
          iBlueModifier = 1;
          myClient.flush();
          Serial.print("Local Time");
          SerialPrintTime (iSecond,iMinute,iHour);
          Serial.print('\n');
        }
      }
    }
            // send a response to client
            myClient.println("HTTP/1.1 200 OK");
            myClient.println("Content-type:text/html");
            myClient.println();
            myClient.print("<body><font size=""22"">  <br> Color Mix:<br>");

            myClient.print("Red:");
            myClient.print(iRedModifier);
            myClient.print("<br>");

            myClient.print("Green:");
            myClient.print(iGreenModifier);
            myClient.print("<br>");


            myClient.print("Blue:");
            myClient.print(iBlueModifier);
            myClient.print("<br><hr>");     

            myClient.print("Manual Hour:");
            myClient.print(mHour);
            myClient.print("<br>");  

            myClient.print("Manual Mins:");
            myClient.print(mMins);
            myClient.print("<br><hr>");  

            NTPClass.update();
  
            myClient.print("Local Time:");
            myClient.print(NTPClass.getFormattedTime());
            myClient.print("<br> <br> </font><footer> <hr> <font size=""8""> Usage: /red(1,0), /green(1,0), /blue(1,0), /hour(1-12), /mins(1-60), /demo </font></hr></footer></body>");     
  
            
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

  //ESP.deepSleep(60e6); deep sleep if running on battery
}

//Used for some debugging
void SerialPrintTime(int Sec, int Min, int Hour) {
  //Serial.print(NTPClass.getEpochTime());
  //Serial.print(" - ");
  Serial.print("\n");
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

// If manual time is set for calibration of the clock
  if (mHour > 0 && SIMULATE == 0) {
    iSecond = 0;
    iMinute = mMins;
    iHour = mHour;
    Serial.print("Manual Time:");
    SerialPrintTime (iSecond,iMinute,iHour);
  }

  for (int i = 0; i <= LEDS_USED ; i++) {
    strip.setPixelColor(i, 0,0,0);
  }

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

    if (sDebug != 0) { 
      SerialPrintTime(iSecond,iMinute,iHour);

      Serial.print("\n");

      Serial.print("R:");
      Serial.print(iRedModifier);
      Serial.print("  -  ");

      Serial.print("G:");
      Serial.print(iGreenModifier);

      Serial.print("  -  B:");
      Serial.print(iBlueModifier);
      Serial.print("\n");
      Serial.print("\n");
    }

  strip.show(); //show the result
}