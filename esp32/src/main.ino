#ifdef ESP32
  #include <WiFiClientSecure.h>
  #include <HTTPClient.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESP8266HTTPClient.h>
#endif

#include <SparkFunTSL2561.h>
#include <Wire.h>
#include <OneWire.h>
#include <HTTPClient.h>
 
// DS18S20 Temperature chip i/o
OneWire ds(19);  // on pin 19 

// Create an SFE_TSL2561 object, here called "light":
 
SFE_TSL2561 light;
 
// Global variables:
 
boolean gain; // Gain setting, 0 = X1, 1 = X16;
unsigned int ms; // Integration ("shutter") time in milliseconds


//WiFi設定
const char* ssid     = "";
const char* password = "";

const char *server = "hooks.slack.com";

HTTPClient http; 

void setup(void) {
  // initialize inputs/outputs
  // start serial port
  Serial.begin(115200);
  Serial.println("Temp & TSL2561 example sketch");
  // Initialize the SFE_TSL2561 library
 
light.begin();
 
// Get factory ID from sensor:
// (Just for fun, you don't need to do this to operate the sensor)
 
unsigned char ID;
 
if (light.getID(ID))
{
Serial.print("Got factory ID: 0X");
Serial.print(ID,HEX);
Serial.println(", should be 0X5X");
}
// Most library commands will return true if communications was successful,
// and false if there was a problem. You can ignore this returned value,
// or check whether a command worked correctly and retrieve an error code:
else
{
byte error = light.getError();
printError(error);
}
 
// The light sensor has a default integration time of 402ms,
// and a default gain of low (1X).
 
// If you would like to change either of these, you can
// do so using the setTiming() command.
 
// If gain = false (0), device is set to low gain (1X)
// If gain = high (1), device is set to high gain (16X)
 
gain = 0;
 
// If time = 0, integration will be 13.7ms
// If time = 1, integration will be 101ms
// If time = 2, integration will be 402ms
// If time = 3, use manual start / stop to perform your own integration
 
unsigned char time = 2;
 
// setTiming() will set the third parameter (ms) to the
// requested integration time in ms (this will be useful later):
 
Serial.println("Set timing...");
light.setTiming(gain,time,ms);
 
// To start taking measurements, power up the sensor:
 
Serial.println("Powerup...");
light.setPowerUp();
 
// The sensor will now gather light during the integration time.
// After the specified time, you can retrieve the result from the sensor.
// Once a measurement occurs, another integration period will start.

 //Wifi 接続
    WiFi.begin(ssid, password);
    Serial.print("WiFi connecting");

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }
    Serial.println(" connected");


}

 
void loop(void) {
  // ms = 1000;
  // light.manualStart();

  //delay(ms);
  delay(3000);

  // light.manualStop();
 
// Once integration is complete, we'll retrieve the data.
 
// There are two light sensors on the device, one for visible light
// and one for infrared. Both sensors are needed for lux calculations.
 
// Retrieve the data from the device:
 
unsigned int data0, data1;
String lux_string; 
if (light.getData(data0,data1)){
// getData() returned true, communication was successful
 
Serial.print("data0: ");
Serial.print(data0);
Serial.print(" data1: ");
Serial.print(data1);

// To calculate lux, pass all your settings and readings
// to the getLux() function.

double lux; // Resulting lux value
boolean good; // True if neither sensor is saturated
 
// Perform lux calculation:

good = light.getLux(gain,ms,data0,data1,lux);

// Print out the results:

Serial.print(" lux: ");
Serial.print(lux);
lux_string  = String(lux);
if (good) Serial.println(" (good)"); else Serial.println(" (BAD)");
}else{
// getData() returned false because of an I2C error, inform the user.
byte error = light.getError();
printError(error);
}
//  String lux_string  = String(lux);
delay(3000);
  float temperature = getTemperature();
 String tmp_string  = String(temperature);
  delay(3000);
  Serial.print(temperature);
  Serial.println();
  //slack();
String s = "toshi";
Serial.print("posting data...");
http.begin("https://hooks.slack.com/services/");
http.addHeader("Content-Type", "application/json");
int httpCode = http.POST("{'username':'IoT-DATA', 'text':':door::bell: lux: " + lux_string + " temperature: " + tmp_string  + " '}");

if(httpCode > 0) {
Serial.printf("[HTTP] POST... code: %d\n", httpCode);
} else {
Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
}
//http.end();
Serial.println ("Slack done");
}

void slack(){
Serial.print("posting data...");
http.begin( "https://hooks.slack.com/services/");
 http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST("{'username':'IoT-DATA', 'text':':door::bell: #{lux}'}");

 if(httpCode > 0) {
    Serial.printf("[HTTP] POST... code: %d\n", httpCode);
  } else {
    Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  //http.end();
  Serial.println ("Slack done");
}

void printError(byte error)
// If there's an I2C error, this function will
// print out an explanation.
{
Serial.print("I2C error: ");
Serial.print(error,DEC);
Serial.print(", ");
 
switch(error)
{
case 0:
Serial.println("success");
break;
case 1:
Serial.println("data too long for transmit buffer");
break;
case 2:
Serial.println("received NACK on address (disconnected?)");
break;
case 3:
Serial.println("received NACK on data");
break;
case 4:
Serial.println("other error");
break;
default:
Serial.println("unknown error");
}
}
 
void updateData(byte* data) {
  byte i;
  byte present = 0;
  byte addr[8];
 
  ds.reset_search();
  if ( !ds.search(addr)) {
      ds.reset_search();
      return;
  }
 
  if ( OneWire::crc8( addr, 7) != addr[7]) {
    // CRC is not valid!
    return;
  }
 
  if ( addr[0] != 0x10 && addr[0] != 0x28) {
    // Device family is not recognized: 0x
    return;
  }
 
  ds.reset();
  ds.select(addr);
  ds.write(0x44,1);         // start conversion, with parasite power on at the end
 
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
 
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad
 
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }
 
}
 
float getTemperature() {
  byte data[12];
  updateData(data);
 
  int HighByte, LowByte, TReading, SignBit, Tc_100;
  LowByte = data[0];
  HighByte = data[1];
  TReading = (HighByte << 8) + LowByte;
  SignBit = TReading & 0x8000;  // test most sig bit
 
  if (SignBit) // negative
  {
    TReading = (TReading ^ 0xffff) + 1; // 2's comp
  }
  Tc_100 = (6 * TReading) + TReading / 4;    // multiply by (100 * 0.0625) or 6.25
 
  if (SignBit) {
    Tc_100 = - Tc_100;
  }
 
  return Tc_100 / 100.0;
}
