

/***********************************************************************
Original Pong Clock v 5.1, Oct 2013 by Nick Hall
Distributed under the terms of the GPL.

See Nick's blog:
http://123led.wordpress.com/

############################################################################
    ******************Works with IDE 1.6.5 *************************
############################################################################
Code modded by Brett Oliver from V5.3
http://home.btconnect.com/brettoliver1/Pong_Clock/Pong_Clock.htm
Added the following
PIR motion detected Display enable
Sync from master clock every 30 seconds
Quick brightness control added
DS3231 added (no change to code required)
1 seconds, 30 second and sync LEDs added
Display mode order re-arranged with my favorite "Slide" in position 1
Countdown timer with alarm added on Normal Mode
Randon mode changed to dual mode, Slide & Pong switching on preset minutes/seconds
Added Adafruit MPC9808 temperature Sensor displayed in Celsius on Slide mode


Removed the following
1 display mode (jumbled character effect)




Release Notes for v7.5 Kitchen clock with timer
Fix bug where timer set before and ending after midnight fails
Added quck brightness call to set_brightness() on press of button Disp Adj on Pong, Slide and Timer mode only


v7.4 test version for 7.5 has code to set RTC for 23:57:00 for testing timer past midnight + many print statements


Release Notes for v7.3 Kitchen clock with timer
Addded temperature display on slide mode
Added Adafruit MPC9808 temperature Sensor and library
Timer function added to normal mode
Added buttons for timer and cntrol buttons for sond module


Release Notes for v7.2 Random mode changed to only use Slide and Pong. Now preset every 8th minute 50 secs Slide until the next 5th minute 10 secs Pong
Slide will then start again at the next 8th minute 50 secs etc etc

Release Notes for v7.1 Slide mode when 1st digit of mins = 9,0,1,2 or 3  Pong mode when 4,5,6,7 or 8 mins VOID

Release Notes for v7.0 set slide & pong as only modes in random mode change every 5 mins VOID

Release Notes for v6.9 serial print of time re enabled


Release Notes for V6.8
Pong clock with rotaion every other minute added


Release Notes for V6.7
Final code tidy up

Release Notes for V6.6
Void

Release Notes for V6.5
Remote Btn 4 directly controls brightness without using setup menu
Brightness adjustable directly over 5 steps

Release Notes for V6.4
6.3 VOID
sync30sec was pin 7 now A0
pin 7 remote BTN 4 connected but not used
increased key reapeat delay when setting clock to allow for remote control input

Release Notes for V6.2
LEDsyncenable replaced with 1 sec LED PIR control of status LEDs

Release Notes for V6.1
30 sync LED held on for 1 sec @ 30secs

Release Notes for V6.0
Sync mods

Release Notes for V5.9
Sync and sync'd LEDs added

Release Notes for V5.8
Button library used for all switches

Release Notes for V5.7
Jumble mode removed to save memory
Slide set as default

Release Notes for V5.6
Man sync button added

Release Notes for V5.5
PIR detector added to blank LEDs when no movement detected

Release Notes for V5.4
Sync pulse now does not sync if RTC already on 30 seconds

Release Notes for V5.3 
My Name added to version number to avoid confusion with original clock
DS3231 added (no code changes)
30 second synchronization added to keep RTC in sync to Master Clock
###################################################################################################

Release Notes for V5.1.

*Fixed a bug in pong mode where the bat missed ball and it wasn't on the minute
*Fixed a bug in normal and slide mode where the date tens digit wasn't cleared when some months changed
*Fixed a bug where the display was corrupted in digits mode and also in 12hr mode
*Addded lower case font. A tweaked version of the one courtesy of Richard Shipman
*Added full LED test on startup.
*Time is printed to serial port for testing
*Menu and set clock items reordered.


Release Notes for V5.0.

*Tested to be compatible with Arduino IDE 1.6.5
*Normal mode now has seconds
*New slide effect mode 
*Separate setup menu
*New pong “Ready?” message
*Daylight Savings mode (DST ADJ) adds / removes 1 hr
*Time now set on upload
*Various other display and code tweaks. Now using the RTClib from Adafruit

Thanks to all who contributed to this including:
SuperTech-IT over at Instructibles, Kirby Heintzelman, Alexandre Suter, Richard Shipman.

Uses 2x Sure 2416 LED modules, arduino and DS1307 clock chip.
Distributed under the terms of the GPL.
             
Holtek HT1632 LED driver chip code:
As implemented on the Sure Electronics DE-DP016 display board (16*24 dot matrix LED module.)
Nov, 2008 by Bill Westfield ("WestfW")
Copyrighted and distributed under the terms of the Berkely license (copy freely, but include this notice of original author.)
***********************************************************************/

//include libraries

#include <Adafruit_MCP9808.h>
// Create the MCP9808 temperature sensor object
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();


#include <ht1632c.h>                     // Holtek LED driver by WestFW - updated to HT1632C by Nick Hall
#include <avr/pgmspace.h>                // Enable data to be stored in Flash Mem as well as SRAM              
#include <Font.h>                        // Font library
#include <Wire.h>                        // DS1307 clock
#include "RTClib.h"                      // DS1307 clock
#include <Button.h>                      // Button library by Alexander Brevig

//define constants
#define ASSERT(condition)                // Nothing
#define X_MAX 47                         // Matrix X max LED coordinate (for 2 displays placed next to each other)
#define Y_MAX 15                         // Matrix Y max LED coordinate (for 2 displays placed next to each other)
#define NUM_DISPLAYS 2                   // Num displays for shadow ram data allocation
#define FADEDELAY 30                     // Time to fade display to black
#define NUM_MODES 9                      // Number of clock & display modes (conting zero as the first mode)
#define NUM_SETTINGS_MODES 5             // Number settings modes = 6 (conting zero as the first mode)
#define NUM_DISPLAY_MODES 4              // Number display modes = 5 (conting zero as the first mode) Brett changed to 4
#define BAT1_X 2                         // Pong left bat x pos (this is where the ball collision occurs, the bat is drawn 1 behind these coords)
#define BAT2_X 45                        // Pong right bat x pos (this is where the ball collision occurs, the bat is drawn 1 behind these coords)
#define plot(x,y,v)  ht1632_plot(x,y,v)  // Plot LED
#define cls          ht1632_clear        // Clear display
#define SLIDE_DELAY 20                   // The time in milliseconds for the slide effect per character in slide mode. Make this higher for a slower effect
#define NEXT_DATE_MIN 10                 // After the date has been displayed automatically, the next time it's displayed is at least 10 mintues later
#define NEXT_DATE_MAX 15                 // After the date has been displayed automatically, the next time it's displayed is at most 15 mintues later

//global variables
static const byte ht1632_data = 10;      // Data pin for sure module
static const byte ht1632_wrclk = 11;     // Write clock pin for sure module
static const byte ht1632_cs[2] = {
  4,5};  // Chip_selects one for each sure module. Remember to set the DIP switches on the modules too.

Button buttonA = Button(2,BUTTON_PULLUP);       // Setup button A (using button library)
Button buttonB = Button(3,BUTTON_PULLUP);       // Setup button B (using button library)
Button buttonSync = Button(6,BUTTON_PULLUP);    // Setup button Sync (using button library)
Button buttonbrightness = Button(7,BUTTON_PULLUP);// Setup button D brightness quick adj (using button library)
Button buttonPIR = Button(8,BUTTON_PULLUP);    // Setup PIR switch (using button library)
Button buttonStTmr = Button(15,BUTTON_PULLUP); // Setup Start timer button (using button library)
Button buttonSetTime = Button(16,BUTTON_PULLUP); // Setup Set time button (using button library)

int sync30sec = A0 ;                       // 30 second sync pulse input
int sync30secval = 0;
int syncset = 1;                          // set to 1 on 0 seconds to enable 30 sec sync pulse
int PIR = 8;                              // PIR input
int LED30sec = 9;                         // 30 second pulse monitor LED
int LEDsync = 12;                         // LED lights when clock is sync'd

int minutes1 = 0;                         // gets 1st mins char from mins 
int minutes2 = 0;                         // gets 2nd mins char from mins 

int start = 0;                            //time timer is started
//int stop  = 0;                            // time timer to stop
                          
//int t3[3];                               //time to stop timer
int rmn3[2];                             // time remaining
int timerEnable = 0;                     // enables timer buttonStTmr if 0
int timerSetEnable = 0;                     // enables timer buttonSetTime if 0
int stopTime = 0;                        // this is the time the timer will run out
int timeNow = 0;                         // current time to be taken off stopTime to get timeRemain
int timeRemain = 0;                      // this is the time remaining on the timer
int tmrStep = 30;                        // amount of seconds timer steps by when buttonSetTime is pressed
int timerValue = 210;                    // this is the time set on the timer when buttonStTmr value incremented by buttonSetTime
                                         // timer deafults to 210 or 4mins (30 will be added on 1st press) 
int tmrOutput = 17;                        // (A3) output when timer reaches 0
int timerVal = 0;                      // used to convert timerValue to show on display when timer setting
float c = 0;                             // temp in C from Adafruit_MCP9808

char celsiusStr[5];                      // converted reading from temperature sensor float value

RTC_DS1307 ds1307;                       //RTC object


int rtc[7];                              // Holds real time clock output
byte clock_mode = 0;                     // Default clock mode. Default = 0 (slide)
byte old_mode = clock_mode;              // Stores the previous clock mode, so if we go to date or whatever, we know what mode to go back to after.
bool ampm = 0;                           // Define 12 or 24 hour time. 0 = 24 hour. 1 = 12 hour 
bool random_mode = 0;                   // Define random mode - changes the display type every few hours. Default = 0 (off)
bool random_modestore = 0;                   // stores value of Random mode when using timer
bool daylight_mode = 0;                // Define if DST is on or off. Default = 0 (off).
byte change_mode_time = 0;               // Holds hour when clock mode will next change if in random mode. 
byte brightness = 7;                    // Screen brightness - default is 7 which half.
byte next_display_date;                  // Holds the minute at which the date is automatically next displayed

char days[7][4] = {
  "Sun","Mon","Tue", "Wed", "Thu", "Fri", "Sat"}; //day array - used in slide, normal and jumble modes (The DS1307 outputs 1-7 values for day of week)
char daysfull[7][9]={
     "Sunday", "Monday", "Tuesday", "Wed", "Thursday", "Friday", "Saturday" };
char suffix[4][3]={
  "st", "nd", "rd", "th"};  //date suffix array, used in slide, normal and jumble modes. e,g, 1st 2nd ...


//intial setup
void setup ()  
{
 
  pinMode(13,OUTPUT);
  pinMode(LEDsync,OUTPUT);
  pinMode(LED30sec,OUTPUT);
  pinMode(sync30sec,INPUT);
  pinMode(tmrOutput, OUTPUT);
 
  digitalWrite(2, HIGH);                 // turn on pullup resistor for button on pin 2
  digitalWrite(3, HIGH);                 // turn on pullup resistor for button on pin 3
  digitalWrite(6, HIGH);                 // turn on pullup resistor for button on pin 6
  digitalWrite(7, HIGH);                 // turn on pullup resistor for button on pin 6
  digitalWrite(8, HIGH);                 // turn on pullup resistor for button on pin 8
  digitalWrite(15, HIGH);                 // turn on pullup resistor for button on pin 15
  digitalWrite(16, HIGH);                 // turn on pullup resistor for button on pin 16

  Serial.begin(57600);                   // Setup serial output 
  ht1632_setup();                        // Setup display (uses flow chart from page 17 of sure datasheet)
  randomSeed(analogRead(1));             // Setup random number generator
  ht1632_sendcmd(0, HT1632_CMD_PWM + brightness);
  ht1632_sendcmd(1, HT1632_CMD_PWM + brightness);
  
  //Setup DS1307 RTC
  #ifdef AVR       
    Wire.begin();
  #else
    Wire1.begin(); // Shield I2C pins connect to alt I2C bus on Arduino 
  #endif
    ds1307.begin(); //start RTC Clock

  if (! ds1307.isrunning()) {
    Serial.println("RTC is NOT running!");
    ds1307.adjust(DateTime(__DATE__, __TIME__));  // sets the RTC to the date & time this sketch was compiled
  }
  printver(); // Display clock software version

   digitalWrite(tmrOutput, HIGH); //Turns off timer alarm sound
   
   //Adafruit  MCP9808 temp sensor
    // Make sure the sensor is found, you can also pass in a different i2c
  // address with tempsensor.begin(0x19) for example
  if (!tempsensor.begin(0x18)) {
 //   Serial.println("Couldn't find MCP9808!");
    while (1);
 }
  // tempsensor.begin(0x18); // loads the  MSP9808 default address

// Brett set ds1307 time to check timer workings uncomment to use
//  DateTime now = ds1307.now();
// ds1307.adjust(DateTime(now.year(), now.month(), now.day(), 23, 56, 0)); 
  // Brett set ds1307 time to check timer workings uncomment to use
}


// ****** MAIN LOOP ******

void loop ()

{
 digitalWrite(tmrOutput, HIGH); //Turns off timer alarm sound
  //run the clock with whatever mode is set by clock_mode - the default is set at top of code.
  
  switch (clock_mode){
        
  case 0: 
    slide();
    break;  
  case 1: 
    pong(); 
    break;
  case 2: 
    digits(); 
    break;
  case 3: 
    word_clock(); 
    break;
  case 4:
  normal(); // now Timer
    break;  
     case 5: 
     setup_menu(); 
    break; 
    
    
  
  }
}


// ****** DISPLAY-ROUTINES ******



