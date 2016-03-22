/*
Code : Nucleus connection to fiware
Author : Daniel Ferreira Maida (dfm2@cin.ufpe.br)
Project Leader : Paulo Henrique Monteiro Borba (phmb@cin.ufpe.br)
INES-ESCIN.
*/

#include <Adafruit_CC3000.h>
#include <SPI.h>
#include <Ultrasonic.h>

#define ADAFRUIT_CC3000_IRQ  3
#define ADAFRUIT_CC3000_VBAT 5
#define ADAFRUIT_CC3000_CS   10
#define pino_trigger 8
#define pino_echo 9
#define F2(progmem_ptr) (const __FlashStringHelper *)progmem_ptr
#define IDLE_TIMEOUT_MS  3000

Ultrasonic ultrasonic_sensor(pino_trigger, pino_echo);

Adafruit_CC3000 wifiModule = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, 
                                         ADAFRUIT_CC3000_VBAT,SPI_CLOCK_DIVIDER); 
Adafruit_CC3000_Client fiwareConnection;   

#define WLAN_SSID   "TesteArduino"  //WLAN SSID
#define WLAN_PASS   "12345678"      //WLAN PASSWORD
#define WLAN_SECURITY WLAN_SEC_WPA2 //WLAN SECURITY TYPE
const unsigned long dhcpTimeout     = 60L * 1000L;
uint8_t CONNECTION_ATTEMPTS = 3;
uint32_t fiware_ip = wifiModule.IP2U32(130,206,119,206); //Fiware server IP
uint32_t t = 0L;
void setup() {
  
  Serial.begin(115200);

  
  Serial.println(F("Initializing WiFi Module..."));
  
  if(!wifiModule.begin())
  {
    Serial.println(F("Fatal error, check your wiring, no shield found!"));
    while(1); // There's nothing we can do in this case :( - loop of death
  }
  connectToAccessPoint();
}

void loop() {

    Serial.println(F("Initializing post request...."));
    
    String oilLevel = levelRead();

    if(oilLevel <= 100 && oilLevel >= 0)
    {
    
      int contentLength = 265 + oilLevel.length();

      connectToFiware();
    
      Serial.println(F("Connected to fiware ..."));
   
      doFiwarePostRequest(oilLevel, contentLength);
    
      Serial.println("Sent...");

      if(fiwareConnection.connected()){
          printResponse();
      }

      Serial.println(F("Closing connection ..."));
    
      fiwareConnection.close();
    
      Serial.println(F("Connection closed ..."));
    
    }
    else
    {
      Serial.println("Wrong read");
    }
    
    delay(5000);
}

String levelRead()
{
  int cmDistance;
  long echoReturnTime;
  
  for(int i = 0; i < 10; i++)
  {
      echoReturnTime = ultrasonic_sensor.timing();
      cmDistance = ultrasonic_sensor.convert(echoReturnTime, Ultrasonic::CM);
  }
  
  int  level = 100 - (((cmDistance - 5) * 10)/8) ;
  return String(level);
  
}

void doFiwarePostRequest(String oilLevel, int contentLength)
{
          
          //headers
          Serial.println("Sent Header 1");
          fiwareConnection.fastrprint(F("POST ")); fiwareConnection.fastrprint(F("/v1/updateContext")); fiwareConnection.fastrprint(F(" HTTP/1.1\r\n")); //post header
          Serial.println("Sent Header 2");
          fiwareConnection.fastrprint(F("Host: ")); fiwareConnection.fastrprint(F("130.206.119.206:1026")); fiwareConnection.fastrprint(F("\r\n"));   //host header
          Serial.println("Sent Header 3");
          fiwareConnection.fastrprint(F("Content-Type: application/json")); fiwareConnection.fastrprint(F("\r\n")); // content-type header
          Serial.println("Sent Header 4");
          fiwareConnection.fastrprint(F("Content-Length: ")); fiwareConnection.print(contentLength); fiwareConnection.fastrprint(F("\r\n")); //content-length header
          Serial.println("Sent Header 5");
          fiwareConnection.fastrprint(F("\r\n")); // empty line for body
          //body
          Serial.println("Sent Body 1");
          fiwareConnection.fastrprint(F("{ \"contextElements\": [ { \"type\": \"Nucleus\", \"isPattern\": \"false\", \"id\": \"NucleusAlpha\", \"attributes\": [ { \"name\": \"coordinates\", \"type\": \"String\", \"value\": \"-8.055404,-34.951013\" }, { \"name\": \"level\", \"type\": \"float\", \"value\": \""));
          Serial.println("Sent Body 2");
          fiwareConnection.print(oilLevel);
          Serial.println("Sent Body 3");
          fiwareConnection.fastrprint(F("\" } ] } ], \"updateAction\": \"APPEND\" }"));
          Serial.println("Sent Body 4");
          fiwareConnection.fastrprintln("");
          Serial.println(oilLevel);
}

void printResponse()
{
    Serial.println(F("Printing Response..."));
    
    unsigned long lastRead = millis();
    while (fiwareConnection.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (fiwareConnection.available()) {
        char c = fiwareConnection.read();
        Serial.print(c);
        lastRead = millis();
      }
    }
    
}

void connectToFiware()
{
  while(!fiwareConnection.connected()){
       
       fiwareConnection = wifiModule.connectTCP(fiware_ip, 1026);
       
       if(!wifiModule.checkConnected())
       {
         Serial.println(F("Connection dropped"));
         connectToAccessPoint();
       }
    
    }
}

void connectToAccessPoint()
{
  Serial.println(F("\nDeleting old connection profiles"));
  
  while (!wifiModule.deleteProfiles()) {
    Serial.println(F("Failed to delete old profiles!"));
  }
  
  Serial.println(F("Deleted!"));

  Serial.println(F("Establishing connection to the access point"));
  
  while (!wifiModule.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY,CONNECTION_ATTEMPTS)) {
     Serial.println(F("Connection to access point failed...Trying again..."));
     delay(100);
  }
  
  Serial.println(F("Connected!"));

  Serial.print(F("Requesting address from DHCP server..."));
  for(t=millis(); !wifiModule.checkDHCP() && ((millis() - t) < dhcpTimeout); delay(1000));
  if(wifiModule.checkDHCP()) {
    Serial.println(F("OK"));
  } else {
    Serial.println(F("failed"));
    return;
  } 
}




