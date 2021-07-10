
#include "EEPROM.h"    // We are going to read and write PICC's UIDs from/to EEPROM
#include <SPI.h>        // RC522 Module uses SPI protocol
#include <MFRC522.h>  // Library for Mifare RC522 Devices





#define EEPROM_SIZE 512

boolean programMode = false;  // initialize programming mode to false
boolean match = false;
uint8_t successRead;    // Variable integer to keep if we have Successful Read from Reader
byte storedCard[4];
byte storedCard1[4];   // Stores an ID read from EEPROM
byte readCard[4];   // Stores scanned ID read from RFID Module
byte masterCard[4];   // Stores master card's ID read from EEPROM
// Create MFRC522 instance.
#define SS_PIN D4
#define RST_PIN D3
MFRC522 mfrc522(SS_PIN, RST_PIN);

///////////////////////////////////////// Setup ///////////////////////////////////
void setup() {
  //Arduino Pin Configuration



  //Protocol Configuration
  Serial.begin(115200);  // Initialize serial communications with PC
  SPI.begin();           // MFRC522 Hardware uses SPI protocol
  mfrc522.PCD_Init();    // Initialize MFRC522 Hardware

  //If you set Antenna Gain to Max it will increase reading distance
  //mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

  Serial.println("Access Control Example v0.1");   // For debugging purposes

  EEPROM.begin(64);
 Serial.println(" bytes read from Flash . Values are:");
  for (int i = 0; i < EEPROM_SIZE; i++)
  {
    Serial.print(byte(EEPROM.read(i))); Serial.print(" ");
  }
  Serial.println();

  //Wipe Code - If the Button (wipeB) Pressed while setup run (powered on) it wipes EEPROM
  // Check if master card defined, if not let user choose a master card
  // This also useful to just redefine the Master Card
  // You can keep other EEPROM records just write other than 143 to EEPROM address 1
  // EEPROM address 1 should hold magical number which is '143'
  if (EEPROM.read(1) != 143) {
    Serial.println("No Master Card Defined");
    Serial.println("Scan A PICC to Define as Master Card");
    do {
      successRead = getID();           
      delay(200);
      
    }
    while (!successRead);                  
    for ( uint8_t j = 0; j < 4; j++ ) {       
      EEPROM.write( 2 + j, readCard[j] ); 
    }
    EEPROM.write(1, 143);                 
    EEPROM.commit();
    Serial.println("Master Card Defined");
  }
  Serial.println("-------------------");
  Serial.println("Master Card's UID");
  for ( uint8_t i = 0; i < 4; i++ ) {          // Read Master Card's UID from EEPROM
    masterCard[i] = EEPROM.read(2 + i);    // Write it to masterCard
    Serial.print(masterCard[i], HEX);
  }
  Serial.println("");
  Serial.println("-------------------");
  Serial.println("Everything is ready");
  Serial.println("Waiting PICCs to be scanned");
      // Everything ready lets give user some feedback by cycling leds
}


///////////////////////////////////////// Main Loop ///////////////////////////////////
void loop () {
  do {
      successRead = getID();
      delay(200);  // sets successRead to 1 when we get read from reader otherwise 0
    }
    
  while (!successRead);

    
  if (programMode) {
    if ( isMaster(readCard) ) { //When in program mode check First If master card scanned again to exit program mode
      Serial.println("Master Card Scanned");
      Serial.println("Exiting Program Mode");
      Serial.println("-----------------------------");
      programMode = false;
      return;
    }
    else {
      
      if ( findID(readCard) ) { // If scanned card is known delete it
        Serial.println("I know this PICC, removing...");
        deleteID(readCard);
        Serial.println("-----------------------------");
        Serial.println("Scan a PICC to ADD or REMOVE to EEPROM");
        
      }
      else {                    // If scanned card is not known add it
        Serial.println("I do not know this PICC, adding...");
        writeID(readCard);
        Serial.println("-----------------------------");
      }
        for( uint8_t i = 0; i < 4; i++  )
        {
          readCard[i] = 0;
          }
          successRead = getID();
    }
  }
  else {
    if ( isMaster(readCard)) {    // If scanned card's ID matches Master Card's ID - enter program mode
      programMode = true;
      Serial.println("Hello Master - Entered Program Mode");
      uint8_t count = EEPROM.read(0);   // Read the first Byte of EEPROM that
      Serial.print("I have ");     // stores the number of ID's in EEPROM
      Serial.print(count);
      Serial.print(" record(s) on EEPROM");
      Serial.println("");
      Serial.println("Scan a PICC to ADD or REMOVE to EEPROM");
      Serial.println("Scan Master Card again to Exit Program Mode");
      Serial.println("-----------------------------");
    }
    else {
      if ( findID(readCard) ) { // If not, see if the card is in the EEPROM
        Serial.println("Welcome, You shall pass");
      }
      else {      // If not, show that the ID was not valid
        Serial.println("You shall not pass");
      }
    }
  }
  for( uint8_t i = 0; i < 4; i++  )
        {
          readCard[i] = 0;
          }
}