//ht1632_chipselect / ht1632_chipfree
//Select or de-select a particular ht1632 chip. De-selecting a chip ends the commands being sent to a chip.
//CD pins are active-low; writing 0 to the pin selects the chip.
void ht1632_chipselect(byte chipno)
{
  DEBUGPRINT("\nHT1632(%d) ", chipno);
  digitalWrite(chipno, 0);
}

void ht1632_chipfree(byte chipno)
{
  DEBUGPRINT(" [done %d]", chipno);
  digitalWrite(chipno, 1);
}


//ht1632_writebits
//Write bits (up to 8) to h1632 on pins ht1632_data, ht1632_wrclk Chip is assumed to already be chip-selected
//Bits are shifted out from MSB to LSB, with the first bit sent being (bits & firstbit), shifted till firsbit is zero.
void ht1632_writebits (byte bits, byte firstbit)
{
  DEBUGPRINT(" ");
  while (firstbit) {
    DEBUGPRINT((bits&firstbit ? "1" : "0"));
    digitalWrite(ht1632_wrclk, LOW);
    if (bits & firstbit) {
      digitalWrite(ht1632_data, HIGH);
    } 
    else {
      digitalWrite(ht1632_data, LOW);
    }
    digitalWrite(ht1632_wrclk, HIGH);
    firstbit >>= 1;
  }
}


// ht1632_sendcmd
// Send a command to the ht1632 chip. A command consists of a 3-bit "CMD" ID, an 8bit command, and one "don't care bit".
//   Select 1 0 0 c7 c6 c5 c4 c3 c2 c1 c0 xx Free
static void ht1632_sendcmd (byte d, byte command)
{
  ht1632_chipselect(ht1632_cs[d]);        // Select chip
  ht1632_writebits(HT1632_ID_CMD, 1<<2);  // send 3 bits of id: COMMMAND
  ht1632_writebits(command, 1<<7);        // send the actual command
  ht1632_writebits(0, 1);         	  // one extra dont-care bit in commands.
  ht1632_chipfree(ht1632_cs[d]);          //done
}


//ht1632_senddata
//send a nibble (4 bits) of data to a particular memory location of the
//ht1632.  The command has 3 bit ID, 7 bits of address, and 4 bits of data.
//   Select 1 0 1 A6 A5 A4 A3 A2 A1 A0 D0 D1 D2 D3 Free
//Note that the address is sent MSB first, while the data is sent LSB first!
//This means that somewhere a bit reversal will have to be done to get
//zero-based addressing of words and dots within words.
static void ht1632_senddata (byte d, byte address, byte data)
{
  ht1632_chipselect(ht1632_cs[d]);      // Select chip
  ht1632_writebits(HT1632_ID_WR, 1<<2); // Send ID: WRITE to RAM
  ht1632_writebits(address, 1<<6);      // Send address
  ht1632_writebits(data, 1<<3);         // Send 4 bits of data
  ht1632_chipfree(ht1632_cs[d]);        // Done.
}


//ht1632_setup
//setup the ht1632 chips
void ht1632_setup()
{
  for (byte d=0; d<NUM_DISPLAYS; d++) {
    pinMode(ht1632_cs[d], OUTPUT);

    digitalWrite(ht1632_cs[d], HIGH);  // Unselect (active low)

    pinMode(ht1632_wrclk, OUTPUT);
    pinMode(ht1632_data, OUTPUT);

    ht1632_sendcmd(d, HT1632_CMD_SYSON);    // System on 
    ht1632_sendcmd(d, HT1632_CMD_LEDON);    // LEDs on 
    ht1632_sendcmd(d, HT1632_CMD_COMS01);   // NMOS Output 24 row x 24 Com mode

    for (byte i=0; i<128; i++)
      ht1632_senddata(d, i, 0);  // clear the display!
      
  }
}


//we keep a copy of the display controller contents so that we can know which bits are on without having to (slowly) read the device.
//Note that we only use the low four bits of the shadow ram, since we're shadowing 4-bit memory.  This makes things faster, and we
//use the other half for a "snapshot" when we want to plot new data based on older data...
byte ht1632_shadowram[NUM_DISPLAYS * 96];  // our copy of the display's RAM


//plot a point on the display, with the upper left hand corner being (0,0).
//Note that Y increases going "downward" in contrast with most mathematical coordiate systems, but in common with many displays
//No error checking; bad things may happen if arguments are out of bounds!  (The ASSERTS compile to nothing by default
void ht1632_plot (char x, char y, char val)
{

  char addr, bitval;

  ASSERT(x >= 0);
  ASSERT(x <= X_MAX);
  ASSERT(y >= 0);
  ASSERT(y <= y_MAX);

  byte d;
  //select display depending on plot values passed in
  if (x >= 0 && x <=23 ) {
    d = 0;
  }  
  if (x >=24 && x <=47) {
    d = 1;
    x = x-24; 
  }   

  /*
   * The 4 bits in a single memory word go DOWN, with the LSB (first transmitted) bit being on top.  However, writebits()
   * sends the MSB first, so we have to do a sort of bit-reversal somewhere.  Here, this is done by shifting the single bit in
   * the opposite direction from what you might expect.
   */

  bitval = 8>>(y&3);  // compute which bit will need set

  addr = (x<<2) + (y>>2);  // compute which memory word this is in 

  if (val) {  // Modify the shadow memory
    ht1632_shadowram[(d * 96)  + addr] |= bitval;
  } 
  else {
    ht1632_shadowram[(d * 96) + addr] &= ~bitval;
  }
  // Now copy the new memory value to the display
  ht1632_senddata(d, addr, ht1632_shadowram[(d * 96) + addr]);
}


//get_shadowram
//return the value of a pixel from the shadow ram.
byte get_shadowram(byte x, byte y)
{
  byte addr, bitval, d;

  //select display depending on plot values passed in
  if (x >= 0 && x <=23 ) {
    d = 0;
  }  
  if (x >=24 && x <=47) {
    d = 1;
    x = x-24; 
  }  

  bitval = 8>>(y&3);  // compute which bit will need set
  addr = (x<<2) + (y>>2);       // compute which memory word this is in 
  return (0 != (ht1632_shadowram[(d * 96) + addr] & bitval));
}


//snapshot_shadowram
//Copy the shadow ram into the snapshot ram (the upper bits)
//This gives us a separate copy so we can plot new data while
//still having a copy of the old data.  snapshotram is NOT
//updated by the plot functions (except "clear")
void snapshot_shadowram()
{
  for (byte i=0; i< sizeof ht1632_shadowram; i++) {
    ht1632_shadowram[i] = (ht1632_shadowram[i] & 0x0F) | ht1632_shadowram[i] << 4;  // Use the upper bits
  }

}

//get_snapshotram
//get a pixel value from the snapshot ram (instead of
//the actual displayed (shadow) memory
byte get_snapshotram(byte x, byte y)
{

  byte addr, bitval;
  byte d = 0;

  //select display depending on plot values passed in 
  if (x >=24 && x <=47) {
    d = 1;
    x = x-24; 
  }  

  bitval = 128>>(y&3);  // user upper bits!
  addr = (x<<2) + (y>>2);   // compute which memory word this is in 
  if (ht1632_shadowram[(d * 96) + addr] & bitval)
    return 1;
  return 0;
}


//ht1632_clear
//clear the display, and the shadow memory, and the snapshot
//memory.  This uses the "write multiple words" capability of
//the chipset by writing all 96 words of memory without raising
//the chipselect signal.
void ht1632_clear()
{
  char i;
  for(byte d=0; d<NUM_DISPLAYS; d++)
  {
    ht1632_chipselect(ht1632_cs[d]);  // Select chip
    ht1632_writebits(HT1632_ID_WR, 1<<2);  // send ID: WRITE to RAM
    ht1632_writebits(0, 1<<6); // Send address
    for (i = 0; i < 96/2; i++) // Clear entire display
      ht1632_writebits(0, 1<<7); // send 8 bits of data
    ht1632_chipfree(ht1632_cs[d]); // done
    for (i=0; i < 96; i++)
      ht1632_shadowram[96*d + i] = 0;
  }
}


/*
 * fade_down
 * fade the display to black
 */
void fade_down() {
  char intensity;
  for (intensity=brightness; intensity >= 0; intensity--) {
    ht1632_sendcmd(0, HT1632_CMD_PWM + intensity); //send intensity commands using CS0 for display 0
    ht1632_sendcmd(1, HT1632_CMD_PWM + intensity); //send intensity commands using CS0 for display 1
    delay(FADEDELAY);
  }
  //clear the display and set it to full brightness again so we're ready to plot new stuff
  cls();
  ht1632_sendcmd(0, HT1632_CMD_PWM + brightness);
  ht1632_sendcmd(1, HT1632_CMD_PWM + brightness);
}


/*
 * fade_up
 * fade the display up to full brightness
 */
void fade_up() {
  char intensity;
  for ( intensity=0; intensity < brightness; intensity++) {
    ht1632_sendcmd(0, HT1632_CMD_PWM + intensity); //send intensity commands using CS0 for display 0
    ht1632_sendcmd(1, HT1632_CMD_PWM + intensity); //send intensity commands using CS0 for display 1
    delay(FADEDELAY);
  }
}


/* ht1632_putchar
 * Copy a 5x7 character glyph from the myfont data structure to display memory, with its upper left at the given coordinate
 * This is unoptimized and simply uses plot() to draw each dot.
 */
void ht1632_putchar(byte x, byte y, char c)
{
   
  byte dots;
  //  if (c >= 'A' && c <= 'Z' || (c >= 'a' && c <= 'z') ) {
  //    c &= 0x1F;   // A-Z maps to 1-26
  //  } 
  if (c >= 'A' && c <= 'Z' ) {
    c &= 0x1F;   // A-Z maps to 1-26
  } 
  else if (c >= 'a' && c <= 'z') {
    c = (c - 'a') + 41;   // A-Z maps to 41-67
  } 
  else if (c >= '0' && c <= '9') {
    c = (c - '0') + 31;
  } 
  else if (c == ' ') {
    c = 0; // space
  }
  else if (c == '.') {
    c = 27; // full stop
  }
  else if (c == '\'') {
    c = 28; // single quote mark
  }  
  else if (c == ':') {
    c = 29; // clock_mode selector arrow
  }
  else if (c == '>') {
    c = 30; // clock_mode selector arrow
  }
  else if (c >= -80 && c<= -67) {
    c *= -1;
  }

  for (char col=0; col< 5; col++) {
    dots = pgm_read_byte_near(&myfont[c][col]);
    for (char row=0; row < 7; row++) {
      //check coords are on screen before trying to plot
      if ((x >= 0) && (x <= X_MAX) && (y >= 0) && (y <= Y_MAX)){
        
        if (dots & (64>>row)) {     // only 7 rows.
          plot(x+col, y+row, 1);
        } else {
          plot(x+col, y+row, 0);
        }  
      }
    }
  }
}

//ht1632_putbigchar
//Copy a 10x14 character glyph from the myfont data structure to display memory, with its upper left at the given coordinate
//This is unoptimized and simply uses plot() to draw each dot.
void ht1632_putbigchar(byte x, byte y, char c)
{
  byte dots;
  if (c >= 'A' && c <= 'Z' || (c >= 'a' && c <= 'z') ) {
    return;   //return, as the 10x14 font contains only numeric characters 
  } 
  if (c >= '0' && c <= '9') {
    c = (c - '0');
    c &= 0x1F;
  } 

  for (byte col=0; col< 10; col++) {
    dots = pgm_read_byte_near(&mybigfont[c][col]);
    for (char row=0; row < 8; row++) {
      if (dots & (128>>row))   	   
        plot(x+col, y+row, 1);
      else 
        plot(x+col, y+row, 0);
    }

    dots = pgm_read_byte_near(&mybigfont[c][col+10]);
    for (char row=0; row < 8; row++) {
      if (dots & (128>>row))   	   
        plot(x+col, y+row+8, 1);
      else 
        plot(x+col, y+row+8, 0);
    } 
  }  
}


// ht1632_puttinychar
// Copy a 3x5 character glyph from the myfont data structure to display memory, with its upper left at the given coordinate
// This is unoptimized and simply uses plot() to draw each dot.
void ht1632_puttinychar(byte x, byte y, char c)
{
  byte dots;
  if (c >= 'A' && c <= 'Z' || (c >= 'a' && c <= 'z') ) {
    c &= 0x1F;   // A-Z maps to 1-26
  } 
  else if (c >= '0' && c <= '9') {
    c = (c - '0') + 31;
  } 
  else if (c == ' ') {
    c = 0; // space
  }
  else if (c == '.') {
    c = 27; // full stop
  }
  else if (c == '\'') {
    c = 28; // single quote mark
  } 
  else if (c == '!') {
    c = 29; // single quote mark
  }  
  else if (c == '?') {
    c = 30; // single quote mark
  }

  for (byte col=0; col< 3; col++) {
    dots = pgm_read_byte_near(&mytinyfont[c][col]);
    for (char row=0; row < 5; row++) {
      if (dots & (16>>row))   	   
        plot(x+col, y+row, 1);
      else 
        plot(x+col, y+row, 0);
    }
  }  
}




//************ CLOCK MODES ***************



