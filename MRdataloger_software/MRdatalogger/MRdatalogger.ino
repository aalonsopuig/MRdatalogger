/*
MRDatalogger
Datalogger for Multirotor
Designed by Alejandro Alonso
July 2015


 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Temperature & humidity
 * PCF8591 ADC with several temp sensors


Credits:

  -Web Server created 18 Dec 2009 by David A. Mellis modified 9 Apr 2012 by Tom Igoe
  -Anemometer: https://kesslerarduino.wordpress.com/tag/arduino/
  -DHT humidity/temperature sensors Written by ladyada, public domain
  -SD card usage: by Tom Igoe (SD card datalogger
  -PCF8591 ADC code by John Boxall June 2013 http://tronixstuff.com/2013/06/17/tutorial-arduino-and-pcf8591-adc-dac-ic/
*/

#include <SPI.h>
#include <Ethernet.h>
#include "DHT.h"        //Temp & Humidity sensor library
#include <SD.h>
#include "Wire.h"

//PIN DEFINITIONS
//Digital pins
#define SDSelect    4   //Pin selection of SD card
#define Pin_DHT1    9   //DHT1 humidity/temperature sensor
//Ethernet shield attached to pins 10, 11, 12, 13

//Analog pins
#define Pin_LM35_1   3  //define a pin for LM35 temp sensor1
#define Pin_LM35_2   2  //define a pin for LM35 temp sensor2
#define Pin_LM35_3   1  //define a pin for LM35 temp sensor3
#define Pin_LightS   0   //define a pin for Photo resistor


//OTHER PREPROCESOR
#define PCF8591_1 (0x90 >> 1)       // I2C bus address
#define PCF8591_2 (0x92 >> 1)       // I2C bus address
#define SREAD_CHK_TIME 4000          //Sensors. Time in milliseconds between reading values
#define DHTTYPE DHT11               // DHT 11 Temp & Humidity sensor
//#define DHTTYPE DHT22             // DHT 22  (AM2302)
//#define DHTTYPE DHT21             // DHT 21 (AM2301)
#define TempFactorLM35_12bit  0.488       //LM35 temp sensor reading must be multiplied by this to get ºC if 12 bit ADC
#define TempFactorLM35_8bit  1.95       //LM35 temp sensor reading must be multiplied by this to get ºC if 8 bit ADC
#define FirstFileNum 1000           //First number of datalog files
#define LastFileNum 9999            //Last number of datalog files. 

//INSTANCES CREATION
File dataFile;
IPAddress ip(192, 168, 1, 177);
//IPAddress ip(172, 30, 192, 108);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

// Initialize DHT sensors for normal 16mhz Arduino
DHT dht1(Pin_DHT1, DHTTYPE);
// NOTE: For working with a faster chip, like an Arduino Due or Teensy, you
// might need to increase the threshold for cycle counts considered a 1 or 0.
// You can do this by passing a 3rd parameter for this threshold.  It's a bit
// of fiddling to find the right value, but in general the faster the CPU the
// higher the value.  The default for a 16mhz AVR is a value of 6.  For an
// Arduino Due that runs at 84mhz a value of 30 works.
// Example to initialize DHT sensor for Arduino Due:
//DHT dht(DHTPIN, DHTTYPE, 30);

//CONSTANTS
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0x90, 0xA2, 0xDA, 0x0F, 0x4D, 0x9F
};

//VARIABLES DEFINITION
unsigned long SRead_time_init;  //sensor measurement timer
unsigned long SRead_time_lapse; //sensor measurement timer
byte DHT1_Humidity; //DHT humidity. Sensor1
byte DHT1_Temp; //DHT temperature. Sensor1
char LogFileName[16];  //Logs File name
byte LM35_Temp [11];  //Temperature sensors
int Filenum;

void setup() {
  //SERIAL PORT SETUP
  // Open serial communications and wait for port to open:
//  Serial.begin(9600);
//  while (!Serial) {
//    ; // wait for serial port to connect. Needed for Leonardo only
//  }

  //SD CARD SETUP
//  Serial.print("Initializing SD card...");
  // see if the card is present and can be initialized:
SD.begin(SDSelect);
//  if (!SD.begin(SDSelect)) {
//    Serial.println("SD failed");
    // don't do anything more:
//    return;
//  }
  //File name format: log_xxxx.log, where xxxx is a consecutive number, so each time the
  //arduino is reset or switched on, it creates and write to a new file.
  for (Filenum = FirstFileNum; Filenum <= LastFileNum; Filenum++) {
    //Decide wich will be the file name
    sprintf(LogFileName, "logM%04d.txt", Filenum);
    if(!SD.exists(LogFileName)) {
      Filenum=LastFileNum+1;  //To go out of FOR
    }
  }
  dataFile = SD.open(LogFileName, FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println("millis(), DHT1_Humidity, DHT1_Temp, LM35_Temp1, LM35_Temp2, LM35_Temp3, LM35_Temp4, LM35_Temp5, LM35_Temp6, LM35_Temp7, LM35_Temp8, LM35_Temp9, LM35_Temp10, LM35_Temp11, LightSensor");
    dataFile.close();
  }
//  Serial.print("SD card initialized. ");
//  Serial.print("Log file: ");
//  Serial.println(LogFileName);  
//  Serial.println("millis(), DHT1_Humidity, DHT1_Temp, LM35_Temp1, LM35_Temp2, LM35_Temp3, LM35_Temp4, LM35_Temp5, LM35_Temp6, LM35_Temp7, LM35_Temp8, LM35_Temp9, LM35_Temp10, LM35_Temp11, LightSensor");
  
  //I2C SETUP
   Wire.begin();
  
  //ETHERNET SETUP
  //start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
//  Serial.print("server is at ");
//  Serial.println(Ethernet.localIP());

  //TEMP & HUMIDITY SENSOR SETUP
  dht1.begin();

  //Other
  SRead_time_init = millis();

}


