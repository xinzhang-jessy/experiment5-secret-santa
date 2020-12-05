/**
  SecretSanta
  Kristjan Buckingham & Xin Zhang (Jessy)
  DIGF 6037: Creation & Computation
  Experiment 5: Remote Presence
  OCAD University
  Created on December 1, 2020
  
  Based on:
  DIGF 6037 Creation & Computation
  Kate Hartman & Nick Puckett
  Experiment 5: Trading Temperatures - wifi_blinkfadetest

  Based on the MPR121test.ino example file

  Based on: 
  Creation & Computation - Digital Futures, OCAD University
  blinkMultipleLEDs_loopANDarrays, Kate Hartman / Nick Puckett
  Fade an LED using a timer
  Based on Basic Arduino Fade example, but updated to remove the delay
**/

#include <WiFiNINA.h>
#define PubNub_BASE_CLIENT WiFiClient
#include <PubNub.h>
#include <ArduinoJson.h>
#include <SparkFunLSM6DS3.h>
#include "Wire.h"
#include <Adafruit_MPR121.h>

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

Adafruit_MPR121 cap = Adafruit_MPR121();

//**Details of your local Wifi Network

//Name of your access point
char ssid[] = "HIVE";
//Wifi password
char pass[] = "zhanghan";

int status = WL_IDLE_STATUS;       // the Wifi radio's status

// pubnub keys
char pubkey[] = "pub-c-942b4aee-c1bb-4a02-9fbe-907d6e217dcc";
char subkey[] = "sub-c-d19dd86c-3448-11eb-a9dc-46d577f2b3de";

// channel and ID data
const char* myID = "Jessy"; // place your name here, this will be put into your "sender" value for an outgoing messsage
char publishChannel[] = "jessyData"; // channel to publish YOUR data
char readChannel[] = "jessyData"; // channel to read THEIR data

// JSON variables
StaticJsonDocument<200> outMessage; // The JSON from the outgoing message
StaticJsonDocument<200> inMessage; // JSON object for receiving the incoming values
//create the names of the parameters you will use in your message
String JsonParamName1 = "publisher";
String JsonParamName2 = "stocking1";
String JsonParamName3 = "stocking2";
String JsonParamName4 = "stocking3";

int kristjanStocking1 = 0;
int kristjanStocking2 = 0;
int kristjanStocking3 = 0;

int serverCheckRate = 1000; //how often to publish/read data on PN
unsigned long lastSendCheck; //time of last publish
unsigned long lastReceiveCheck; //time of last receive

unsigned long timestamp;
unsigned long lastMsgTimestamp; // time of last message timestamp

const char* inMessagePublisher;

// Keeps track of the last pins touched
uint16_t currtouched = 0;
int ledPins[] = {12, 11, 10,};
int ledPinPositions[] = {1, 3, 5};
int totalCapPins = 3;
int capPinValues[] = {0, 0, 0};
int ledTotal = 3;

int fadeRate = 3;
int maxBrightValue = 150;
int ledBrightness[] = {10, 10, 10,};

void setup()
{
  Serial.begin(9600);
  connectToPubNub();

  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
  Serial.println("MPR121 found!");
}

void loop()
{
  //run the function to check the cap interface
  checkAllCapPins(totalCapPins);

  //make a new line to separate the message
  Serial.println();
  // put a delay so it isn't overwhelming
  delay(100);
  //setMyLeds();
  if (capPinValues[0] == 1 || capPinValues[1] == 1 || capPinValues[2] == 1)
  {
    sendAfterDelay(serverCheckRate);
  }
 
  checkMessages(serverCheckRate);
  fadeLeds();
}

void fadeLeds()
{
  for (int i = 0; i < ledTotal; i++)
  {
    if (ledBrightness[i] > 0) 
    {
      ledBrightness[i]-=10;
      analogWrite(ledPins[i], ledBrightness[i]);
    }
  }
}

void connectToPubNub()
{
  // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to the network, SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    Serial.print("*");
    // wait 2 seconds for connection:
    delay(2000);
  }
  // once you are connected :
  Serial.println();
  Serial.print("You're connected to ");
  Serial.println(ssid);
  PubNub.begin(pubkey, subkey);
  Serial.println("Connected to PubNub Server");
}