// digits()
// show the time in 10x14 characters and update it whilst the loop runs
void digits()
{
  //Brett turns random mode on if on before and returns random mode store to 0

if (random_modestore == 1)
{
random_mode = random_modestore;
    
}
//Brett end turns random mode on if on before

  cls();
  char buffer[3];   //for int to char conversion to turn rtc values into chars we can print on screen
  byte offset = 0;  //used to offset the x postition of the digits and centre the display when we are in 12 hour mode and the clock shows only 3 digits. e.g. 3:21
  byte x,y;         //used to draw a clear box over the left hand "1" of the display when we roll from 12:59 -> 1:00am in 12 hour mode. 

  //do 12/24 hour conversion if ampm set to 1
  byte hours = rtc[2];
  if (hours > 12) {
    hours = hours - ampm * 12;
  }
  if (hours < 1) {
    hours = hours + ampm * 12;
  }

  //set the next minute we show the date at
  set_next_date();

  // initially set mins to value 100 - so it wll never equal rtc[1] on the first loop of the clock, meaning we draw the clock display when we enter the function
  byte secs = 100;
  byte mins = 100;
  int count = 0;

  //run clock main loop as long as run_mode returns true
  while (run_mode()){

    //get the time from the clock chip
    get_time();

    //check to see if the buttons have been pressed    
    if(buttonA.uniquePress()){
      switch_mode(); // pick the new mode via the menu
      return; //exit this mode back to loop(), so the new clock mode function is called.
    }
    if(buttonB.uniquePress()){
      display_date();
      fade_down();
      return; //exit the mode back to loop(), Then we reenter the function and the clock mode starts again. This refreshes the display
    }

   //Brett select timer mode from any other mode by pressing buttonStTmr
   if(buttonStTmr.uniquePress()){
     
      fade_down();
      clock_mode = 4;
     
      return; //exit the mode back to loop(), Then we reenter the function and the clock mode starts again. This refreshes the display
    }

 

    //check whether it's time to automatically display the date
    check_show_date();

    //draw the flashing : as on if the secs have changed.
    if (secs != rtc[0]) {  

      //update secs with new value
      secs = rtc[0];

      //draw :
      plot (23 - offset,4,1); //top point
      plot (23 - offset,5,1);
      plot (24 - offset,4,1);
      plot (24 - offset,5,1);
      plot (23 - offset,10,1); //bottom point
      plot (23 - offset,11,1);
      plot (24 - offset,10,1);
      plot (24 - offset,11,1);
      count = 400;
    }    

    //if count has run out, turn off the :
    if (count == 0){
      plot (23 - offset,4,0); //top point
      plot (23 - offset,5,0);
      plot (24 - offset,4,0);
      plot (24 - offset,5,0);
      plot (23 - offset,10,0); //bottom point
      plot (23 - offset,11,0);
      plot (24 - offset,10,0);
      plot (24 - offset,11,0);
    } 
    else {
      count--;
    }


    //re draw the display if mins != rtc[1] i.e. if the time has changed from what we had stored in mins, (also trigggered on first entering function when mins is 100)
    if (mins != rtc[1]) {  
         
      //update mins and hours with the new values
      mins = rtc[1];
      hours = rtc[2];
      
      //adjust hours of ampm set to 12 hour mode
      if (hours > 12) {
        hours = hours - ampm * 12;
      }
      if (hours < 1) {
        hours = hours + ampm * 12;
      }
      
      itoa(hours,buffer,10);
      
      //if hours < 10 the num e.g. "3" hours, itoa coverts this to chars with space "3 " which we dont want
      if (hours < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }

      //print hours 
      //if we in 12 hour mode and hours < 10, then don't print the leading zero, and set the offset so we centre the display with 3 digits. 
      if (ampm && hours < 10) {
        offset = 5;
        
        //if the time is 1:00am clear the entire display as the offset changes at this time and we need to blank out the old 12:59
        if((hours == 1 && mins == 0) ){
         cls();
        } 
      } 
      else {
        //else no offset and print hours tens digit
        offset = 0;
        
        //if the time is 10:00am clear the entire display as the offset changes at this time and we need to blank out the old 9:59
        if (hours == 10 && mins == 0) {
          cls();
        } 
        
        
        ht1632_putbigchar(0,  1, buffer[0]);
      }
      //print hours ones digit
      ht1632_putbigchar(12 - offset, 1, buffer[1]);


      //print mins
      //add leading zero if mins < 10
      itoa (mins, buffer, 10);
      if (mins < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }
      //print mins tens and ones digits
      ht1632_putbigchar(26 - offset, 1, buffer[0]);
      ht1632_putbigchar(38 - offset, 1, buffer[1]);  
    }
  }
  fade_down();
}



// play pong - using the score as the time
void pong(){

  //Brett turns random mode on if on before and returns random mode store to 0

if (random_modestore == 1)
{
random_mode = random_modestore;
    
}
//Brett end turns random mode on if on before


  float ballpos_x, ballpos_y;
  byte erase_x = 10;  //holds ball old pos so we can erase it, set to blank area of screen initially.
  byte erase_y = 10;
  float ballvel_x, ballvel_y;
  int bat1_y = 5;  //bat starting y positions
  int bat2_y = 5;  
  int bat1_target_y = 5;  //bat targets for bats to move to
  int bat2_target_y = 5;
  byte bat1_update = 1;  //flags - set to update bat position
  byte bat2_update = 1;
  byte bat1miss, bat2miss; //flags set on the minute or hour that trigger the bats to miss the ball, thus upping the score to match the time.
  byte restart = 1;   //game restart flag - set to 1 initially to setup 1st game
  byte bat_start_range; //y range that bat can be in so it hits the ball - used when we know we need to hit the ball, but want some randomness as to which part of the 6 pixel bat is hit
  byte bat_end_range;
  
  //show "play pong" message and draw pitch line
  pong_setup();
 
  //run clock main loop as long as run_mode returns true
  while (run_mode()){

    if(buttonA.uniquePress()){
      switch_mode();  
      return;
    }
    if(buttonB.uniquePress()){
      display_date();
      fade_down();
      return;
    }

    // Brett 
// Quick Brightness control button D
   if(buttonbrightness.uniquePress())
    {
      fade_down();
      set_brightness();
       return;
      
         }
//Brett

  //Brett select timer mode from any other mode by pressing buttonStTmr
  if(buttonStTmr.uniquePress()){
      //void normal(); // Now Timer
      clock_mode = 4;
     // fade_down();
      return; //exit the mode back to loop(), Then we reenter the function and the clock mode starts again. This refreshes the display
    }    


    //if restart flag is 1, setup a new game
    if (restart) {

      //check whether it's time to automatically display the date
      if (check_show_date()){
        //if date was displayed, we need to redraw the entire display, so run the setup routine
        pong_setup();
      }

      //erase ball pos
      plot (erase_x, erase_y, 0);

      //update score / time
      byte mins = rtc[1];
      byte hours = rtc[2];
      if (hours > 12) {
        hours = hours - ampm * 12;
      }
      if (hours < 1) {
        hours = hours + ampm * 12;
      }

      char buffer[3];

      itoa(hours,buffer,10);
      //fix  - as otherwise if num has leading zero, e.g. "03" hours, itoa coverts this to chars with space "3 ". 
      if (hours < 10) {
        buffer[1] = buffer[0];
        //uncomment the next 5 lines that are commented out if you want to blank the leading hours zero in pong if in 12 hour mode.
        //if (ampm) {
        //  buffer[0] = ' ';
        //}
        //else{
          buffer[0] = '0';
        //}
      }
      ht1632_puttinychar(14, 0, buffer[0] );
      ht1632_puttinychar(18, 0, buffer[1]);

      itoa(mins,buffer,10); 
      if (mins < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      } 
      ht1632_puttinychar(28, 0, buffer[0]);
      ht1632_puttinychar(32, 0, buffer[1]);  

      //set ball start pos
      ballpos_x = 23;
      ballpos_y = random (4,12);

      //pick random ball direction
      if (random(0,2) > 0) {
        ballvel_x = 1; 
      } 
      else {
        ballvel_x = -1;
      }
      if (random(0,2) > 0) {
        ballvel_y = 0.5; 
      } 
      else {
        ballvel_y = -0.5;
      }
      //draw bats in initial positions
      bat1miss = 0; 
      bat2miss = 0;
      //reset game restart flag
      restart = 0;

      //short wait before we launch the ball for effect
      delay(400);
    }

    //get the time from the rtc
    get_time();
 
    //if coming up to the minute: secs = 59 and mins < 59, flag bat 2 (right side) to miss the return so we inc the minutes score
    if (rtc[0] == 59 && rtc[1] < 59){
      bat1miss = 1;
    }
    // if coming up to the hour: secs = 59  and mins = 59, flag bat 1 (left side) to miss the return, so we inc the hours score.
    if (rtc[0] == 59 && rtc[1] == 59){
      bat2miss = 1;
    }


    //AI - we run 2 sets of 'AI' for each bat to work out where to go to hit the ball back 

    //very basic AI...
    // For each bat, First just tell the bat to move to the height of the ball when we get to a random location.
    //for bat1
    if (ballpos_x == random(30,41)){// && ballvel_x < 0) {
      bat1_target_y = ballpos_y;
    }
    //for bat2
    if (ballpos_x == random(8,19)){//  && ballvel_x > 0) {
      bat2_target_y = ballpos_y;
    }

    //when the ball is closer to the left bat, run the ball maths to find out where the ball will land
    if (ballpos_x == 23 && ballvel_x < 0) {
  
      byte end_ball_y = pong_get_ball_endpoint(ballpos_x, ballpos_y, ballvel_x, ballvel_y);
      
      //if the miss flag is set,  then the bat needs to miss the ball when it gets to end_ball_y
      if (bat1miss == 1){
        bat1miss = 0;
        if ( end_ball_y > 8){
          bat1_target_y = random (0,3); 
        } 
        else {
          bat1_target_y = 8 + random (0,3);              
        }      
      }
       
      //if the miss flag isn't set,  set bat target to ball end point with some randomness so its not always hitting top of bat
      else {
        
        if (end_ball_y <= 5){
          bat_start_range = 0;
        } else {
          bat_start_range = end_ball_y - 5;
        }
        
        //bat end range. if ball <= 10 end range = ball end. else ball end = 10
        if (end_ball_y <= 10){
          bat_end_range = end_ball_y;
        } else {
          bat_end_range = 10; 
        }
        
        bat1_target_y = random(bat_start_range,bat_end_range+1);
      }
    }


    //right bat AI
    //if positive velocity then predict for right bat - first just match ball height

    //when the ball is closer to the right bat, run the ball maths to find out where it will land
    if (ballpos_x == 25 && ballvel_x > 0) {
     
      byte end_ball_y = pong_get_ball_endpoint(ballpos_x, ballpos_y, ballvel_x, ballvel_y);

      //if flag set to miss, move bat out way of ball
      if (bat2miss == 1){
        bat2miss = 0;
        //if ball end point above 8 then move bat down, else move it up- so either way it misses
        if (end_ball_y > 8){
          bat2_target_y = random (0,3); 
        } else {
          bat2_target_y = 8 + random (0,3);
        }      
      } else {
        
        //set bat target range - i.e. the range of coords bat can be at to where it will still hit the ball. 
        // We pick a random value in this range so the ball doesn't always hit the top pixel of the bat
        //bat is 6 pixels long. values below are top pixel of bat and take into account it's 6 pixel length
        
        // if  ball end 0 bat end pos range = 0 to 0
        // if  ball end 1 bat end pos = 0 to 1
        // if  ball end 2 bat end pos = 0 to 2
        // if  ball end 3 bat end pos = 0 to 3
        // if  ball end 4 bat end pos = 0 to 4
        // if  ball end 5 bat end pos = 0 to 5
        // if  ball end 6 bat end pos = 1 to 6
        // if  ball end 7 bat end pos = 2 up 7
        // if  ball end 8 bat end pos = 3 to 8
        // if  ball end 9 bat end pos = 4 to 9
        // if  ball end 10 bat end pos = 5 to 10
        // if  ball end 11 bat end pos = 6 to 10 
        // if  ball end 12 bat end pos = 7 to 10 
        // if  ball end 13 bat end pos = 8 to 10 
        // if  ball end 14 bat end pos = 9 to 10 
        // if  ball end 15 bat end pos = 10 to 10 
                  
        //bat start range. If ball <= 5 start range = 0. else range = ball end - 5
        if (end_ball_y <= 5){
          bat_start_range = 0;
        } else {
          bat_start_range = end_ball_y - 5;
        }
        
        //bat end range. if ball <= 10 end range = ball end. else ball end = 10
        if (end_ball_y <= 10){
          bat_end_range = end_ball_y;
        } else {
          bat_end_range = 10; 
        }

        bat2_target_y = random(bat_start_range,bat_end_range+1);
      }
    }
    

    //move bat 1 towards target    
    //if bat y greater than target y move down until hit 0 (dont go any further or bat will move off screen)
    if (bat1_y > bat1_target_y && bat1_y > 0 ) {
      bat1_y--;
      bat1_update = 1;
    }

    //if bat y less than target y move up until hit 10 (as bat is 6)
    if (bat1_y < bat1_target_y && bat1_y < 10) {
      bat1_y++;
      bat1_update = 1;
    }
    
    //draw bat 1
    if (bat1_update){
      for (byte i = 0; i < 16; i++){
        if (i - bat1_y < 6 &&  i - bat1_y > -1){
          plot(BAT1_X-1, i , 1);
          plot(BAT1_X-2, i , 1);
        } 
        else {
          plot(BAT1_X-1, i , 0);
          plot(BAT1_X-2, i , 0);
        }
      } 
    }


    //move bat 2 towards target

    //if bat y greater than target y move down until hit 0
    if (bat2_y > bat2_target_y && bat2_y > 0 ) {
      bat2_y--;
      bat2_update = 1;
    }

    //if bat y less than target y move up until hit max of 10 (as bat is 6)
    if (bat2_y < bat2_target_y && bat2_y < 10) {
      bat2_y++;
      bat2_update = 1;
    }

    //draw bat2
    if (bat2_update){
      for (byte i = 0; i < 16; i++){
        if (  i - bat2_y < 6 && i - bat2_y > -1){
          plot(BAT2_X+1, i , 1);
          plot(BAT2_X+2, i , 1);
        } 
        else {
          plot(BAT2_X+1, i , 0);
          plot(BAT2_X+2, i , 0);
        }
      } 
    }

    //update the ball position using the velocity
    ballpos_x =  ballpos_x + ballvel_x;
    ballpos_y =  ballpos_y + ballvel_y;

    //check ball collision with top of screen and reverse the y velocity if hit
    if (ballpos_y <= 0 ){
      ballvel_y = ballvel_y * -1;
      ballpos_y = 0; //make sure value goes no less that 0
    }
    
    //check ball collision with bottom of screen and reverse the y velocity if hit
    if (ballpos_y >= 15){
      ballvel_y = ballvel_y * -1;
      ballpos_y = 15; //make sure value goes no more than 15
    }

    //check for ball collision with bat1. check ballx is same as batx
    //and also check if bally lies within width of bat i.e. baty to baty + 6. We can use the exp if(a < b && b < c) 
    if ((int)ballpos_x == BAT1_X && (bat1_y <= (int)ballpos_y && (int)ballpos_y <= bat1_y + 5) ) { 

      //random if bat flicks ball to return it - and therefor changes ball velocity
      if(!random(0,3)) { //not true == no flick - just straight rebound and no change to ball y vel
        ballvel_x = ballvel_x * -1;
      } 
      else {
        bat1_update = 1;
        byte flick;  //0 = up, 1 = down.

        if (bat1_y > 1 || bat1_y < 8){
          flick = random(0,2);   //pick a random dir to flick - up or down
        }

        //if bat 1 or 2 away from top only flick down
        if (bat1_y <=1 ){
          flick = 0;   //move bat down 1 or 2 pixels 
        } 
        //if bat 1 or 2 away from bottom only flick up
        if (bat1_y >=  8 ){
          flick = 1;  //move bat up 1 or 2 pixels 
        }

        switch (flick) {
          //flick up
        case 0:
          bat1_target_y = bat1_target_y + random(1,3);
          ballvel_x = ballvel_x * -1;
          if (ballvel_y < 2) {
            ballvel_y = ballvel_y + 0.2;
          }
          break;

          //flick down
        case 1:   
          bat1_target_y = bat1_target_y - random(1,3);
          ballvel_x = ballvel_x * -1;
          if (ballvel_y > 0.2) {
            ballvel_y = ballvel_y - 0.2;
          }
          break;
        }
      }
    }


    //check for ball collision with bat2. check ballx is same as batx
    //and also check if bally lies within width of bat i.e. baty to baty + 6. We can use the exp if(a < b && b < c) 
    if ((int)ballpos_x == BAT2_X && (bat2_y <= (int)ballpos_y && (int)ballpos_y <= bat2_y + 5) ) { 

      //random if bat flicks ball to return it - and therefor changes ball velocity
      if(!random(0,3)) {
        ballvel_x = ballvel_x * -1;    //not true = no flick - just straight rebound and no change to ball y vel
      } 
      else {
        bat1_update = 1;
        byte flick;  //0 = up, 1 = down.

        if (bat2_y > 1 || bat2_y < 8){
          flick = random(0,2);   //pick a random dir to flick - up or down
        }
        //if bat 1 or 2 away from top only flick down
        if (bat2_y <= 1 ){
          flick = 0;  //move bat up 1 or 2 pixels 
        } 
        //if bat 1 or 2 away from bottom only flick up
        if (bat2_y >=  8 ){
          flick = 1;   //move bat down 1 or 2 pixels 
        }

        switch (flick) {
          //flick up
        case 0:
          bat2_target_y = bat2_target_y + random(1,3);
          ballvel_x = ballvel_x * -1;
          if (ballvel_y < 2) {
            ballvel_y = ballvel_y + 0.2;
          }
          break;

          //flick down
        case 1:   
          bat2_target_y = bat2_target_y - random(1,3);
          ballvel_x = ballvel_x * -1;
          if (ballvel_y > 0.2) {
            ballvel_y = ballvel_y - 0.2;
          }
          break;
        }
      }
    }
    
    //plot the ball on the screen
    byte plot_x = (int)(ballpos_x + 0.5f);
    byte plot_y = (int)(ballpos_y + 0.5f);

    //take a snapshot of all the led states
    snapshot_shadowram();

    //if the led at the ball pos is on already, dont bother printing the ball.
    if (get_shadowram(plot_x, plot_y)){
      //erase old point, but don't update the erase positions, so next loop the same point will be erased rather than this point which shuldn't be
      plot (erase_x, erase_y, 0);   
    } 
    else {
      //else plot the ball and erase the old position
      plot (plot_x, plot_y, 1);     
      plot (erase_x, erase_y, 0); 
      //reset erase to new pos
      erase_x = plot_x; 
      erase_y = plot_y;
    }

    //check if a bat missed the ball. if it did, reset the game.
    if ((int)ballpos_x == 0 ||(int) ballpos_x == 47){
      restart = 1; 
    } 
    delay(20);
  } 
  fade_down();
}

