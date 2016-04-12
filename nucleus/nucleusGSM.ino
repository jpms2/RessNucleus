/*
Code : Nucleus connection to fiware
Author : João Pedro de Medeiros Santos (jpms2@cin.ufpe.br)
Project Leader : Paulo Henrique Monteiro Borba (phmb@cin.ufpe.br)
INES-ESCIN.
*/

#include <call.h>
#include <gps.h>
#include <GSM.h>
#include <HWSerial.h>
#include <inetGSM.h>
#include <LOG.h>
#include <SIM900.h>
#include <sms.h>
#include <Streaming.h>
#include <WideTextFinder.h>
#include <Ultrasonic.h>
//#include "SIM900.h"
#include <SoftwareSerial.h>
#include "inetGSM.h"

#define pino_trigger 8
#define pino_echo 9
#define pino_trigger2 12
#define pino_echo2 13

InetGSM inet;

Ultrasonic ultrasonic_sensor(pino_trigger, pino_echo);
Ultrasonic ultrasonic_sensor2(pino_trigger2, pino_echo2);

boolean started = false;
char smsbuffer[160];
char n[20];

byte valor;

void setup()
{
  Serial.begin(9600);
  powerUpOrDown();
  Serial.println(F("Testing GSM Shield SIM900"));
  if (gsm.begin(2400))
  {
    Serial.println(F("\nstatus=READY"));
    started = true;
  }
  else Serial.println(F("\nstatus=IDLE"));
}

void loop()
{
  if (started) {
    Serial.println(F("Initializing post request...."));
    String oilLevel = levelRead();
    if((oilLevel.toInt() <= 100) && (oilLevel.toInt() >= 0))
    {
      int contentLength = 265 + oilLevel.length();
        send_GSM(oilLevel);
        delay(10000);
      
    } 
    else
    {
      Serial.println("Wrong read");
    }
    delay(1000);
  }
}

String levelRead()
{
  Serial.println("Reading level");
  long sensorOneLevelValues [10];
  long sensorTwoLevelValues [10];
  
  for(int i = 0; i < 10; i++)
  {
      int echoReturnTime = ultrasonic_sensor.timing();
      long sensorOneDistance = ultrasonic_sensor.convert(echoReturnTime, Ultrasonic::CM);
      sensorOneLevelValues[i] = 100 - (((sensorOneDistance) * 10)/7);
      delay(100);
      
      int echoReturnTime2 = ultrasonic_sensor2.timing();
      long sensorTwoDistance = ultrasonic_sensor2.convert(echoReturnTime2, Ultrasonic::CM);
      sensorTwoLevelValues[i] = 100 - (((sensorTwoDistance) * 10)/7);
      delay(100);
  }

  int modalValue = getModalValue(sensorOneLevelValues, sensorTwoLevelValues);
  Serial.println(modalValue);
  int avgMedianValue = getAverageMedian(sensorOneLevelValues, sensorTwoLevelValues, modalValue);

  return String(avgMedianValue);
}

int getAverageMedian(long sensorOneLevelValues [], long sensorTwoLevelValues [], int modalValue)
{
  Serial.println("Calculating avg");
  int elementCounter = 0;
  int elementSum = 0;
  
  for(int i = 0; i < 10; i++)
  {
    if(((int)sensorOneLevelValues[i]/10) == modalValue)
    {
      elementSum += sensorOneLevelValues[i];
      elementCounter++;
    }
    if(((int)sensorTwoLevelValues[i]/10) == modalValue )
    {
      elementSum += sensorTwoLevelValues[i];
      elementCounter++;
    }
  }

  if(elementCounter != 0)
  {
    return (elementSum/elementCounter);
  }
  else
  {
    return 0;    
  }
}

int getModalValue(long sensorOneLevelValues [], long sensorTwoLevelValues [])
{
  Serial.println("Calculating mode");
  int sensorOneModalValues [11] = {0,0,0,0,0,0,0,0,0,0,0};
  int sensorTwoModalValues [11] = {0,0,0,0,0,0,0,0,0,0,0};
  
  for(int i = 0; i < 11; i++)
  {
    int modalIndex = (int) (sensorOneLevelValues[i]/10);
    sensorOneModalValues[modalIndex] = sensorOneModalValues[modalIndex] + 1;
    
    modalIndex = (int) (sensorTwoLevelValues[i]/10);
    sensorTwoModalValues[modalIndex] = sensorTwoModalValues[modalIndex] + 1;
  }

  int biggestModalValueOne = 0;
  int biggestModalValueTwo = 0;

  int modalValueOne = 0;
  int modalValueTwo = 0;

  for(int i = 0; i < 11; i++)
  {
    if(sensorOneModalValues[i] > biggestModalValueOne)
    {
      biggestModalValueOne = sensorOneModalValues[i];
      modalValueOne = i;
    }
    if(sensorTwoModalValues[i] > biggestModalValueTwo)
    {
      biggestModalValueTwo = sensorTwoModalValues[i];
      modalValueTwo = i;
    }
  }

  if(modalValueOne < modalValueTwo)
  {
    return modalValueOne;
  }
  else
  {
    return modalValueTwo;
  }
}

void powerUpOrDown()
{
  //Starts GSM Shield
  Serial.print(F("Starting GSM..."));
  pinMode(6, OUTPUT);
  digitalWrite(6, LOW);
  delay(1000);
  digitalWrite(6, HIGH);
  delay(1000);
  Serial.println(F("OK!"));
  digitalWrite(6, LOW);
  delay(500);
}

void send_GSM(String value)
{
  char temp_string[55];
  char msg[10];
  int numdata;
  String postBody = ("{ \"contextElements\": [ { \"type\": \"Nucleus\", \"isPattern\": \"false\", \"id\": \"NucleusAlpha\", \"attributes\": [ { \"name\": \"coordinates\", \"type\": \"String\", \"value\": \"-8.048397,-34.953650\" }, { \"name\": \"level\", \"type\": \"float\", \"value\": \"") + value;
  if (inet.attachGPRS((char*) "claro.com.br", (char*)"claro", (char*)"claro"))
    Serial.println(F("status=Connected..."));
  else Serial.println(F("status=Not connected !!"));
  delay(100);
  postBody.toCharArray(temp_string, (postBody.length() + 1));
  numdata = inet.httpPOST("130.206.119.206", 1026, "", temp_string, msg, 50);
  delay(5000);
}