void sendAfterDelay(int pollingRate)
{
  if ((millis() - lastSendCheck) >= pollingRate)
  {
    sendMessage(publishChannel);
    lastSendCheck = millis();
  }
}

void sendMessage(char channel[])
{
  Serial.print("Sending Message to ");
  Serial.print(channel);
  Serial.println(" Channel");

  char msg[100]; // variable for the JSON to be serialized into for your outgoing message

  // assemble the JSON to publish
  outMessage[JsonParamName1] = myID; // first key value is sender: publisher
  outMessage[JsonParamName2] = capPinValues[0]; // second key value is the first stocking
  outMessage[JsonParamName3] = capPinValues[1]; // third key value is the second stocking
  outMessage[JsonParamName4] = capPinValues[2]; // fourth key value is the third stocking
  outMessage["timestamp"] = millis();

  serializeJson(outMessage, msg); // serialize JSON to send - sending is the JSON object, and it is serializing it to the char msg
  Serial.println(msg);

  WiFiClient* client = PubNub.publish(channel, msg); // publish the variable char
  if (!client)
  {
    Serial.println("publishing error"); // if there is an error print it out
  }
  else
  {
    Serial.print("   ***SUCCESS");
  }
}

void checkMessages(int pollingRate)
{
  if ((millis() - lastReceiveCheck) >= pollingRate)
  {
    //check for new incoming messages
    readMessage(readChannel);

    //save the time so timer works
    lastReceiveCheck = millis();
  }
}

void readMessage(char channel[])
{
  String msg;
  auto inputClient = PubNub.history(channel,1);
  if (!inputClient)
  {
    delay(1000);
    return;
  }
  HistoryCracker getMessage(inputClient);
  while (!getMessage.finished())
  {
    getMessage.get(msg);
    //basic error check to make sure the message has content
    if (msg.length() > 0)
    {
      Serial.print("**Received Message on ");
      Serial.print(channel);
      Serial.println(" Channel");
      Serial.println(msg);
      //parse the incoming text into the JSON object

      deserializeJson(inMessage, msg); // parse the  JSON value received

      //read the values from the message and store them in local variables
      timestamp = inMessage["timestamp"];
      if (timestamp == lastMsgTimestamp){
        Serial.print("processed message, ignore for now");
        return;
      }
      inMessagePublisher = inMessage[JsonParamName1]; // this is will be "their name"
      kristjanStocking1 = inMessage[JsonParamName2];
      kristjanStocking2 = inMessage[JsonParamName3];
      kristjanStocking3 = inMessage[JsonParamName4];
      

      if (kristjanStocking1 == 1){
        ledBrightness[0] = maxBrightValue;
        analogWrite(ledPins[0], ledBrightness[0]);
      }

      if (kristjanStocking2 == 1){
        ledBrightness[1] = maxBrightValue;
        analogWrite(ledPins[1], ledBrightness[1]);
      }

      if (kristjanStocking3 == 1){
        ledBrightness[2] = maxBrightValue;
        analogWrite(ledPins[2], ledBrightness[2]);
      }
      lastMsgTimestamp = timestamp;

    }
  }
  inputClient->stop();
}

void setMyLeds()
{
  for (int i = 0; i < ledTotal; i++)
  {
    if (capPinValues[i] == 1)
    {
      analogWrite(ledPins[ledPinPositions[i]], 150);
    } else if (capPinValues[i] == 0)
    {
      analogWrite(ledPins[ledPinPositions[i]], 0);
    }
  }
}

void checkAllCapPins(int totalCapPins)
{
  // Get the currently touched pads
  currtouched = cap.touched();

  for (uint8_t i = 0; i < totalCapPins; i++)
  {
    // it if *is* touched set 1 if no set 0
    if ((currtouched & _BV(i)))
    {
      capPinValues[i] = 1;
    }
    else
    {
      capPinValues[i] = 0;
    }
    Serial.print(capPinValues[i]);

    ///adds a comma after every value but the last one
    if (i < totalCapPins - 1)
    {
      Serial.print(",");
    }
  }

}