//pong ball calculation
byte pong_get_ball_endpoint(float tempballpos_x, float  tempballpos_y, float  tempballvel_x, float tempballvel_y) {

  //run calculations until ball hits bat
  while (tempballpos_x > BAT1_X && tempballpos_x < BAT2_X  ){     
    tempballpos_x = tempballpos_x + tempballvel_x;
    tempballpos_y = tempballpos_y + tempballvel_y;
    
    //check for collisions with top / bottom
     if (tempballpos_y <= 0 ){
      tempballvel_y = tempballvel_y * -1;
      tempballpos_y = 0; //make sure value goes no less that 0
    }

    if (tempballpos_y >= 15){
      tempballvel_y = tempballvel_y * -1;
      tempballpos_y = 15; //make sure value goes no more than 15
    }
  }  
  return tempballpos_y; 
}


//play pong message and draw centre line
void pong_setup(){
  //setup
  cls();
  
  //uncomment for clear screen "wipe" effect
  //flashing_cursor (0,0,47,16,0);

  //uncomment for "Ready? Play Pong!" Message
  byte i = 0;
  char intro1[6] = "Ready";
  while(intro1[i]) {
    delay(25);
    ht1632_puttinychar((i*4)+12, 4, intro1[i]); 
    i++;
  }
  //flashing question mark
  for (byte i = 0; i <2; i++) {
    ht1632_puttinychar(34, 4, '?');
    delay(200);
    ht1632_puttinychar(34, 4, ' ');
    delay(400);
  }

  cls();
  i = 0;
  char intro2[11] = "Play Pong!";
  while(intro2[i]) {
    ht1632_puttinychar((i*4)+6, 4, intro2[i]); 
    i++;
  }
  delay(800); //hold message for a moment
  cls();
  // end "Play Pong!" Message code

  get_time();

  //draw pitch centre line
  byte offset = random(0,2); //offset top centre line randomly, so different led's get used.
  for (byte i = 0; i <16; i++) {

    //uncomment for dotted centre line
    if ( i % 2 == 0 ) { //plot point if an even number
      plot(24, i+offset, 1);
      delay(30);
    }
    // end dotted centre line code

    // uncomment for solid pitch centre line if peferred
    //plot(24, i, 1); 
  } 
 
  delay(120); //delay for effect
}


//print a clock using words rather than numbers
void word_clock() {
  
  //Brett turns random mode on if on before and returns random mode store to 0

if (random_modestore == 1)
{
random_mode = random_modestore;
    
}
//Brett end turns random mode on if on before
  cls();

  char numbers[19][10]   = { 
    "one", "two", "three", "four","five","six","seven","eight","nine","ten",
    "eleven","twelve", "thirteen","fourteen","fifteen","sixteen","seventeen","eighteen","nineteen"};              
  char numberstens[5][7] = { 
    "ten","twenty","thirty","forty","fifty"};

  byte hours_y, mins_y; //hours and mins and positions for hours and mins lines  

  byte hours = rtc[2];
  if (hours > 12) {
    hours = hours - ampm * 12;
  }
  if (hours < 1) {
    hours = hours + ampm * 12;
  }

  get_time(); //get the time from the clock chip
  byte old_mins = 100; //store mins in old_mins. We compare mins and old mins & when they are different we redraw the display. Set this to 100 initially so display is drawn when mode starts.
  byte mins;

  //run clock main loop as long as run_mode returns true
  while (run_mode()){ 

    if(buttonA.uniquePress()){
      switch_mode();
      return;
    }
    if(buttonB.uniquePress()){
      display_date();
      fade_down();
      return;

 
    }
  //Brett select timer mode from any other mode by pressing buttonStTmr
 if(buttonStTmr.uniquePress()){
      //void normal();
      clock_mode = 4;
     // fade_down();
      return; //exit the mode back to loop(), Then we reenter the function and the clock mode starts again. This refreshes the display
    }
    get_time(); //get the time from the clock chip
    mins = rtc[1];  //get mins

  
    //if mins is different from old_mins - redraw display  
    if (mins != old_mins){

      //update old_mins with current mins value
      old_mins = mins;

      //reset these for comparison next time
      mins = rtc[1];   
      hours = rtc[2];

      //make hours into 12 hour format
      if (hours > 12){ 
        hours = hours - 12; 
      }
      if (hours == 0){ 
        hours = 12; 
      } 

      //split mins value up into two separate digits 
      int minsdigit = rtc[1] % 10;
      byte minsdigitten = (rtc[1] / 10) % 10;

      char str_top[12];
      char str_bot[12];

      //if mins <= 10 , then top line has to read "minsdigti past" and bottom line reads hours
      if (mins < 10) {     
        strcpy (str_top,numbers[minsdigit - 1]);
        strcat (str_top," PAST");
        strcpy (str_bot,numbers[hours - 1]);
      }
      //if mins = 10, cant use minsdigit as above, so soecial case to print 10 past /n hour.
      if (mins == 10) {     
        strcpy (str_top,numbers[9]);
        strcat (str_top," PAST");
        strcpy (str_bot,numbers[hours - 1]);
      }

      //if time is not on the hour - i.e. both mins digits are not zero, 
      //then make top line read "hours" and bottom line read "minstens mins" e.g. "three /n twenty one"
      else if (minsdigitten != 0 && minsdigit != 0  ) {

        strcpy (str_top,numbers[hours - 1]); 

        //if mins is in the teens, use teens from the numbers array for the bottom line, e.g. "three /n fifteen"
        if (mins >= 11 && mins <= 19) {
          strcpy (str_bot, numbers[mins - 1]);

          //else bottom line reads "minstens mins" e.g. "three \n twenty three"
        } 
        else {     
          strcpy (str_bot, numberstens[minsdigitten - 1]);
          strcat (str_bot, " "); 
          strcat (str_bot, numbers[minsdigit - 1]); 
        }
      }
      // if mins digit is zero, don't print it. read read "hours" "minstens" e.g. "three /n twenty"
      else if (minsdigitten != 0 && minsdigit == 0  ) {
        strcpy (str_top, numbers[hours - 1]);     
        strcpy (str_bot, numberstens[minsdigitten - 1]);
      }

      //if both mins are zero, i.e. it is on the hour, the top line reads "hours" and bottom line reads "o'clock"
      else if (minsdigitten == 0 && minsdigit == 0  ) {
        strcpy (str_top,numbers[hours - 1]);     
        strcpy (str_bot, "O'CLOCK");
      }

      //work out offset to center top line on display. 
      byte len = 0;
      while(str_top[len]) { 
        len++; 
      }; //get length of message
      byte offset_top = (X_MAX - ((len - 1)*4)) / 2; //

      //work out offset to center bottom line on display. 
      len = 0;
      while(str_bot[len]) { 
        len++; 
      }; //get length of message
      byte offset_bot = (X_MAX - ((len - 1)*4)) / 2; //

      fade_down();

      //plot hours line
      byte i = 0;
      while(str_top[i]) {
        ht1632_puttinychar((i*4) + offset_top, 2, str_top[i]); 
        i++;
      }

      i = 0;
      while(str_bot[i]) {
        ht1632_puttinychar((i*4) + offset_bot, 9, str_bot[i]); 
        i++;
      }
    }
    delay (50); 
  }
  fade_down();
}

