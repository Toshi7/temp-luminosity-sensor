#include <OneWire.h>
 
// DS18S20 Temperature chip i/o
OneWire ds(23);  // on pin 6
 
void setup(void) {
  // initialize inputs/outputs
  // start serial port
  Serial.begin(115200);
}
 
void loop(void) {
  float temperature = getTemperature();
 
  Serial.print(temperature);
  Serial.println();
  delay(2000);
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
