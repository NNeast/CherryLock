  /****************************************************************************
   *                     EEPROM mapping for Cherry Lock                       *
   *                     ==============================                       *
   *                                                                          *
   *  EEPROM on Arduino UNO has 1024 bytes of space. In this section I will   *
   *  explain what belongs to where. First, let's start with basics.          *
   *                                                                          *
   *  Adress space of EEPROM is 0-1023 - each adress belonging to each byte.  *
   *  My notation is simple:                                                  *
   *                                                                          *
   *      )  [45] referes to adress 45,                                       *
   *      )  [45:4] referes to 32-bit word, starting from adress 45.          *
   *                                                                          *
   *  So, now when notation is explained - here's the mapping:                *
   *                                                                          *
   *      [0]     Number of access cards added. Used to determine where to    *
   *              search for card ID's.                                       *
   *      [1]     Contains 42, if the Master card is defined. If other value  *
   *              will be written, then after restart system will ask to      *
   *              define Master card.                                         *
   *      [2:4]   Master card ID.                                             *
   *                                                                          *
   *      [6:4]                                                               *
   *      [10:4]                                                              *
   *      [14:4]                                                              *
   *         .                                                                *
   *         .    List of access cards' ID's. There's space for 253 cards.    *
   *         .                                                                *
   *      [1010:4]                                                            *
   *      [1014:4]                                                            *
   *      [1018:4]                                                            *
   *                                                                          *
   ****************************************************************************/

/*
   --------------------------------------------------------------------------------------------------------------------
   Example sketch/program showing An Arduino Door Access Control featuring RFID, EEPROM, Relay
   --------------------------------------------------------------------------------------------------------------------
   This is a MFRC522 library example; for further details and other examples see: https://github.com/miguelbalboa/rfid

   This example showing a complete Door Access Control System

  Simple Work Flow (not limited to) :
                                      +---------+
  +----------------------------------->READ TAGS+^------------------------------------------+
  |                              +--------------------+                                     |
  |                              |                    |                                     |
  |                              |                    |                                     |
  |                         +----v-----+        +-----v----+                                |
  |                         |MASTER TAG|        |OTHER TAGS|                                |
  |                         +--+-------+        ++-------------+                            |
  |                            |                 |             |                            |
  |                            |                 |             |                            |
  |                      +-----v---+        +----v----+   +----v------+                     |
  |         +------------+READ TAGS+---+    |KNOWN TAG|   |UNKNOWN TAG|                     |
  |         |            +-+-------+   |    +-----------+ +------------------+              |
  |         |              |           |                |                    |              |
  |    +----v-----+   +----v----+   +--v--------+     +-v----------+  +------v----+         |
  |    |MASTER TAG|   |KNOWN TAG|   |UNKNOWN TAG|     |GRANT ACCESS|  |DENY ACCESS|         |
  |    +----------+   +---+-----+   +-----+-----+     +-----+------+  +-----+-----+         |
  |                       |               |                 |               |               |
  |       +----+     +----v------+     +--v---+             |               +--------------->
  +-------+EXIT|     |DELETE FROM|     |ADD TO|             |                               |
          +----+     |  EEPROM   |     |EEPROM|             |                               |
                     +-----------+     +------+             +-------------------------------+


   Use a Master Card which is act as Programmer then you can able to choose card holders who will granted access or not

 * **Easy User Interface**

   Just one RFID tag needed whether Delete or Add Tags. You can choose to use Leds for output or Serial LCD module to inform users.

 * **Stores Information on EEPROM**

   Information stored on non volatile Arduino's EEPROM memory to preserve Users' tag and Master Card. No Information lost
   if power lost. EEPROM has unlimited Read cycle but roughly 100,000 limited Write cycle.

 * **Security**
   To keep it simple we are going to use Tag's Unique IDs. It's simple and not hacker proof.

   @license Released into the public domain.

   Typical pin layout used:
   -----------------------------------------------------------------------------------------
               MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
               Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
   Signal      Pin          Pin           Pin       Pin        Pin              Pin
   -----------------------------------------------------------------------------------------
   RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
   SPI SS      SDA(SS)      10            53        D10        10               10
   SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
   SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
   SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
*/