/*
//show time and date and use a random jumble of letters transition each time the time changes.
void jumble() {

  char allchars[63] = {
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890"    }; //chars to show as part of jumble effect
  char endchar[16]; // the 16 end characters that the clock settles on
  byte counter[16]; //16 counters - one for each character on the display. These each count down from a random number to 0. When they hit 0 the end charater is displayed
  byte mins = 100;
  byte seq[16];
  byte secs = rtc[0]; //seconds
  byte old_secs = secs; //holds old seconds value - from last time seconds were updated o display - used to check if seconds have changed

  cls();
  
  //run clock main loop as long as run_mode returns true
  while (run_mode()){

    get_time();
  
    //check buttons 
    if(buttonA.uniquePress()){
      switch_mode();
      return;      
    }
    if(buttonB.uniquePress()){
      display_date();
      fade_down();
      return;
    }

    //if secs changed then update them on the display
    secs = rtc[0];
    if (secs != old_secs){

      //secs
      char buffer[3];
      itoa(secs,buffer,10);

      //fix - as otherwise if num has leading zero, e.g. "03" secs, itoa coverts this to chars with space "3 ". 
      if (secs < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }

      ht1632_putchar( 30, 0, ':'); //seconds colon
      ht1632_putchar( 36, 0, buffer[0]); //seconds
      ht1632_putchar( 42, 0, buffer[1]); //seconds
      old_secs = secs;
    }

    //if minute changes or show runs out, do the jumble effect
    if (mins != rtc[1]  ) {  

      //fill an arry with 0-15 and randomize the order so we can plot letters in a jumbled pattern rather than sequentially
      for (int i=0; i<16; i++) {
        seq[i] = i;  // fill the array in order
      }
      //randomise out array of numbers 
      for (int i=0; i<(16-1); i++) {
        int r = i + (rand() % (16-i)); // Random remaining position.
        int temp = seq[i]; 
        seq[i] = seq[r]; 
        seq[r] = temp;
      }


      //reset these for comparison next time 
      mins = rtc[1];
      byte hours = rtc[2];   
      if (hours > 12) {
        hours = hours - ampm * 12;
      }
      if (hours < 1) {
        hours = hours + ampm * 12;
      }

      byte dow   = rtc[3]; // the DS1307 outputs 0 - 6 where 0 = Sunday. 
      byte date  = rtc[4];

      byte alldone = 0;

      //set counters to random numbers between 3 - 23. They show a random character until they get to 0, then the correct end char is shown
      for(byte c=0; c<16 ; c++) {
        counter[c] = 3 + random (0,20);
      }

      //set final characters
      char buffer[3];
      itoa(hours,buffer,10);

      //fix - as otherwise if num has leading zero, e.g. "03" hours, itoa coverts this to chars with space "3 ". 
      if (hours < 10) {
        buffer[1] = buffer[0];

        //if we are in 12 hour mode blank the leading zero.
        if (ampm) {
          buffer[0] = ' ';
        }
        else{
          buffer[0] = '0';
        }
      }

      endchar[0] = buffer[0];
      endchar[1] = buffer[1];
      endchar[2] = ':';

      itoa (mins, buffer, 10);
      if (mins < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }

      endchar[3] = buffer[0];
      endchar[4] = buffer[1];

      itoa (date, buffer, 10);
      if (date < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }

      //second colon on top line
      endchar[5] = ':';

      //then work out date 2 letter suffix - eg st, nd, rd, th etc
      //char suffix[4][3]={"st", "nd", "rd", "th"}; is defined at top of code
      byte s = 3; 
      if(date == 1 || date == 21 || date == 31) {
        s = 0;
      } 
      else if (date == 2 || date == 22) {
        s = 1;
      } 
      else if (date == 3 || date == 23) {
        s = 2;
      }

      //endchar[5] = ' ';
      //endchar[6] = ' ';
      //endchar[7] = ' ';

      //set bottom line
      endchar[8] = days[dow][0];
      endchar[9] = days[dow][1];
      endchar[10] = days[dow][2];
      endchar[11] = ' ';
      endchar[12] = buffer[0];
      endchar[13] = buffer[1];
      endchar[14] = suffix[s][0];
      endchar[15] = suffix[s][1];

      byte x = 0;
      byte y = 0;

      //until all counters are 0
      while (alldone < 16){

        //for each char    
        for(byte c=0; c<16 ; c++) {

          //update endchar for seconds during jumble effect - as they may change. rtc[0] is secs from DS1307
          get_time();
          char buffer[3];
          secs = rtc[0];
          itoa(secs,buffer,10);
          //fix - as otherwise if num has leading zero, e.g. "03" secs, itoa coverts this to chars with space "3 ". 
          if (secs < 10) {
            buffer[1] = buffer[0];
            buffer[0] = '0';
          }
          //set seconds as end chars
          endchar[6] = buffer[0];
          endchar[7] = buffer[1];

          //seq
          if (seq[c] < 8) { 
            x = 0;
            y = 0; 
          } 
          else {
            x = 8;
            y = 8;   
          }

          //if counter > 1 then put random char
          if (counter[ seq[c] ] > 0) {
            ht1632_putchar( ( seq[c] -x) * 6, y, allchars[random(0,63)]); //random
            counter[ seq[c] ]--;
          } 

          //if counter == 1 then put final char, the set counter to 0. As we don't want to trigger this 'if' again as we don't want to keep inc'ing alldone.
          //if (counter[ seq[c] ] <= 1) {
          if (counter[ seq[c] ] == 1) {
            ht1632_putchar( (seq[c]-x) * 6, y, endchar[seq[c]]); //final char
            counter[seq[c]] = 0; //set counter to 0
            alldone++; //increment done tally of counters
          } 

          //if counter == 0 then print the char again but don't inc alldone. The only chars that should change are the seconds.
          if (counter[seq[c]] == 0) {
            ht1632_putchar( (seq[c]-x) * 6, y, endchar[seq[c]]);
            ;
          }

          if(buttonA.uniquePress()){
            switch_mode();
            return;      
          }
        }
      }
    }
    delay(50);
  } 
  fade_down();
}
*/
void normal() { // Now Timer

  char textchar[16]; // the 16 characters on the display
  byte mins = 100; //mins
  byte secs = rtc[0]; //seconds
  byte old_secs = secs; //holds old seconds value - from last time seconds were updated o display - used to check if seconds have changed
  byte minsTmr = 100; //mins
  byte secsTmr = rmn3[0]; //seconds timer
  byte old_secsTmr = secsTmr; //holds old seconds timer value - from last time timer seconds were updated on the display - used to check if Timer seconds have changed
  cls();
  timerValue = 240; //On each load of timer the timer is set to 4mins
  
  //if random mode is on, turn it off while in timer mode
  if (random_mode){

    //turn random mode off
    random_mode = 0;
    random_modestore = 1; // store the value of random mode to restore on exit of timer
  }
    else 
    {
      random_modestore = 0;
    }

  //run clock main loop as long as run_mode returns true
  while (run_mode()){


    //check buttons 
    if(buttonA.uniquePress()){
      //Brett resets timer on press of button A disable if you want timer to continue in other modes
      timerEnable = 0; // stops timer when current time = stop time
      timerValue = 0; //resets timerValue back to 0sec
      //Brett resets timer on exit disable if you want timer to continue in other modes
      fade_down();
      clock_mode = 0;
      return;      
    }

// Brett 
// Quick Brightness control button D
   if(buttonbrightness.uniquePress())
    {
      fade_down();
      set_brightness();
       return;
      
         }
//Brett


    
 //***********************************************************
  //Brett set timer forwards
if(buttonSetTime.uniquePress()) //sets time for timer
{
 if ( timerSetEnable == 0 )
 {

  timerSetEnable = 1;
} 
  tmrStep = 30;
  timerValue = tmrStep + timerValue; // add 30 secs to timer value each time buttonSetTime is pressed
  
  if( timerValue > 3599)// if timer goes over 59mins 59 secs timer goes to zero
  {
  timerValue = 0;
}
 
 
 // Serial.print("timerValue ");
//  Serial.println(timerValue);
}
     
 //************************************************************ 
 //Brett set timer backwards
if(buttonB.uniquePress()) //sets time for timer
{
 if ( timerSetEnable == 0 )
 {

  timerSetEnable = 1;
} 
  tmrStep = 30;
  timerValue = timerValue - tmrStep; // take 30 secs off timer value each time buttonB is pressed
   
   if( timerValue < 0)// if timer goes less than 0 timer goes to 59mins 30sec
  {
  timerValue = 3570;
  delay (100);
}

  //Serial.print("timerValue ");
 // Serial.println(timerValue);
}





    
 //***********************************************************
  //Brett trigger timer
if(buttonStTmr.uniquePress()) 
{
 if ( timerEnable == 0 )
 {
 start = rtc[2]*3600 + rtc[1]* 60+ rtc[0]; // stores time of timer start
 stopTime = start + timerValue; //this is the time the timer will start at in seconds

 timerEnable = 1; // disable/enables timer if buttonStTmr is pressed
}

}

 // Brett correction if timer set to stop after midnight
 if ( stopTime > 20863 )
 {
  stopTime = timerValue- (20863- start);
  //timeRemain = stopTime + 20863 - (start - timeNow); // this is the time remaining on the timer
 }
// end correction if timer set to stop after midnight

 

 
 // displays time remaining on timer only when  buttonStTmr is pressed

 timeNow = rtc[2]*3600 + rtc[1] * 60 + rtc[0]; //current time to be taken off stopTime to get timeRemain
 timeRemain = stopTime - timeNow; // this is the time remaining on the timer
 
//Brett fix to show time remaining if timer goes across midnight
if ( timeRemain < 0 )
{
   timeRemain = (20863 - timeNow) + stopTime ;
}


 
// end fix to show time remaining if timer goes across midnight



 rmn3[0] = timeRemain % 60;
 timeRemain /= 60;              // Same as timeRemain = timeRemain / 60
 rmn3[1] = timeRemain % 60;
 timeRemain /= 60;
 rmn3[2] = timeRemain;




 
// start display timerValue on display when buttonSetTime pressed until buttonStTmr is pressed then 
  if (timerSetEnable == 1 && timerEnable ==0 ) // buttonSetTime has been pressed
{
timerVal = timerValue;  
rmn3[0] = timerVal % 60;
 timerVal /= 60;              // Same as timerVal = timerVal / 60
 rmn3[1] = timerVal % 60;
 timerVal /= 60;
 rmn3[2] = timerVal;
  
}
 
// end display timerValue on display until timer starts
/*
  Serial.print("timerSetEnable ");
  Serial.println(timerSetEnable);

  Serial.print("timerEnable ");
  Serial.println(timerEnable);
*/
// below timerEnable == 1 this stops reset of timer if display reaches 0 in setting timer mode
// below timeNow ==0 resets timer at midnight to stop timer errors if timer is set before and runs after midnight need fixing
  if (rmn3[1] == 0 && rmn3[0] == 0 && timerEnable == 1 )// when timer reaches zero or if time reaches midnight
{
timerEnable = 0; // stops timer when current time = stop time


 timerValue = 0; //resets timerValue back to 0sec
 start = 0; //resets start time to 0

//if random mode was on before timer mode, turn it back on
  if (random_modestore){

     //turn randome mode back on. 
    random_mode = 1;
    random_modestore = 0;
    
  }
    

 
  
  
  digitalWrite(tmrOutput, LOW); // turns on timer output to alarm sounder
  delay (250);
  digitalWrite(tmrOutput, HIGH);// turns on timer output to alarm sounder

  
  
 
 //Brett enable to jump back to slide mode after timer ends
  clock_mode = 0;
  fade_down();
 return; //exit the mode back to loop()
 //Brett enable to jump back to slide mode after timer ends
} 


     
 //************************************************************ 
  
    
    get_time();

    //check buttons 
    if(buttonA.uniquePress()){


      
      timeRemain = 0; // resets time remain on exit
      timerValue = 0; //resets timerValue back to 0sec
      rmn3[0] = 0;
       rmn3[1] = 0;
       rmn3[2] = 0;
       start = 0; //resets start time to 0
      switch_mode();
      return;      
    }
    
    //Brett button B display date removed in Normal/timer mode but steps timer back instead
    /*
    if(buttonB.uniquePress()){
      display_date();
      fade_down();
      return;
    }
   */

   
    //if secs changed then update them on the display
    secs = rtc[0];
    if (secs != old_secs){

      //secs
      char buffer[3];
      itoa(secs,buffer,10);

      //fix - as otherwise if num has leading zero, e.g. "03" secs, itoa coverts this to chars with space "3 ". 
      if (secs < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }

      ht1632_putchar( 30, 0, ':'); //seconds colon
      ht1632_putchar( 36, 0, buffer[0]); //seconds
     ht1632_putchar( 42, 0, buffer[1]); //seconds
      old_secs = secs;
    }

    //if minute changes change time
    if (mins != rtc[1]) {  

      //reset these for comparison next time 
      mins = rtc[1];
      byte hours = rtc[2];   
      if (hours > 12) {
        hours = hours - ampm * 12;
      }
      if (hours < 1) {
        hours = hours + ampm * 12;
      }

      byte dow  = rtc[3]; // the DS1307 outputs 0 - 6 where 0 = Sunday0 - 6 where 0 = Sunday. 
      byte date = rtc[4];

      //set characters
      char buffer[3];
      itoa(hours,buffer,10);

      //fix - as otherwise if num has leading zero, e.g. "03" hours, itoa coverts this to chars with space "3 ". 
      if (hours < 10) {
        buffer[1] = buffer[0];
        //if we are in 12 hour mode blank the leading zero.
        if (ampm) {
          buffer[0] = ' ';
        }
        else{
          buffer[0] = '0';
        }
      }
      //set hours chars
      textchar[0] = buffer[0];
      textchar[1] = buffer[1];
      textchar[2] = ':';

      itoa (mins, buffer, 10);
      if (mins < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }

   
      
      //set mins characters
      textchar[3] = buffer[0];
      textchar[4] = buffer[1];

      //do seconds
      textchar[5] = ':';
      buffer[3];
      secs = rtc[0];
      itoa(secs,buffer,10);

      //fix - as otherwise if num has leading zero, e.g. "03" secs, itoa coverts this to chars with space "3 ". 
      if (secs < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }
      //set seconds
      textchar[6] = buffer[0];
      textchar[7] = buffer[1];


   
      
      byte x = 0;
      //byte y = 0;

      //print each char 
      for(byte x=0; x<8 ; x++) {
        ht1632_putchar( x * 6, 0, textchar[x]); //top line
     //  ht1632_putchar( y * 6, 8, textchar[y+8]); //bottom line  

      
      }
     }// Brett  ******************************* removed from line 1839

//***************************************************************************
// Brett show time remain on bottom line
  //if timer secs changed then update them on the display
  
    secsTmr = rmn3[0];
    if (secsTmr != old_secsTmr){

      //Timer secs
      char buffer[3];
      itoa(secsTmr,buffer,10);

      //fix - as otherwise if num has leading zero, e.g. "03" secs, itoa coverts this to chars with space "3 ". 
      if (secsTmr < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }
      // char position top row 0-8 bot row 9-15 times pos by 6 to get putchar location 2nd is 0 top 8 bot
   //   ht1632_putchar( 78, 8, ':'); //seconds colon
      ht1632_putchar( 84, 8, buffer[0]); //seconds 
     ht1632_putchar( 90, 8, buffer[1]); //seconds
      old_secsTmr = secsTmr;
    }

    //if minute changes change time
    if (minsTmr != rmn3[1]) {  

      //reset these for comparison next time 
      minsTmr = rmn3[1];
    }


      //set characters
     char buffer[3];
      itoa (minsTmr, buffer, 10);
      if (minsTmr < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }

    // Brett zeroes tmr mins if timer not running
     if ( timerEnable == 0 && timerSetEnable == 0 )
    {      
    buffer[1] = '0';
    buffer[0] = '0';
    }
      
      //set mins characters
      textchar[11] = buffer[0];
      textchar[12] = buffer[1];




    
     //do Timer secs
      textchar[13] = ':';
      buffer[3];
      secsTmr = rmn3[0];
      itoa(secsTmr,buffer,10);

      
    
       



        //fix - as otherwise if num has leading zero, e.g. "03" secs, itoa coverts this to chars with space "3 ". 
      if (secsTmr < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }

       // Brett zeroes tmr seconds if timer not running
     if ( timerEnable == 0 && timerSetEnable == 0 )
    {      
    buffer[1] = '0';
    buffer[0] = '0';
    }
     
       // Brett test tmr zero
      //set timer seconds
      textchar[14] = buffer[0];
      textchar[15] = buffer[1];  

    
 //      Serial.print("secsTmr ");
 // Serial.println(secsTmr);
    //}
      //set bottom line
     textchar[8] = 'T';
     textchar[9] = 'm';
     textchar[10] = 'r';
   //   textchar[8] = ' ';
   //   textchar[9] = ' ';
   //   textchar[10] = ' ';
   //   textchar[11] = ' '; // mins 10s
   //   textchar[12] = buffer[0]; // mins 1s
   //   textchar[13] = ':'; // mins secs colon
   //   textchar[14] = rmn3[0]; // secs 10s
   //   textchar[15] = rmn3[0]; // secs 1s

     //  byte x = 0;
      byte y = 0;

      //print each char 
      for(byte y=0; y<8 ; y++) {
     //   ht1632_putchar( x * 6, 0, textchar[x]); //top line
       ht1632_putchar( y * 6, 8, textchar[y+8]); //bottom line

        
      } 
   // } 
    delay(50);
  } 
  fade_down();
}