///////////////////////////////////////// Get PICC's UID ///////////////////////////////////
uint8_t getID() {
  // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 0;
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  Serial.println("Scanned PICC's UID:");
  for ( uint8_t i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.print(readCard[i], HEX);
  }
  Serial.println("");
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}

void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print("MFRC522 Software Version: 0x");
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(" = v1.0");
  else if (v == 0x92)
    Serial.print(" = v2.0");
  else
    Serial.print(" (unknown),probably a chinese clone?");
  Serial.println("");
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF)) {
    Serial.println("WARNING: Communication failure, is the MFRC522 properly connected?");
    Serial.println("SYSTEM HALTED: Check connections.");
       // Turn on red LED
    while (true); // do not go further
  }
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
    EEPROM.write( 0, num );
    EEPROM.commit();// Write the new count to the counter
    for ( uint8_t j = 0; j < 4; j++ ) {   // Loop 4 times
      EEPROM.write( start + j, a[j] );  // Write the array values to EEPROM in the right position
      EEPROM.commit();
    }
    Serial.println("Succesfully added ID record to EEPROM");
  }
  else {
    
    Serial.println("Failed! There is something wrong with ID or bad EEPROM");
  }
}

///////////////////////////////////////// Remove ID from EEPROM   ///////////////////////////////////
void deleteID( byte c[] ) {
  if ( !findID( c ) ) {     // Before we delete from the EEPROM, check to see if we have this card!
     Serial.println("Failed! There is something wrong with ID or bad EEPROM");
  }
  else {
    uint8_t num = EEPROM.read(0);   // Get the numer of used spaces, position 0 stores the number of ID cards
    uint8_t slot;       // Figure out the slot number of the card
    uint8_t start;      // = ( num * 4 ) + 6; // Figure out where the next slot starts
    uint8_t looping;    // The number of times the loop repeats
    uint8_t j;
    uint8_t count = EEPROM.read(0); // Read the first Byte of EEPROM that stores number of cards
    slot = findIDSLOT( c );   // Figure out the slot number of the card to delete
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;      // Decrement the counter by one
    EEPROM.write( 0, num );   // Write the new count to the counter
    EEPROM.commit();
    for ( j = 0; j < looping; j++ ) {         // Loop the card shift times
      EEPROM.write( start + j, EEPROM.read(start + 4 + j));   // Shift the array values to 4 places earlier in the EEPROM
      EEPROM.commit();    
    }
    
    for ( uint8_t k = 0; k < 4; k++ ) {         // Shifting loop
      EEPROM.write( start + j + k, 0);
      EEPROM.commit();
    }
    
    Serial.println("Succesfully removed ID record from EEPROM");
  }
}


///////////////////////////////////////// Find Slot   ///////////////////////////////////
uint8_t findIDSLOT( byte find[] ) {
  uint8_t count = EEPROM.read(0);       // Read the first Byte of EEPROM that
  for ( uint8_t i = 1; i <= count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);                // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      // is the same as the find[] ID card passed
      return i;
      break;
    }
  }
}

///////////////////////////////////////// Find ID From EEPROM   ///////////////////////////////////
bool findID( byte find[] ) {
  uint8_t count = EEPROM.read(0);     // Read the first Byte of EEPROM that
  for ( uint8_t i = 1; i < count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);          // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      return true;
      break;
    }
    else {    // If not, return false
    }
  }
  return false;  
}


bool isMaster( byte test[] ){
  if (checkTwo(test,masterCard))
  return true;
  else
  return false;
}


///////////////////////////////////////// Check Bytes   ///////////////////////////////////
bool checkTwo ( byte a[], byte b[] )
{
  if (a[0] != NULL)
  match = true;
  for ( uint8_t k = 0; k < 4; k++ ) {   // Loop 4 times
    if ( a[k] != b[k] )      // IF a != b then false, because: one fails, all fail
       match = false;
    }
    if (match){
      return true;
    }
    else{
      return false;
    }
}