/* CONFIG SECTION
 *  
 * Here you can configure the features you want in the compiled version.
 * To disable the option, just comment it. To enable, uncomment.
 *  
 */

#define SERIAL_DEBUG
//#define WIPE_BUTTON_ACTIVE
//#define WORK_LED_ACTIVE
//#define COMMON_ANODE


#ifdef WORK_LED_ACTIVE
  #ifdef COMMON_ANODE
    #define LED_ON LOW
    #define LED_OFF HIGH
  #else
    #define LED_ON HIGH
    #define LED_OFF LOW
  #endif

  #define LED_RED 7    // Set Led Pins
  #define LED_GREEN 6
  #define LED_BLUE 5
#endif

#ifdef WIPE_BUTTON_ACTIVE
  #define BUTTON_WIPE 3     // Button pin for WipeMode
#endif

#define RELAY 4     // Set Relay Pin
#define RELAY_LOCKED LOW
#define RELAY_OPENED HIGH

#define MOSI_PIN_MFRC 8
#define MISO_PIN_MFRC 9
#define SCK_PIN_MFRC 10
#define SS_PIN_MFRC A0
#define RST_PIN_MFRC A1

#define SS_PIN_SD A2
#define LOGFILE "LOGFILE.LOG"

#define WIRELESS_BUTTON 2

/* IMPORT SECTION
 *  
 * Here are located the imports needed for the app to work.
 *  
 */

#include <EEPROM.h>     // We are going to read and write PICC's UIDs from/to EEPROM
#include <SPI.h>        // RC522 Module uses SPI protocol
#include <MFRC522.h>  // Library for Mifare RC522 Devices

#include <TimeLib.h>
#include <Wire.h>
#include <DS1307RTC.h>

#include <Fat16.h>
#include "RFID.h"


boolean match = false;          // initialize card match to false
boolean programMode = false;  // initialize programming mode to false
boolean replaceMaster = false;

uint8_t successRead;    // Variable integer to keep if we have Successful Read from Reader

byte storedCard[4];   // Stores an ID read from EEPROM
byte readCard[4];   // Stores scanned ID read from RFID Module
byte masterCard[4];   // Stores master card's ID read from EEPROM

RFID RC522(SS_PIN_MFRC, RST_PIN_MFRC, MOSI_PIN_MFRC, MISO_PIN_MFRC, SCK_PIN_MFRC);

SdCard card;
Fat16 file;