//like normal but with slide effect
void slide() {
//Brett turns random mode on if on before and returns random mode store to 0

if (random_modestore == 1)
{
random_mode = random_modestore;
    
}
//Brett end turns random mode on if on before

  
//Top row time
  byte digits_old[6] = {
    99,99,99,99,99,99    }; //old values  we store time in. Set to somthing that will never match the time initially so all digits get drawn wnen the mode starts
  byte digits_new[6]; //new digits time will slide to reveal
  
  byte digits_x_pos[6] = {
     42,36,24,18,6,0    }; //x pos for which to draw each digit at - last 2 are bottom line
 
 //Bottom Row Temperature
 char celsius_old[6] = {
    0,0,0,0,0,0}; //old values  we store time in. Set to somthing that will never match the time initially so all digits get drawn wnen the mode starts
  celsiusStr[5]; //new digits time will slide to reveal
  
  byte celsius_x_pos[6] = {6,12,20,24,30}; //x pos for which to draw each digit at - last 2 are bottom line
 
 
  char old_char[2]; //used when we use itoa to transpose the current digit (type byte) into a char to pass to the animation function
  char new_char[2]; //used when we use itoa to transpose the new digit (type byte) into a char to pass to the animation function

  //old_chars - stores the 5 day and date suffix chars on the display. e.g. "mon" and "st". We feed these into the slide animation as the current char when these chars are updated. 
  //We sent them as A initially, which are used when the clocl enters the mode and no last chars are stored.
//  char old_chars[6] = "00000";

  //plot the clock colons on the display - these don't change
  cls();
  ht1632_putchar( 12, 0, ':'); 
  ht1632_putchar( 30, 0, ':'); 
  //ht1632_putchar( 20, 8, '.'); //bott row
  ht1632_putchar( 0, 8, ' '); //bott row
  
  ht1632_putchar( 42, 8, 'C'); //bott row

  //Draw box for degree symbol
  for (byte x=38; x<40; x++){
    plot(x,8,38);
    plot(x,9,41);
  }

  //*********************




  
  byte old_secs = rtc[0]; //store seconds in old_secs. We compare secs and old secs. WHen they are different we redraw the display

  //run clock main loop as long as run_mode returns true
  while (run_mode()){ 

    get_time();
  
    //check buttons 
    if(buttonA.uniquePress()){
      switch_mode();
      return;      
    }
    if(buttonB.uniquePress()){
      display_date();
      fade_down();
      return;
    }
  //Brett select timer mode from any other mode by pressing buttonStTmr
  if(buttonStTmr.uniquePress()){
      //void normal();
      clock_mode = 4;
     // fade_down();
      return; //exit the mode back to loop(), Then we reenter the function and the clock mode starts again. This refreshes the display
    }
//Brett end select timer mode from any other mode by pressing buttonStTmr

// Brett 
// Quick Brightness control button D
   if(buttonbrightness.uniquePress())
    {
      fade_down();
      set_brightness();
       return;
      
         }
//Brett
    //if secs have changed then update the display
    if (rtc[0] != old_secs){
      old_secs = rtc[0];

//Brett*******************************************************

//Serial.println("wake up MCP9808.... "); // wake up MSP9808 - power consumption ~200 mikro Ampere

  tempsensor.shutdown_wake(0);   // Don't remove this line! required before reading temp
 //tempsensor.begin(0x18); // loads the  MSP9808 default address
  // Read and print out the temperature, convert to *F not required
  c = tempsensor.readTempC();
 // Brett convert to f removed
 // float f = c * 9.0 / 5.0 + 32;
//  Serial.print("Temp: "); Serial.print(c);Serial.println("*C");
 // Serial.print(f); Serial.println("*F");
  delay(250); // Brett don't remove
  
 // Serial.println("Shutdown MCP9808.... ");
  tempsensor.shutdown_wake(1); // shutdown MSP9808 - power consumption ~0.1 mikro Ampere


 //Brett*****************************************************  


//Temperature
// convert temperatur sensor value to string  
 dtostrf(c, 5, 2, celsiusStr); //dtostrf(floatvar, StringLengthIncDecimalPoint, numVarsAfterDecimal, charbuf);
/*
 floatvar float variable
StringLengthIncDecimalPoint This is the length of the string that will be created
numVarsAfterDecimal The number of digits after the deimal point to print
charbuf the array to store the results

*/
/*
 Serial.print("Celsius String0 ");
  Serial.println(celsiusStr[0]);

Serial.print("Celsius String1 ");
  Serial.println(celsiusStr[1]);
  

Serial.print("Celsius String3 ");
  Serial.println(celsiusStr[3]);

Serial.print("Celsius String4 ");
  Serial.println(celsiusStr[4]);
  

 */
 
  
//Brett*****************************************************  



    

      //do 12/24 hour conversion if ampm set to 1
      byte hours = rtc[2];
      if (hours > 12) {
        hours = hours - ampm * 12;
      }
      if (hours < 1) {
        hours = hours + ampm * 12;
      }

      //split all date and time into individual digits - stick in digits_new array

      //rtc[0] = secs                        //array pos and digit stored
      digits_new[0] = (rtc[0]%10);           //0 - secs ones
      digits_new[1] = ((rtc[0]/10)%10);      //1 - secs tens
      //rtc[1] = mins
      digits_new[2] = (rtc[1]%10);           //2 - mins ones
      digits_new[3] = ((rtc[1]/10)%10);      //3 - mins tens
      //rtc[2] = hours
      digits_new[4] = (hours%10);           //4 - hour ones
      digits_new[5] = ((hours/10)%10);      //5 - hour tens
    /*
      //rtc[4] = date
     digits_new[6] = celsiusStr[0];
     
    digits_new[7] = celsiusStr[1];
    
     digits_new[8] = celsiusStr[2];      //7 - date tens

   digits_new[9] = celsiusStr[3];      //7 - date tens
     */

      //draw initial screen of all chars. After this we just draw the changes.

      //compare top line digits 0 to 5 (secs, mins and hours)
      for (byte i = 0; i <= 5; i++){
        //see if digit has changed...
        if (digits_old[i] != digits_new[i]) {

          //run sequence for each in turn
          for (byte seq = 0; seq <=8 ; seq++){

            //convert digit to string 
            itoa(digits_old[i],old_char,10);
            itoa(digits_new[i],new_char,10);

            //if set to 12 hour mode and we're on digit 5 (hours tens mode) then check to see if this is a zero. If it is, blank it instead so we get 2.00pm not 02.00pm
            if (ampm && i==5){
              if (digits_new[5] == 0) { 
                new_char[0] = ' ';           
              } 
              if (digits_old[5] == 0) { 
                old_char[0] = ' '; 
              }
            }
            //draw the animation frame for each digit
            slideanim(digits_x_pos[i],0,seq,old_char[0],new_char[0]);  
            delay(SLIDE_DELAY);
            
          }
        }
      }

    

      //save digita array to old for comparison next loop
      for (byte i = 0; i <= 6; i++){
        digits_old[i] =  digits_new[i];
        
      }



//*************************************
//Temperature
  
  if ( digits_new[0] == 0 )//check for temperature change every 10 seconds (when 1 seconds digit is 0)
  {
 //draw initial screen of all chars. After this we just draw the changes.
 //compare top line digits 0 to 5 (secs, mins and hours)
       for (char i = 0; i <=4 ; i++){
        //see if digit has changed from last 10 seconds 
        if (celsius_old[i] != celsiusStr[i]) {
  
 
          //run sequence for each in turn
          for (byte seq = 0; seq <=8 ; seq++){

          // itoa not required as already a string

         
            //draw the animation frame for each digit
            slideanim(celsius_x_pos[i],8,seq,celsius_old[i],celsiusStr[i]);  
            delay(SLIDE_DELAY);
           
          }
        }
      }

    

      //save digita array to old for comparison next loop
      for (char i = 0; i <= 4; i++){
        celsius_old[i] =  celsiusStr[i];
      
  
       
      }

  }





//*************************************


 
    }//secs/oldsecs
  }//while loop
  fade_down();
}    