void loop() {
  
  if (millis() < SRead_time_init) SRead_time_init = millis(); // Verify overflow of millis
  else {
    SRead_time_lapse = millis() - SRead_time_init;
    if (SRead_time_lapse > SREAD_CHK_TIME) {
      SRead_time_init = millis();

      //SENSORS READINGS  
      
      //LM35 Temp Sensors
       Wire.beginTransmission(PCF8591_1); // wake up PCF8591
       Wire.write(0x04); // control byte - read ADC0 then auto-increment
       Wire.endTransmission(); // end tranmission
       Wire.requestFrom(PCF8591_1, 5);
       LM35_Temp[0]=Wire.read();
       LM35_Temp[0]=(byte) ((float)Wire.read()*TempFactorLM35_8bit);
       LM35_Temp[1]=(byte) ((float)Wire.read()*TempFactorLM35_8bit);
       LM35_Temp[2]=(byte) ((float)Wire.read()*TempFactorLM35_8bit);
       LM35_Temp[3]=(byte) ((float)Wire.read()*TempFactorLM35_8bit);
       Wire.beginTransmission(PCF8591_2); // wake up PCF8591
       Wire.write(0x04); // control byte - read ADC0 then auto-increment
       Wire.endTransmission(); // end tranmission
       Wire.requestFrom(PCF8591_2, 5);
       LM35_Temp[4]=Wire.read();
       LM35_Temp[4]=(byte) ((float)Wire.read()*TempFactorLM35_8bit);
       LM35_Temp[5]=(byte) ((float)Wire.read()*TempFactorLM35_8bit);
       LM35_Temp[6]=(byte) ((float)Wire.read()*TempFactorLM35_8bit);
       LM35_Temp[7]=(byte) ((float)Wire.read()*TempFactorLM35_8bit);

      //DHT Temp & Humidity
      // Reading temperature or humidity takes about 250 milliseconds!
      // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
      DHT1_Humidity = dht1.readHumidity();
      // Read temperature as Celsius
      DHT1_Temp = dht1.readTemperature();

      //Light sensor
      int LightSensor = analogRead(Pin_LightS);
    
      //Analog LM35 sensors
      LM35_Temp[8] = analogRead(Pin_LM35_1); //Don't delete this. Eliminate spureous data
      LM35_Temp[8] = (byte) ((float)analogRead(Pin_LM35_1)*TempFactorLM35_12bit);
      LM35_Temp[9] = (byte) ((float)analogRead(Pin_LM35_2)*TempFactorLM35_12bit);
      LM35_Temp[10] = (byte) ((float)analogRead(Pin_LM35_3)*TempFactorLM35_12bit);


      //LOG FILE DATA SAVING
      String dataString = String(millis());
      dataString += ",";
      dataString += String(DHT1_Humidity);
      dataString += ",";
      dataString += String(DHT1_Temp);
      dataString += ",";
      for (byte n=0;n<11;n++) {
        dataString += String(LM35_Temp[n]);
        dataString += ",";
      }
      dataString += String(LightSensor);
    
      // open the file. note that only one file can be open at a time,
      // so you have to close this one before opening another.
      dataFile = SD.open(LogFileName, FILE_WRITE);
    
      // if the file is available, write to it:
      if (dataFile) {
        dataFile.println(dataString);
        dataFile.close();
      }
      // if the file isn't open, pop up an error:
///      else {
//        Serial.println("error opening log file");
//      }
    // print to the serial port too:
//    Serial.println(dataString);
    }
  }

  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
//    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
//        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.println("MULTISENSE Unit<br />");
          client.println("<br />");

          
          //DHT1 Temp & Humidity sensor printing
          client.print("Humidity: ");
          client.print(DHT1_Humidity);
          client.println(" %<br />");
          client.print("Temp.  Gral: ");
          client.print(DHT1_Temp);
          client.println(" C<br />");

          //Temp sensor printing
          for (byte n=0;n<11;n++) {
            client.print("Temp ");
            client.print(n+1);
            client.print(": ");
            client.print(LM35_Temp[n]);
            client.println(" C<br />");
          }
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
//    Serial.println("client disconnected");
  }
}