///////////////////////////////////////// Setup ///////////////////////////////////
void setup() {
  digitalWrite(RELAY, RELAY_OPENED);
  pinMode(RELAY, OUTPUT);
  //Arduino Pin Configuration
  #ifdef WORK_LED_ACTIVE
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
  #endif
  #ifdef WIPE_BUTTON_ACTIVE
    pinMode(BUTTON_WIPE, INPUT_PULLUP);   // Enable pin's pull up resistor
  #endif
  pinMode(RELAY, OUTPUT);
  pinMode(WIRELESS_BUTTON, INPUT);
  
  

  //Protocol Configuration
  #ifdef SERIAL_DEBUG
    Serial.begin(9600);  // Initialize serial communications with PC
    Serial.println("Initializing...");
  #endif
  delay(3000);
  //Be careful how relay circuit behave on while resetting or power-cycling your Arduino
  digitalWrite(RELAY, RELAY_LOCKED);    // Make sure door is locked
  #ifdef WORK_LED_ACTIVE
    setLED(LED_OFF, LED_OFF, LED_OFF);
  #endif
  setSyncProvider(RTC.get); // Initialize RTC and set it as default time source
  SPI.begin();
  RC522.init();
  if (!card.begin(SS_PIN_SD)){
    #ifdef SERIAL_DEBUG
    Serial.println("Begin failed!");
    #endif
  }
  delay(200);
  if (!Fat16::init(&card)){
    #ifdef SERIAL_DEBUG
    Serial.println("Init failed!");
    #endif
  }

  //If you set Antenna Gain to Max it will increase reading distance
  //mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

  #ifdef SERIAL_DEBUG
    Serial.println(F("Access Control Example v0.1"));   // For debugging purposes
  #endif

  #ifdef WIPE_BUTTON_ACTIVE  //Wipe Code - If the Button (BUTTON_WIPE) Pressed while setup run (powered on) it wipes EEPROM
    if (digitalRead(BUTTON_WIPE) == LOW) {  // when button pressed pin should get low, button connected to ground
      #ifdef WORK_LED_ACTIVE
        setLED(LED_ON, LED_OFF, LED_OFF);
      #endif
      
      #ifdef SERIAL_DEBUG
        Serial.println(F("Wipe Button Pressed"));
        Serial.println(F("You have 10 seconds to Cancel"));
        Serial.println(F("This will be remove all records and cannot be undone"));
      #endif
      
      bool buttonState = monitorWipeButton(10000); // Give user enough time to cancel operation
      if (buttonState == true && digitalRead(BUTTON_WIPE) == LOW) {    // If button still be pressed, wipe EEPROM
        #ifdef SERIAL_DEBUG
          Serial.println(F("Starting Wiping EEPROM"));
        #endif
        
        for (uint16_t x = 0; x < EEPROM.length(); x = x + 1) {    //Loop end of EEPROM address
          if (EEPROM.read(x) == 0) {              //If EEPROM address 0
            // do nothing, already clear, go to the next address in order to save time and reduce writes to EEPROM
          }
          else {
            EEPROM.write(x, 0);       // if not write 0 to clear, it takes 3.3mS
          }
        }
        
        #ifdef SERIAL_DEBUG
          Serial.println(F("EEPROM Successfully Wiped"));
        #endif
        
        #ifdef WORK_LED_ACTIVE
          setLED(LED_OFF, LED_OFF, LED_OFF);
          delay(200);
          setLED(LED_ON, LED_OFF, LED_OFF);
          delay(200);
          setLED(LED_OFF, LED_OFF, LED_OFF);
          delay(200);
          setLED(LED_ON, LED_OFF, LED_OFF);
          delay(200);
          setLED(LED_OFF, LED_OFF, LED_OFF);
        #endif
      }
      else {
        #ifdef SERIAL_DEBUG
          Serial.println(F("Wiping Cancelled")); // Show some feedback that the wipe button did not pressed for 15 seconds
        #endif
        
        #ifdef WORK_LED_ACTIVE
          setLED(LED_OFF, LED_OFF, LED_OFF);
        #endif
      }
    }
  #endif

  
  // Check if master card defined, if not let user choose a master card
  // This also useful to just redefine the Master Card
  // You can keep other EEPROM records just write other than 42 to EEPROM address 1
  // EEPROM address 1 should hold magical number which is '42'
  if (EEPROM.read(1) != 42) {
    #ifdef SERIAL_DEBUG
      Serial.println(F("No Master Card Defined"));
      Serial.println(F("Scan A PICC to Define as Master Card"));
    #endif
    
    do {
      successRead = getID();            // sets successRead to 1 when we get read from reader otherwise 0
      #ifdef WORK_LED_ACTIVE
        setLED(LED_OFF, LED_OFF, LED_ON);
        delay(200);
        setLED(LED_OFF, LED_OFF, LED_OFF);
        delay(200);
      #endif
    }
    while (!successRead);                  // Program will not go further while you not get a successful read
    for ( uint8_t j = 0; j < 4; j++ ) {        // Loop 4 times
      EEPROM.write( 2 + j, readCard[j] );  // Write scanned PICC's UID to EEPROM, start from address 3
    }
    EEPROM.write(1, 42);                  // Write to EEPROM we defined Master Card.
    
    #ifdef SERIAL_DEBUG
      Serial.println(F("Master Card Defined"));
    #endif
  }
  #ifdef SERIAL_DEBUG
    Serial.println(F("-------------------"));
    Serial.println(F("Master Card's UID"));
  #endif
  
  for ( uint8_t i = 0; i < 4; i++ ) {          // Read Master Card's UID from EEPROM
    masterCard[i] = EEPROM.read(2 + i);    // Write it to masterCard
    
    #ifdef SERIAL_DEBUG
      Serial.print(masterCard[i], HEX);
    #endif
  }
  
  #ifdef SERIAL_DEBUG
    Serial.println("");
    Serial.println(F("-------------------"));
    Serial.println(F("Everything is ready"));
    Serial.println(F("Waiting PICCs to be scanned"));
  #endif
  
  #ifdef WORK_LED_ACTIVE
    cycleLeds();    // Everything ready lets give user some feedback by cycling leds
  #endif
}

