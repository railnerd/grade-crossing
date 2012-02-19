#include <WaveHC.h>
#include <WaveUtil.h>

#define  EB_Detector  A0
#define  WB_Detector  A1

#define  LED_LEFT     A2
#define  LED_RIGHT    A3


enum {
  kIdle = 0,
  kEastboundApproach,
  kWestboundApproach,
  kApproachCommon,
  kOccupied
};

int interlockingState = kIdle;
int deactivateTimer = 10000;

// Function Prototypes

void setupSDCard();
void playWAVFile(char *name);
void playCrossingBell(void);

void turnOffLEDs(void);
void animateLEDs(void);


void setup()
{
  Serial.begin(9600);  // debugging

  Serial.print(F("Free Memory:"));
  Serial.println(FreeRam());

  // Set up detector inputs, and enable on-chip ~20K pullup resistors
  pinMode(EB_Detector,INPUT);
  pinMode(WB_Detector,INPUT);
  digitalWrite(EB_Detector,HIGH);
  digitalWrite(WB_Detector,HIGH);

  // Set up LED outputs
  pinMode(LED_LEFT,OUTPUT);
  pinMode(LED_RIGHT,OUTPUT);

  // Get ready to play audio files from the SD Card
  setupSDCard();
  turnOffLEDs();
}




void loop()
{
  switch (interlockingState) {
  case kIdle:
    if ((digitalRead(EB_Detector) == LOW) && (digitalRead(WB_Detector) == HIGH)) {
      interlockingState = kEastboundApproach;
    } 
    else if ((digitalRead(WB_Detector) == LOW) && (digitalRead(EB_Detector) == HIGH)) {
      interlockingState = kWestboundApproach;    
    }
    break;

  case kEastboundApproach:
    // EBS = clear
    // WBS = stop
//  Serial.println(F("eastbound approach"));
    interlockingState = kApproachCommon;
    break;

  case kWestboundApproach:
    // WBS = clear
    // EBS = stop
//  Serial.println(F("westbound approach"));
    interlockingState = kApproachCommon;
    break;

  case kApproachCommon:
    deactivateTimer = 10000;
    interlockingState = kOccupied;
    break;

  case kOccupied:
    animateLEDs();
    playCrossingBell();

    // Hang out in occupied state until both detectors are showing clear
    if ((digitalRead(WB_Detector) == HIGH) && (digitalRead(EB_Detector) == HIGH)) {
      deactivateTimer--;
      if (!deactivateTimer) {
        turnOffLEDs();
        interlockingState = kIdle;
      }
    } else {
      deactivateTimer = 10000;
    }
    break;

  default:
    break;
  }
}


SdReader card;  // SDCard Interface
FatVolume vol;  // Partition on the card
FatReader root; // Root directory on the partition
FatReader f;    // File we're going to play off the root directory

WaveHC wave;    // WAV File Player

void setupSDCard()
{
//  Serial.println(F("setupSDCard"));

  //if (!card.init(true)) { //play with 4 MHz spi if 8MHz isn't working for you
  if (!card.init()) {         //play with 8 MHz spi (default faster!)  
    Serial.println(F("Card init. failed!"));  // Something went wrong, lets print out hy
    while (1)
      ;
  }

  // enable optimize read - some cards may timeout. Disable if you're having problems
  card.partialBlockRead(true);

  // Now we will look for a FAT partition!
  uint8_t part;
  for (part = 0; part < 5; part++) {   // we have up to 5 slots to look in
    if (vol.init(card, part)) 
      break;                           // we found one, lets bail
  }
  if (part == 5) {                     // if we ended up not finding one  :(
    Serial.println(F("No valid FAT partition!"));  // Something went wrong, lets print out why
    while (1)
      ;
  }

  // Try to open the root directory
  if (!root.openRoot(vol)) {
    Serial.println(F("Can't open root dir!"));      // Something went wrong,
    while (1)
      ;
  }
}


void playCrossingBell() {
  if (!wave.isplaying)
    playWAVFile("BELL.WAV");
}


void playWAVFile(char *name) {
  // see if the wave object is currently doing something, stop it.
  if (wave.isplaying) wave.stop();

  // look in the root directory and open the file
  if (!f.open(root, name)) {
    Serial.print(F("Couldn't open file ")); 
    Serial.println(name);
    return;
  }

  // OK read the file and turn it into a wave object
  if (!wave.create(f)) {
    Serial.println(F("Not a valid WAV"));
    return;
  }

  wave.play();
}


// LED Animation Routines

enum {
  kLEDsOff = 0,
  kLEDLeftOn,
  kLEDRightOn
};


int ledState = kLEDsOff;
int ledTimer = 0;

void turnOffLEDs(void) {
//  Serial.println(F("LEDs OFF"));
  digitalWrite(LED_RIGHT,HIGH);
  digitalWrite(LED_LEFT,HIGH);
  ledState = kLEDsOff;
}

void animateLEDs(void) {

  if (ledTimer)
    ledTimer--;

  switch (ledState) {

  case kLEDsOff:
//    Serial.println(F("LEDs ON"));
    ledTimer = 0;
    // fall through

  case kLEDRightOn:
    if (!ledTimer) {
      ledState = kLEDLeftOn;
      ledTimer = 10000;
      digitalWrite(LED_LEFT,LOW);
      digitalWrite(LED_RIGHT,HIGH);
//      Serial.println(F("LEFT"));
    } 
    break;

  case kLEDLeftOn:
    if (!ledTimer) {
      ledState = kLEDRightOn;
      ledTimer = 10000;
      digitalWrite(LED_LEFT,HIGH);
      digitalWrite(LED_RIGHT,LOW);
//      Serial.println(F("RIGHT"));
    }
    break;
  }
}