//called by slide
//this draws the animation of one char sliding on and the other sliding off. There are 8 steps in the animation, we call the function to draw one of the steps from 0-7
//inputs are are char x and y, animation frame sequence (0-7) and the current and new chars being drawn.
void slideanim(byte x, byte y, byte sequence, char current_c, char new_c) {

  //  To slide one char off and another on we need 9 steps or frames in sequence...

  //  seq# 0123456 <-rows of the display
  //   |   |||||||
  //  seq0 0123456  START - all rows of the display 0-6 show the current characters rows 0-6
  //  seq1  012345  current char moves down one row on the display. We only see it's rows 0-5. There are at display positions 1-6 There is a blank row inserted at the top
  //  seq2 6 01234  current char moves down 2 rows. we now only see rows 0-4 at display rows 2-6 on the display. Row 1 of the display is blank. Row 0 shows row 6 of the new char
  //  seq3 56 0123  
  //  seq4 456 012  half old / half new char
  //  seq5 3456 01 
  //  seq6 23456 0
  //  seq7 123456 
  //  seq8 0123456  END - all rows show the new char

  //from above we can see...
  //currentchar runs 0-6 then 0-5 then 0-4 all the way to 0. starting Y position increases by 1 row each time. 
  //new char runs 6 then 5-6 then 4-6 then 3-6. starting Y position increases by 1 row each time. 

  //if sequence number is below 7, we need to draw the current char
  if (sequence <7){
    byte dots;
    // if (current_c >= 'A' &&  || (current_c >= 'a' && current_c <= 'z') ) {
    //   current_c &= 0x1F;   // A-Z maps to 1-26
    // } 
    if (current_c >= 'A' && current_c <= 'Z' ) {
      current_c &= 0x1F;   // A-Z maps to 1-26
    } 
    else if (current_c >= 'a' && current_c <= 'z') {
      current_c = (current_c - 'a') + 41;   // A-Z maps to 41-67
    } 
    else if (current_c >= '0' && current_c <= '9') {
      current_c = (current_c - '0') + 31;
    }     
    else if (current_c == ' ') {
      current_c = 0; // space
    }
    else if (current_c == '.') {
      current_c = 27; // full stop
    }
    else if (current_c == '\'') {
      current_c = 28; // single quote mark
    }  
    else if (current_c == ':') {
      current_c = 29; //colon
    }
    else if (current_c == '>') {
      current_c = 30; // clock_mode selector arrow
    }

    byte curr_char_row_max=6-sequence; //the maximum number of rows to draw is 6 - sequence number
    byte start_y = sequence; //y position to start at - is same as sequence number. We inc this each loop

    //plot each row up to row maximum (calculated from sequence number)
    for (byte curr_char_row = 0; curr_char_row <= curr_char_row_max; curr_char_row++){       
      for (byte col=0; col< 5; col++) {
        dots = pgm_read_byte_near(&myfont[current_c][col]);
        if (dots & (64>>curr_char_row))   	    
          plot(x+col, y + start_y, 1); //plot led on
        else 
          plot(x+col, y + start_y, 0);  //else plot led off
      }
      start_y++;//add one to y so we draw next row one down
    }
  }

  //draw a blank line between the characters if sequence is between 1 and 7. If we don't do this we get the remnants of the current chars last position left on the display
  if (sequence >= 1 && sequence <= 7){
    for (byte col=0; col< 5; col++) { 
      plot(x+col, y+ (sequence - 1), 0); //the y position to draw the line is equivalent to the sequence number - 1
    }
  }

  //if sequence is above 2, we also need to start drawing the new char
  if (sequence >= 2){

    //work out char   
    byte dots;
    //if (new_c >= 'A' && new_c <= 'Z' || (new_c >= 'a' && new_c <= 'z') ) {
    //  new_c &= 0x1F;   // A-Z maps to 1-26
    //}
    if (new_c >= 'A' && new_c <= 'Z' ) {
      new_c &= 0x1F;   // A-Z maps to 1-26
    } 
    else if (new_c >= 'a' && new_c <= 'z') {
      new_c = (new_c - 'a') + 41;   // A-Z maps to 41-67
    }  
    else if (new_c >= '0' && new_c <= '9') {
      new_c = (new_c - '0') + 31;
    } 
    else if (new_c == ' ') {
      new_c = 0; // space
    }
    else if (new_c == '.') {
      new_c = 27; // full stop
    }
    else if (new_c == '\'') {
      new_c = 28; // single quote mark
    }  
    else if (new_c == ':') {
      new_c = 29; // clock_mode selector arrow
    }
    else if (new_c == '>') {
      new_c = 30; // clock_mode selector arrow
    }

    byte newcharrowmin = 6 - (sequence-2); //minimumm row num to draw for new char - this generates an output of 6 to 0 when fed sequence numbers 2-8. This is the minimum row to draw for the new char
    byte start_y = 0; //y position to start at - is same as sequence number. we inc it each row

    //plot each row up from row minimum (calculated by sequence number) up to 6
    for (byte newcharrow = newcharrowmin; newcharrow <= 6; newcharrow++){ 
      for (byte col=0; col< 5; col++) {
        dots = pgm_read_byte_near(&myfont[new_c][col]);
        if (dots & (64>>newcharrow))   	     
          plot(x+col, y + start_y, 1); //plot led on
        else 
          plot(x+col, y + start_y, 0); //else plot led off
      }
      start_y++;//add one to y so we draw next row one down
    }
  } 
}


//if the minute matches next_display_date then display the date, and set the next minute the date should be displayed
bool check_show_date(){

  if (rtc[1] == next_display_date){
    fade_down();
    display_date();
    fade_down();
    set_next_date();
    return 1; //return true - the date was shown so run whatever setup you need (e.g redraw the pong pitch)
  } 
  else {
    return 0; //the date wasn't shown.
  }

}



//set the next minute the date should be automatically displayed
void set_next_date(){

  get_time();

  next_display_date = rtc[1] + random(NEXT_DATE_MIN, NEXT_DATE_MAX); 
  //check we've not gone over 59
  if (next_display_date >= 59) {
    next_display_date = random(NEXT_DATE_MIN, NEXT_DATE_MAX);
  } 
}


//display_date - print the day of week, date and month with a flashing cursor effect
void display_date()
{

  cls();
  //read the date from the DS1307
 
  byte dow = rtc[3]; // day of week 0 = Sunday
  byte date = rtc[4];
  byte month = rtc[5] - 1; 

  //array of month names to print on the display. Some are shortened as we only have 8 characters across to play with 
  char monthnames[12][9]={
    "January", "February", "March", "April", "May", "June", "July", "August", "Sept", "October", "November", "December"    };

  //call the flashing cursor effect for one blink at x,y pos 0,0, height 5, width 7, repeats 1
  flashing_cursor(0,0,5,7,1);

  //print the day name
  int i = 0;
  while(daysfull[dow][i])
  {
    flashing_cursor(i*6,0,5,7,0);
    ht1632_putchar(i*6 , 0, daysfull[dow][i]); 
    i++;

    //check for button press and exit if there is one.
    if(buttonA.uniquePress() || buttonB.uniquePress()){
      return;
    }
  }

  //pause at the end of the line with a flashing cursor if there is space to print it.
  //if there is no space left, dont print the cursor, just wait.
  if (i*6 < 48){
    flashing_cursor(i*6,0,5,7,1);  
  } 
  else {
    delay(300);
  }

  //flash the cursor on the next line  
  flashing_cursor(0,8,5,7,0);

  //print the date on the next line: First convert the date number to chars so we can print it with ht1632_putchar
  char buffer[3];
  itoa(date,buffer,10);

  //then work out date 2 letter suffix - eg st, nd, rd, th etc
  // char suffix[4][3]={"st", "nd", "rd", "th"  }; define at top of code
  byte s = 3; 
  if(date == 1 || date == 21 || date == 31) {
    s = 0;
  } 
  else if (date == 2 || date == 22) {
    s = 1;
  } 
  else if (date == 3 || date == 23) {
    s = 2;
  } 

  //print the 1st date number
  ht1632_putchar(0, 8, buffer[0]);

  //if date is under 10 - then we only have 1 digit so set positions of sufix etc one character nearer
  byte suffixposx = 6;

  //if date over 9 then print second number and set xpos of suffix to be 1 char further away
  if (date > 9){
    suffixposx = 12;
    flashing_cursor(6,8,5,7,0); 
    ht1632_putchar(6, 8, buffer[1]);
  }

  //print the 2 suffix characters
  flashing_cursor(suffixposx, 8,5,7,0);
  ht1632_putchar(suffixposx, 8, suffix[s][0]); 
  //delay(70);

  flashing_cursor(suffixposx+6,8,5,7,0);
  ht1632_putchar(suffixposx+6, 8, suffix[s][1]); 
  //delay(70);

  //blink cursor after 
  flashing_cursor(suffixposx + 12,8,5,7,1);  

  //replace day name with date on top line - effectively scroll the bottom line up by 8 pixels
  cls(); 

  ht1632_putchar(0, 0, buffer[0]);  //date first digit
  ht1632_putchar(6, 0, buffer[1]);  //date second digit - this may be blank and overwritten if the date is a single number
  ht1632_putchar(suffixposx, 0, suffix[s][0]);   //date suffix
  ht1632_putchar(suffixposx+6, 0, suffix[s][1]);   //date suffix


  //flash the cursor for a second for effect
  flashing_cursor(suffixposx + 12,0,5,7,0);  

  //print the month name on the bottom row
  i = 0;
  while(monthnames[month][i])
  {  
    flashing_cursor(i*6,8,5,7,0);
    ht1632_putchar(i*6, 8, monthnames[month][i]); 
    i++; 

    //check for button press and exit if there is one.
    if(buttonA.uniquePress() || buttonB.uniquePress()){
      return;
    }
  }

  //blink the cursor at end if enough space after the month name, otherwise juts wait a while
  if (i*6 < 48){
    flashing_cursor(i*6,8,5,7,2);  
  } 
  else {
    delay(1000);
  }
  delay(3000);
}

// display a horizontal bar on the screen at offset xbar by ybar with height and width of xbar, ybar
void levelbar (byte xpos, byte ypos, byte xbar, byte ybar) {
  for (byte x = 0; x <= xbar; x++) {
    for (byte y = 0; y <= ybar; y++) {
      plot(x+xpos, y+ypos, 1);
    }
  }
}

//flashing_cursor
//print a flashing_cursor at xpos, ypos and flash it repeats times 
void flashing_cursor(byte xpos, byte ypos, byte cursor_width, byte cursor_height, byte repeats)
{
  for (byte r = 0; r <= repeats; r++) {    
    for (byte x = 0; x <= cursor_width; x++) {
      for (byte y = 0; y <= cursor_height; y++) {
        plot(x+xpos, y+ypos, 1);
      }
    }

    if (repeats > 0) {
      delay(400);
    } 
    else {
      delay(70);
    }

    for (byte x = 0; x <= cursor_width; x++) {
      for (byte y = 0; y <= cursor_height; y++) {
        plot(x+xpos, y+ypos, 0);
      }
    }   
    //if cursor set to repeat, wait a while
    if (repeats > 0) {
      delay(400); 
    }
  }
}



//************ MENU & MISC ROUTINES ******************

// button_delay
// like regular delay but can be quit by a button press

void button_delay(int wait) {
  int i = 0;
  while ( i < wait){
    //check if a button is pressed, if it is, quit waiting
    if(buttonA.uniquePress() || buttonB.uniquePress()){
      return;
    }
    //else wait a moment
    delay (1);
    i++;
  }
}


//dislpay menu to change the clock mode
void switch_mode() {

 

  //remember mode we are in. We use this value if we go into settings mode, so we can change back from settings mode (6) to whatever mode we were in.
  old_mode = clock_mode;  

  char* modes[] = {
    "Slide", "Pong", "Digits", "Words", "Timer", "Setup"};
 
  byte next_clock_mode;
  byte firstrun = 1;

  //loop waiting for button (timeout after 35 loops to return to mode X)
  for(int count=0; count < 35 ; count++) {

    //if user hits button, change the clock_mode
    if(buttonA.uniquePress() || firstrun == 1){

      count = 0;
      cls();

      if (firstrun == 0) { 
        clock_mode++; 
      } 
      if (clock_mode > NUM_DISPLAY_MODES +1 ) { 
        clock_mode = 0; 
      }

      //print arrown and current clock_mode name on line one and print next clock_mode name on line two
      char str_top[9];
      char str_bot[9];

      strcpy (str_top, ">");
      strcat (str_top, modes[clock_mode]);

      next_clock_mode = clock_mode + 1;
      if (next_clock_mode >  NUM_DISPLAY_MODES +1 ) { 
        next_clock_mode = 0; 
      }

      strcpy (str_bot, " ");
      strcat (str_bot, modes[next_clock_mode]);

      byte i = 0;
      while(str_top[i]) {
        ht1632_putchar(i*6, 0, str_top[i]); 
        i++;
      }

      i = 0;
      while(str_bot[i]) {
        ht1632_putchar(i*6, 8, str_bot[i]); 
        i++;
      }
      firstrun = 0;
    }
    delay(50); 
  }
}



//dislpay menu to change the clock settings
void setup_menu() {

  char* set_modes[] = {
     "Go Back", "Random", "24 Hour","Set Clk", "Bright","DST Adj"}; 
  if (ampm == 0) { 
    set_modes[2] = ("12 Hour"); 
  }

  byte setting_mode = 0;
  byte next_setting_mode;
  byte firstrun = 1;

  //loop waiting for button (timeout after 35 loops to return to mode X)
  for(int count=0; count < 35 ; count++) {

    //if user hits button, change the clock_mode
    if(buttonA.uniquePress() || firstrun == 1){

      count = 0;
      cls();

      if (firstrun == 0) { 
        setting_mode++; 
      } 
      if (setting_mode > NUM_SETTINGS_MODES) { 
        setting_mode = 0; 
      }

      //print arrown and current clock_mode name on line one and print next clock_mode name on line two
      char str_top[9];
      char str_bot[9];

      strcpy (str_top, ">");
      strcat (str_top, set_modes[setting_mode]);

      next_setting_mode = setting_mode + 1;
      if (next_setting_mode > NUM_SETTINGS_MODES) { 
        next_setting_mode = 0; 
      }

      strcpy (str_bot, " ");
      strcat (str_bot,  set_modes[next_setting_mode]);
      
      byte i = 0;
      while(str_top[i]) {
        ht1632_putchar(i*6, 0, str_top[i]); 
        i++;
      }

      i = 0;
      while(str_bot[i]) {
        ht1632_putchar(i*6, 8, str_bot[i]); 
        i++;
      }
      firstrun = 0;
    }
    delay(50); 
  }
  
  //pick the mode 
  switch(setting_mode){
    case 0: 
      //exit 
      break;
    case 1: 
      set_random(); 
      break;
    case 2: 
      set_ampm(); 
      break;
    case 3: 
      set_time(); 
      break;
    case 4: 
      set_brightness(); 
      break;
    case 5: 
      set_dst();
      break; 
  }
    
  //change the clock from mode 6 (settings) back to the one it was in before 
  clock_mode=old_mode;
}


//add or remove an hour for daylight savings
void set_dst(){
  cls();

  char text_a[9] = "Daylight";
  char text_b[9] = "Mode On ";
  char text_c[9] = "Mode Off";
   
  byte i = 0;

  get_time();
  byte hr = rtc[2];

  //if daylight mode if on, turn it off
  if (daylight_mode){

    //turn random mode off
    daylight_mode = 0;

    //take one off the hour
    if (hr>0){
      hr--;
    } else {
      hr = 23;
    }

    //print a message on the display
    while(text_a[i]) {
      ht1632_putchar((i*6), 0, text_a[i]);
      ht1632_putchar((i*6), 8, text_c[i]);
      i++;
    }
  } else {
    //turn daylight mode on. 
    daylight_mode = 1;
   
    //add one to the hour
    if (hr<23){
      hr++;
    } else {
      hr = 0;
    }
  
    //print a message on the display
    while(text_a[i]) {
      ht1632_putchar((i*6), 0, text_a[i]);
      ht1632_putchar((i*6), 8, text_b[i]);
      i++;
    }
  }
 
  //write the change to the clock chip
  DateTime now = ds1307.now();
  ds1307.adjust(DateTime(now.year(), now.month(), now.day(), hr, now.minute(), now.second()));
   
  delay(1500); //leave the message up for a second or so
}