long m = 0;

///////////////////////////////////////// Main Loop ///////////////////////////////////
void loop() {
  
  do {
    //RESET, A TO GÓWNO NO
    if (m + 300 <= now()) {
      #ifdef SERIAL_DEBUG
        Serial.println("Resetting...");
      #endif
      
      RC522.init();
      delay(1000);
      
      #ifdef SERIAL_DEBUG
        Serial.println("Reset done.");
      #endif
      
      m = now();
    }

    //WIRELESS

    if (digitalRead(WIRELESS_BUTTON) == HIGH){
      long t = now();
      while (digitalRead(WIRELESS_BUTTON) == HIGH);
      if (t + 5 <= now()){
        while (digitalRead(WIRELESS_BUTTON) == LOW);
        granted(3000);
      }else{

        //normal work
        
        file.writeError = false;
        if (!file.open(LOGFILE, O_CREAT | O_APPEND | O_WRITE)) Serial.println("Open failed!");
        //file.write((uint8_t) now());
        long t = now();
        for (int i = 0; i < 4; i++){
          file.write((uint8_t) 0x0);
        }
        for (int i = 0; i < 4; i++){
          byte j = ((t >> (3 - i) * 8));
          file.write((uint8_t) j);
        }
        if (file.writeError) Serial.println("Write failed");
        if (!file.close()) Serial.println("Close failed");
        granted(3000);
      }
    }
    
    if (digitalRead(WIRELESS_BUTTON) == HIGH){
      
    }
    
    successRead = getID();  // sets successRead to 1 when we get read from reader otherwise 0
    #ifdef WIPE_BUTTON_ACTIVE  // When device is in use if wipe button pressed for 10 seconds initialize Master Card wiping
      if (digitalRead(BUTTON_WIPE) == LOW) { // Check if button is pressed
        // Visualize normal operation is iterrupted by pressing wipe button Red is like more Warning to user
        #ifdef WORK_LED_ACTIVE
          setLED(LED_ON, LED_OFF, LED_OFF);
        #endif
        // Give some feedback
        #ifdef SERIAL_DEBUG
          Serial.println(F("Wipe Button Pressed"));
          Serial.println(F("Master Card will be Erased! in 10 seconds"));
        #endif
        
        bool buttonState = monitorWipeButton(10000); // Give user enough time to cancel operation
        if (buttonState == true && digitalRead(BUTTON_WIPE) == LOW) {    // If button still be pressed, wipe EEPROM
          EEPROM.write(1, 0);                  // Reset Magic Number.
          
          #ifdef SERIAL_DEBUG
            Serial.println(F("Master Card Erased from device"));
            Serial.println(F("Please reset to re-program Master Card"));
          #endif
          while (1);
        }
        #ifdef SERIAL_DEBUG
          Serial.println(F("Master Card Erase Cancelled"));
        #endif
      }
    #endif

    
    if (programMode) {
      #ifdef WORK_LED_ACTIVE
        cycleLeds();              // Program Mode cycles through Red Green Blue waiting to read a new card
      #endif
    }
    else {
      normalModeOn();     // Normal mode, blue Power LED is on, all others are off
    }
  } while (!successRead);   //the program will not go further while you are not getting a successful read

  
  if (programMode) {
    if ( isMaster(readCard) ) { //When in program mode check First If master card scanned again to exit program mode
      #ifdef SERIAL_DEBUG
        Serial.println(F("Master Card Scanned"));
        Serial.println(F("Exiting Program Mode"));
        Serial.println(F("-----------------------------"));
      #endif
      delay(3000);
      programMode = false;
      return;
    }
    else {
      if ( findID(readCard) ) { // If scanned card is known delete it
        #ifdef SERIAL_DEBUG
          Serial.println(F("I know this PICC, removing..."));
        #endif
        
        deleteID(readCard);
        
        #ifdef SERIAL_DEBUG
          Serial.println("-----------------------------");
          Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
        #endif
        delay(3000);
      }
      else {                    // If scanned card is not known add it
        #ifdef SERIAL_DEBUG
          Serial.println(F("I do not know this PICC, adding..."));
        #endif
        
        writeID(readCard);
        
        #ifdef SERIAL_DEBUG
          Serial.println(F("-----------------------------"));
          Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
        #endif
        delay(3000);
      }
    }
  }
  else {
    if ( isMaster(readCard)) {    // If scanned card's ID matches Master Card's ID - enter program mode
      programMode = true;
      #ifdef SERIAL_DEBUG
        Serial.println(F("Hello Master - Entered Program Mode"));
        uint8_t count = EEPROM.read(0);   // Read the first Byte of EEPROM that
        Serial.print(F("I have "));     // stores the number of ID's in EEPROM
        Serial.print(count);
        Serial.print(F(" record(s) on EEPROM"));
        Serial.println("");
        Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
        Serial.println(F("Scan Master Card again to Exit Program Mode"));
        Serial.println(F("-----------------------------"));
      #endif
      delay(3000);
    }
    else {
      if ( findID(readCard) ) { // If not, see if the card is in the EEPROM
        #ifdef SERIAL_DEBUG
          Serial.println(F("Welcome, You shall pass"));
        #endif

        file.writeError = false;
        if (!file.open(LOGFILE, O_CREAT | O_APPEND | O_WRITE)) Serial.println("Open failed!");
//      file.write((uint8_t) now());
        long t = now();
        for (int i = 0; i < 4; i++){
          file.write((uint8_t) readCard[i]);
        }
        for (int i = 0; i < 4; i++){
          byte j = ((t >> (3 - i) * 8));
          file.write((uint8_t) j);
        }
      if (file.writeError) Serial.println("Write failed");
      if (!file.close()) Serial.println("Close failed");
        
        granted(1500);         // Open the door lock for 300 ms
      }
      else {      // If not, show that the ID was not valid
        #ifdef SERIAL_DEBUG
          Serial.println(F("You shall not pass"));
          #endif
        denied();
      }
    }
  }
}

