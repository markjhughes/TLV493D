/*   Infinenon 3D Magnetic I2C
 *   TLV493D
 *   by Mark J. Hughes
 *   for AllAboutCircuits.com
 *   20160817
 */

//--- Begin Includes ---//
#include        // I²C Libraries

// Variable Declaration
const byte addr = 0x5E; // default address of magnetic sensor 0x5E or 0x3E
byte rbuffer[10];       // store data from sensor read registers
byte wbuffer[4];        // store data for sensor write registers.
byte debugcounter;      // variable for debug counter
byte delaytime = 1;     // time to wait before next read.  Delay will increase with errors.

//--- Begin Write Registers ---//
/*  
 *  Mode 1 is the second write register
 *  Mode1_Int   Bxxxxx1xx  Interrupt Enable "1" / Disable "0"
 *  Mode1_Fast  Bxxxxxx1x  Fast Mode Enable "1" / Disable "0" must be 0 for power down
 *  Mode1_Low   Bxxxxxxx1  Low Power Mode Enable "1" / Disable "0"
 *  
 *  Mode 2 is the fourth write register
 *  Mode2_T     B1xxxxxxx  Temperature Measurement Enable "1" / Disable "0"
 *  Mode2_LP    Bx1xxxxxx  LP Period "1" = 12ms / "0"=100ms
 *  Mode2_PT    Bxx1xxxxx  Parity test Enable "1" / Disable "0"
 *  
 */

     Example settings for Ultra-Low Power, Low Power, Fast Mode, and Power Down.
                        Reg 1      Reg 2      Reg 3      Reg 4           
const byte ulpm[] = { B00000000, B00000101, B00000000, B00000000 }; // ultra low power mode
const byte lpm[]  = { B00000000, B00000101, B00000000, B01000000 }; // low power mode
const byte fm[]   = { B00000000, B00000110, B00000000, B00000000 }; // fast mode (unsupported)
const byte pd[]   = { B00000000, B00000001, B00000000, B00000000 }; // power down mode.
 


//--- Begin Setup ---//
void setup() {
  Serial.begin(115200);      // Begin serial connection for debug.
  Wire.begin();              // Begin I²C wire communication

/* Read all registers, although only interested in configuration data
 * stored in rbuffers 7,8,9, as 0-6 might be empty or invalid at the moment.
 */ 
  Wire.requestFrom(addr,sizeof(rbuffer));
  for(int i=0; i < sizeof(rbuffer); i++){
    rbuffer[i] = Wire.read();
    }
// Write Register 0H is non configurable.  Set all bits to 0
wbuffer[0] = B00000000;

// Read Register 7H 6:3 -> Write Register 1H 6:3
wbuffer[1] = rbuffer[7] & B01111000;

// Read Register 8H 7:0 -> Write Register 2H 7:0
wbuffer[2] = rbuffer[8];

// Read Register 9H 4:0 -> Write Register 3H 4:0 (Mod2)
wbuffer[3] = rbuffer[9] & B00001111;
  
// Set Power Mode (ulpm, lpm, fm, pd)
for(int i=0; i < sizeof(wbuffer); i++){
  wbuffer[i] |= lpm[i]
}

  Wire.beginTransmission(addr);        
  for(int i=0; i < sizeof(wbuffer); i++){ 
    Wire.write(wbuffer[i]);               
    }
  Wire.endTransmission();
} 
//--- End of Setup --//

//--- Begin Main Program Loop --//
void loop() {
  
delay(delaytime); // wait time between reads.
// Read sensor registers and store in rbuffer
  Wire.requestFrom(addr,sizeof(rbuffer));
    for(int i=0; i < 6; i++){
      rbuffer[i] = Wire.read();
    }  

// Goto decode functions below     
int x = decodeX(rbuffer[0],rbuffer[4]);
int y = decodeY(rbuffer[1],rbuffer[4]);
int z = decodeZ(rbuffer[2],rbuffer[5]);
int t = decodeT(rbuffer[3],rbuffer[6]);

if(debugcounter % 15 == 0){   // reprint x, y, z, t header every 15 lines.
Serial.print("x"); Serial.print("\t");Serial.print("y");Serial.print("\t");Serial.print("z");Serial.print("\t");Serial.println("t");
}
debugcounter++;               // increment debug counter.

if(rbuffer[3] & B00000011 != 0){ // If bits are not 0, TLV is still reading Bx, By, Bz, or T
  Serial.println("Data read error!");
  //delaytime += 10;
} else { Serial.print(x); Serial.print("\t");Serial.print(y);Serial.print("\t");Serial.print(z);Serial.print("\t");Serial.println(t);}
}
//-- End of Main Program Loop --//

//-- Begin Buffer Decode Routines --//
int decodeX(int a, int b){
/* Shift all bits of register 0 to the left 4 positions.  Bit 8 becomes bit 12.  Bits 0:3 shift in as zero.
 * Determine which of bits 4:7 of register 4 are high, shift them to the right four places -- remask in case
 * they shift in as something other than 0.  bitRead and bitWrite would be a bit more elegant in next version
 * of code.
 */
  int ans = ( a << 4 ) | (((b & B11110000) >> 4) & B00001111);
  if( ans > 1023){ ans -= 2048; } // Interpret bit 12 as +/-
  return ans;
  }

int decodeY(int a, int b){
/* Shift all bits of register 1 to the left 4 positions.  Bit 8 becomes bit 12.  Bits 0-3 shift in as zero.
 * Determine which of the first four bits of register 4 are true.  Add to previous answer.
 */

  int ans = (a << 4) | (b & B00001111);
  if( ans > 1024){ ans -= 2048;} // Interpret bit 12 as +/-
  return ans;
}

int decodeZ(int a, int b){
/* Shift all bits of register 2 to the left 4 positions.  Bit 8 becomes bit 12.  Bits 0-3 are zero.
 * Determine which of the first four bits of register 5 are true.  Add to previous answer.
 */
  int ans = (a << 4) | (b & B00001111);
  if( ans > 1024){ ans -= 2048;}
  return ans;
}

int decodeT(int a, int b){
/* Determine which of the last 4 bits of register 3 are true.  Shift all bits of register 3 to the left 
 * 4 positions.  Bit 8 becomes bit 12.  Bits 0-3 are zero.
 * Determine which of the first four bits of register 6 are true.  Add to previous answer.
 */
  int ans;
  a &= B11110000;
  ans = (a << 4) | b;
  if( ans > 1024){ ans -= 2048;}
  return ans;
}