//run clock main loop as long as run_mode returns true
byte run_mode(){

 //if random mode is on... check if 1st mins char is 9 and secs are 00 
 //or if 1st mins char is 5 and sec are 00 then check which mode to use with set next Random
  if(random_mode){
    //if not Random then return false = i.e. exit mode.
    get_time();
  minutes2 = floor(rtc[1]/10); //gets 2nd mins char from rtc
    minutes1 = rtc[1] - (minutes2 * 10); //gets 1st mins char from rtc

//    Serial.print("mins ");
//  Serial.print(minutes1);

//  Serial.print(" rtc[0] ");
// Serial.println(rtc[0]);  

//  Serial.print(" Clock mode");
 
//  Serial.println(clock_mode);

  // check if 1st mins char is 8 and secs are 50 or if 1st mins char is 5 and sec are 10 then check which mode to use with set next Random
    if (minutes1 == 8 && rtc[0] == 50 || minutes1 == 5 && rtc[0] == 10) 
    {
      //set the next random clock mode 
      set_next_random();
       
      //exit the current mode if not set to Random
      return 0;
    }
  }
  
  //else return 1 - keep running in this mode
  return 1;

}


 




//toggle random mode - pick a different clock mode every few hours
void set_random(){
  cls();

  char text_a[9] = "Random  ";
  char text_b[9] = "Mode On ";
  char text_c[9] = "Mode Off";
  byte i = 0;

  //if random mode is on, turn it off
  if (random_mode){

    //turn random mode off
    random_mode = 0;
    random_modestore = 0;// make randommode store 0 when random off

    //print a message on the display
    while(text_a[i]) {
      ht1632_putchar((i*6), 0, text_a[i]);
      ht1632_putchar((i*6), 8, text_c[i]);
      i++;
    }
  } else {
    //turn randome mode on. 
    random_mode = 1;
    
    //set hour mode will change
 //   set_next_random();
  
    //print a message on the display
    while(text_a[i]) {
      ht1632_putchar((i*6), 0, text_a[i]);
      ht1632_putchar((i*6), 8, text_b[i]);
      i++;
    }  
  } 
  delay(1500); //leave the message up for a second or so
}


//check which mode to set Slide if 9 mins 00 secs or Pong if 5 mins 00 secs
void set_next_random(){
 //Brett
  //set the clock mode Slide if 1st mins digit is 8 and secs are 50 or Pong if 1st mins digit is 5 and secs are 10
  
    if ( minutes1 == 8 && rtc[0] == 50)
    {
      clock_mode = 0;//Slide mode
    }
      else 
      clock_mode = 1;//Pong Mode

 
 }  

 

//set 12 or 24 hour clock
void set_ampm() {

  // AM/PM or 24 hour clock mode - flip the bit (makes 0 into 1, or 1 into 0 for ampm mode)
  ampm = (ampm ^ 1);

  cls();

  //print confirmation message
  char text_a[8] = "24 Hour";
  char text_b[8] = "12 Hour";

  byte i = 0;

  if (ampm == 0){
    while(text_a[i]) {
      ht1632_putchar((i*6)+3 , 4, text_a[i]);
      i++;
    }
  } 
  else {
    while(text_a[i]) {
      ht1632_putchar((i*6)+3 , 4, text_b[i]);
      i++;
    } 
  }  
  delay(1500);
  cls();
}


//change screen brightness
void set_brightness() {

  cls();

  //print "brightness"
  byte i = 0;
  char text[7] = "Bright";
  while(text[i]) {
    ht1632_putchar((i*6)+6, 0, text[i]);
    i++;
  }

  //wait for button input
  while (!buttonA.uniquePress()) {

    

    levelbar (1,8,brightness*3,6);    //display the brightness level as a bar
    while (buttonB.isPressed()) {

      if(brightness == 15) { 
        brightness = 0;
        cls (); 
      } 
      else {
        brightness++; 
      }
      //print the new value 
      i = 0;
      while(text[i]) {
        ht1632_putchar((i*6)+6, 0, text[i]);
        i++;
      }
      levelbar (1,8,brightness*3,6);    //display the brightness level as a bar 
      ht1632_sendcmd(0, HT1632_CMD_PWM + brightness);    //immediately change the 
      ht1632_sendcmd(1, HT1632_CMD_PWM + brightness);    //brightness on both panels
      delay(150);
    }
  }
}


//power up led test & display software version number
void printver(){

  byte i = 0;
  char ver_top[16] = "Pong Clock";
 // char ver_lower[16] = "Version 5.3";
 char ver_lower[16] = "Brett v7.5";

  //test all leds.
  for (byte x=0; x<48; x++){
    for (byte y=0; y<16; y++){
      plot(x,y,1);
    }
  }
  delay(1000);
  fade_down();

  //draw box
  for (byte x=0; x<48; x++){
    plot(x,0,1);
    plot(x,15,1);
  }
  for (byte y=0; y<15; y++){
    plot(0,y,1);
    plot(47,y,1);
  }
  
  //top line
  while(ver_top[i]) {
    ht1632_puttinychar((i*4)+2, 2, ver_top[i]);
    delay(35);
    i++;
  }

  i=0;
  //bottom line
  while(ver_lower[i]) {
    ht1632_puttinychar((i*4)+2, 9, ver_lower[i]);
    delay(35);
    i++;
  }

  delay(4000);
  fade_down();
  // end version message
}




//************* DS1307 ROUTINES ****************


//set time and date routine
void set_time() {

  cls();

  //fill settings with current clock values read from clock
  get_time();
  byte set_min   = rtc[1];
  byte set_hr    = rtc[2];
  byte set_date  = rtc[4];
  byte set_mnth  = rtc[5];
  int  set_yr    = rtc[6]; 

  //Set function - we pass in: which 'set' message to show at top, current value, reset value, and rollover limit.
  set_date = set_value(2, set_date, 1, 31);
  set_mnth = set_value(3, set_mnth, 1, 12);
  set_yr   = set_value(4, set_yr, 2013, 2099);
  set_hr   = set_value(1, set_hr, 0, 23);
  set_min  = set_value(0, set_min, 0, 59);

  ds1307.adjust(DateTime(set_yr, set_mnth, set_date, set_hr, set_min));
  
  cls();
}


//used to set min, hr, date, month, year values. pass 
//message = which 'set' message to print, 
//current value = current value of property we are setting
//reset_value = what to reset value to if to rolls over. E.g. mins roll from 60 to 0, months from 12 to 1
//rollover limit = when value rolls over
int set_value(byte message, int current_value, int reset_value, int rollover_limit){

  cls();
  char messages[6][17]   = {
    "Set Mins", "Set Hour", "Set Day", "Set Mnth", "Set Year"};

  //Print "set xyz" top line
  byte i = 0;
  while(messages[message][i])
  {
    ht1632_putchar(i*6 , 0, messages[message][i]); 
    i++;
  }

  //print digits bottom line
  char buffer[5] = "    ";
  itoa(current_value,buffer,10);
  ht1632_putchar(0 , 8, buffer[0]); 
  ht1632_putchar(6 , 8, buffer[1]); 
  ht1632_putchar(12, 8, buffer[2]); 
  ht1632_putchar(18, 8, buffer[3]); 

  delay(300);
  //wait for button input
  while (!buttonA.uniquePress()) {

    while (buttonB.isPressed()){

      if(current_value < rollover_limit) { 
        current_value++;
      } 
      else {
        current_value = reset_value;
      }
      //print the new value
      itoa(current_value, buffer ,10);
      ht1632_putchar(0 , 8, buffer[0]); 
      ht1632_putchar(6 , 8, buffer[1]);
      ht1632_putchar(12, 8, buffer[2]);
      ht1632_putchar(18, 8, buffer[3]);
      
      //delay(150);
      delay(500);
    }
  }
  return current_value;
}


//Get the time from the DS1307 chip.
void get_time()
{
  //get time
  DateTime now = ds1307.now();
  //save time to array
  rtc[6] = now.year();
  rtc[5] = now.month();
  rtc[4] = now.day();
  rtc[3] = now.dayOfWeek(); //returns 0-6 where 0 = Sunday
  rtc[2] = now.hour();
  rtc[1] = now.minute();
  rtc[0] = now.second();    
  
  
  // ################ Added by Brett Oliver #############################################
  
  
  // rtc sync on 30seconds from master clock
  
  if ( rtc[0] == 24 )
  {
  syncset = 1; // allows sync pulse to set seconds when 1

  }
 
  
 if ( digitalRead (sync30sec) == HIGH && digitalRead (PIR) == HIGH ) // 30 sec pulse monitor LED is On if PIR motion detected
 {
 digitalWrite(LED30sec, HIGH); // 30 sec pulse monitor LED is On
 }
 else
 digitalWrite(LED30sec, LOW); // 30 sec pulse LED is Off
 
  
 if ( digitalRead (sync30sec) == HIGH && syncset == 1 && rtc[0] > 24 && rtc[0] < 36 && rtc[0] != 0 ) // resets RTC to 30 seconds on 30 sec sync pulse and only if RTC[0} is not 0 and is >24 and <36
{
  sync30secval = 30;
  DateTime now = ds1307.now();
 ds1307.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), now.minute(), sync30secval));
 syncset = 0; // resets sync set so sync disabled until seconds reach 0 again
 
 if (digitalRead (PIR) == HIGH )
 {
 digitalWrite(LEDsync, HIGH); // 30 sec sync'd LED is On to show clock was out of sync and is now synchronized
 }
 }
 else 
 if ( rtc[0] == 31 ) // LEDsync held on until secs ==31
 {
 digitalWrite(LEDsync, LOW); 
 }

/*

Serial.print (" sync30sec ");
   Serial.println (sync30sec);
  Serial.print (" sync30secval ");
   Serial.println (sync30secval);
  */
  
  //end rtc sync on 30 seconds
  
  
  
  // PIR control
  if ( digitalRead (PIR) == LOW )
  {
  
 
      ht1632_sendcmd(0, HT1632_CMD_LEDOFF);    //immediately turn the 
      ht1632_sendcmd(1, HT1632_CMD_LEDOFF);    //LEDs off for both panels
  
  //Serial.println("PIR Off");
  
  }
  else
  {
   
       ht1632_sendcmd(0, HT1632_CMD_LEDON);    //immediately turn the 
      ht1632_sendcmd(1, HT1632_CMD_LEDON);    //LEDs on for both panels

   //   Serial.println("PIR On");
  }
  // PIR control end
  /*
   // Quick Brightness control button D
   if(buttonbrightness.uniquePress())
    {
      //fade_down();
      set_brightness();
       
     
         }
*/

 /* 
 
  // Quick Brightness control button D
   if(buttonbrightness.uniquePress())
    {
     brightness = brightness + 3; // adjusts brightness in steps of 3 (5 in total)
      ht1632_sendcmd(0, HT1632_CMD_PWM + brightness);    //immediately change the 
      ht1632_sendcmd(1, HT1632_CMD_PWM + brightness);    //brightness on both panels
      if(brightness > 15)
     { 
        brightness = 0;
       
     }
         }
  */      
  //  Serial.print ("Brightness ");
 //  Serial.println (brightness);
 
 
 
  // End Quick Brightness control
  
  
  
  
  
  // Man 30 second sync
  
  
  //check to see if the buttons have been pressed    
    if(buttonSync.uniquePress())
    {
     sync30secval = 30;
  DateTime now = ds1307.now();
 ds1307.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), now.minute(), sync30secval));
 syncset = 0; // resets sync set so sync disabled until seconds reach 0 again
} 
 
 
 // End Man 30 second sync 
 
 
 
  
  //flash arduino led on pin 13 every second if PIR motion detected
  if ( (rtc[0] % 2) == 0 && digitalRead (PIR) == HIGH )
  { 
    digitalWrite(13,HIGH);
  }
  else{ 
    digitalWrite(13,LOW); 
  }
  
  // end flash arduino led on pin 13 every second if PIR motion detected
  
  // End ################ Added by Brett Oliver #############################################
  
  /*
 
  //print the time to the serial port.
  Serial.print(rtc[2]);
  Serial.print(":");
  Serial.print(rtc[1]);
  Serial.print(":");
  Serial.println(rtc[0]);
 */


 //print the time to the serial port.

  
  
 /*
 Serial.print("Stop time ");
  Serial.print(t3[2]);
  Serial.print(":");
  Serial.print(t3[1]);
  Serial.print(":");
  Serial.println(t3[0]);
*/

/*
Serial.print("Time Remain ");
  Serial.print(rmn3[1]);
  Serial.print(":");
  Serial.println(rmn3[0]);
  
  


 Serial.print("Time Now ");
  Serial.println(timeNow);

   Serial.print("stopTime ");
  Serial.println(stopTime);

Serial.print("start ");
  Serial.println(start);


  Serial.print("timerValue ");
  Serial.println(timerValue);
*/
  
/*
 Serial.print("random_mode ");
  Serial.println(random_mode);

   Serial.print("random_modestore ");
  Serial.println(random_modestore);

 */  



  



}