/////////////////////////////////////////  Access Granted    ///////////////////////////////////
void granted ( uint16_t setDelay) {
  #ifdef WORK_LED_ACTIVE
    setLED(LED_OFF, LED_ON, LED_OFF);
  #endif
  digitalWrite(RELAY, RELAY_OPENED);     // Unlock door!
  delay(setDelay);          // Hold door lock open for given seconds
  digitalWrite(RELAY, RELAY_LOCKED);    // Relock door
}

///////////////////////////////////////// Access Denied  ///////////////////////////////////
void denied() {
  #ifdef WORK_LED_ACTIVE
    setLED(LED_ON, LED_OFF, LED_OFF);
  #endif
  delay(1000);
}


///////////////////////////////////////// Get PICC's UID ///////////////////////////////////
uint8_t getID() {
  // Getting ready for Reading PICCs
  if ( ! RC522.isCard()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! RC522.readCardSerial()) {   //Since a PICC placed get Serial and continue
    return 0;
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  #ifdef SERIAL_DEBUG
    Serial.println(F("Scanned PICC's UID:"));
  #endif
  
  for ( uint8_t i = 0; i < 4; i++) {  //
    readCard[i] = RC522.serNum[i];

    #ifdef SERIAL_DEBUG
      Serial.print(readCard[i], HEX);
    #endif
  }

  #ifdef SERIAL_DEBUG
    Serial.println("");
  #endif
  
  return 1;
}


///////////////////////////////////////// Cycle Leds (Program Mode) ///////////////////////////////////
#ifdef WORK_LED_ACTIVE
  void cycleLeds() {
    setLED(LED_OFF, LED_ON, LED_OFF);
    delay(200);
    setLED(LED_OFF, LED_OFF, LED_ON);
    delay(200);
    setLED(LED_ON, LED_OFF, LED_OFF);
    delay(200);
  }
#endif

//////////////////////////////////////// Normal Mode Led  ///////////////////////////////////
void normalModeOn () {
  #ifdef WORK_LED_ACTIVE
    setLED(LED_OFF, LED_OFF, LED_ON);
  #endif
  digitalWrite(RELAY, RELAY_LOCKED);    // Make sure Door is Locked
}

//////////////////////////////////////// Read an ID from EEPROM //////////////////////////////
void readID( uint8_t number ) {
  uint8_t start = (number * 4 ) + 2;    // Figure out starting position
  for ( uint8_t i = 0; i < 4; i++ ) {     // Loop 4 times to get the 4 Bytes
    storedCard[i] = EEPROM.read(start + i);   // Assign values read from EEPROM to array
  }
}

///////////////////////////////////////// Add ID to EEPROM   ///////////////////////////////////
void writeID( byte a[] ) {
  if ( !findID( a ) ) {     // Before we write to the EEPROM, check to see if we have seen this card before!
    uint8_t num = EEPROM.read(0);     // Get the numer of used spaces, position 0 stores the number of ID cards
    uint8_t start = ( num * 4 ) + 6;  // Figure out where the next slot starts
    num++;                // Increment the counter by one
    EEPROM.write( 0, num );     // Write the new count to the counter
    for ( uint8_t j = 0; j < 4; j++ ) {   // Loop 4 times
      EEPROM.write( start + j, a[j] );  // Write the array values to EEPROM in the right position
    }
    #ifdef WORK_LED_ACTIVE
      successWrite();
    #endif

    #ifdef SERIAL_DEBUG
      Serial.println(F("Succesfully added ID record to EEPROM"));
    #endif
  }
  else {
    #ifdef WORK_LED_ACTIVE
      failedWrite();
    #endif

    #ifdef SERIAL_DEBUG
      Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
    #endif
  }
}

///////////////////////////////////////// Remove ID from EEPROM   ///////////////////////////////////
void deleteID( byte a[] ) {
  if ( !findID( a ) ) {     // Before we delete from the EEPROM, check to see if we have this card!
    #ifdef WORK_LED_ACTIVE
      failedWrite();      // If not
    #endif

    #ifdef SERIAL_DEBUG
      Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
    #endif
  }
  else {
    uint8_t num = EEPROM.read(0);   // Get the numer of used spaces, position 0 stores the number of ID cards
    uint8_t slot;       // Figure out the slot number of the card
    uint8_t start;      // = ( num * 4 ) + 6; // Figure out where the next slot starts
    uint8_t looping;    // The number of times the loop repeats
    uint8_t j;
    uint8_t count = EEPROM.read(0); // Read the first Byte of EEPROM that stores number of cards
    slot = findIDSLOT( a );   // Figure out the slot number of the card to delete
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;      // Decrement the counter by one
    EEPROM.write( 0, num );   // Write the new count to the counter
    for ( j = 0; j < looping; j++ ) {         // Loop the card shift times
      EEPROM.write( start + j, EEPROM.read(start + 4 + j));   // Shift the array values to 4 places earlier in the EEPROM
    }
    for ( uint8_t k = 0; k < 4; k++ ) {         // Shifting loop
      EEPROM.write( start + j + k, 0);
    }
    #ifdef WORK_LED_ACTIVE
      successDelete();
    #endif

    #ifdef SERIAL_DEBUG
      Serial.println(F("Succesfully removed ID record from EEPROM"));
    #endif
  }
}

///////////////////////////////////////// Check Bytes   ///////////////////////////////////
boolean checkTwo ( byte a[], byte b[] ) {
  if ( a[0] != 0 )      // Make sure there is something in the array first
    match = true;       // Assume they match at first
  for ( uint8_t k = 0; k < 4; k++ ) {   // Loop 4 times
    if ( a[k] != b[k] )     // IF a != b then set match = false, one fails, all fail
      match = false;
  }
  if ( match ) {      // Check to see if if match is still true
    return true;      // Return true
  }
  else  {
    return false;       // Return false
  }
}

///////////////////////////////////////// Find Slot   ///////////////////////////////////
uint8_t findIDSLOT( byte find[] ) {
  uint8_t count = EEPROM.read(0);       // Read the first Byte of EEPROM that
  for ( uint8_t i = 1; i <= count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);                // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      // is the same as the find[] ID card passed
      return i;         // The slot number of the card
      break;          // Stop looking we found it
    }
  }
}

///////////////////////////////////////// Find ID From EEPROM   ///////////////////////////////////
boolean findID( byte find[] ) {
  uint8_t count = EEPROM.read(0);     // Read the first Byte of EEPROM that
  for ( uint8_t i = 1; i <= count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);          // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      return true;
      break;  // Stop looking we found it
    }
    else {    // If not, return false
    }
  }
  return false;
}


#ifdef WORK_LED_ACTIVE
  void setLED(bool red, bool green, bool blue){
    digitalWrite(LED_BLUE, blue);
    digitalWrite(LED_RED, red);
    digitalWrite(LED_GREEN, green);
  }
  
  ///////////////////////////////////////// Write Success to EEPROM   ///////////////////////////////////
  // Flashes the green LED 3 times to indicate a successful write to EEPROM
  void successWrite() {
    setLED(LED_OFF, LED_OFF, LED_OFF);
    delay(200);
    setLED(LED_OFF, LED_ON, LED_OFF);
    delay(200);
    setLED(LED_OFF, LED_OFF, LED_OFF);
    delay(200);
    setLED(LED_OFF, LED_ON, LED_OFF);
    delay(200);
    setLED(LED_OFF, LED_OFF, LED_OFF);
    delay(200);
    setLED(LED_OFF, LED_ON, LED_OFF);
    delay(200);
  }
  
  ///////////////////////////////////////// Write Failed to EEPROM   ///////////////////////////////////
  // Flashes the red LED 3 times to indicate a failed write to EEPROM
  void failedWrite() {
    setLED(LED_OFF, LED_OFF, LED_OFF);
    delay(200);
    setLED(LED_ON, LED_OFF, LED_ON);
    delay(200);
    setLED(LED_OFF, LED_OFF, LED_ON);
    delay(200);
    setLED(LED_ON, LED_OFF, LED_ON);
    delay(200);
    setLED(LED_OFF, LED_OFF, LED_ON);
    delay(200);
    setLED(LED_ON, LED_OFF, LED_ON);
    delay(200);
  }
  
  ///////////////////////////////////////// Success Remove UID From EEPROM  ///////////////////////////////////
  // Flashes the blue LED 3 times to indicate a success delete to EEPROM
  void successDelete() {
    setLED(LED_OFF, LED_OFF, LED_OFF);
    delay(200);
    setLED(LED_OFF, LED_OFF, LED_ON);
    delay(200);
    setLED(LED_OFF, LED_OFF, LED_OFF);
    delay(200);
    setLED(LED_OFF, LED_OFF, LED_ON);
    delay(200);
    setLED(LED_OFF, LED_OFF, LED_OFF);
    delay(200);
    setLED(LED_OFF, LED_OFF, LED_ON);
    delay(200);
  }
#endif

////////////////////// Check readCard IF is masterCard   ///////////////////////////////////
// Check to see if the ID passed is the master programing card
boolean isMaster( byte test[] ) {
  if ( checkTwo( test, masterCard ) )
    return true;
  else
    return false;
}

#ifdef WIPE_BUTTON_ACTIVE
  bool monitorWipeButton(uint32_t interval) {
    uint32_t now = (uint32_t)millis();
    while ((uint32_t)millis() - now < interval)  {
      // check on every half a second
      if (((uint32_t)millis() % 500) == 0) {
        if (digitalRead(BUTTON_WIPE) != LOW)
          return false;
      }
    }
    return true;
  }
#endif
