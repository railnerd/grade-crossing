#include <WaveHC.h>
#include <WaveUtil.h>

// Arduino Pin Assignments
#define  EB_DETECT  A0
#define  WB_DETECT  A1
#define  LED_LEFT   A2
#define  LED_RIGHT  A3

// Global State
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
  Serial.begin(9600);  // for debugging

  // Set up detector inputs, and enable on-chip ~20K pullup resistors
  pinMode(EB_DETECT,INPUT);
  pinMode(WB_DETECT,INPUT);
  digitalWrite(EB_DETECT,HIGH);
  digitalWrite(WB_DETECT,HIGH);

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
    if ((digitalRead(EB_DETECT) == LOW) && (digitalRead(WB_DETECT) == HIGH)) {
      interlockingState = kEastboundApproach;
    } 
    else if ((digitalRead(WB_DETECT) == LOW) && (digitalRead(EB_DETECT) == HIGH)) {
      interlockingState = kWestboundApproach;    
    }
    break;

  case kEastboundApproach:
    interlockingState = kApproachCommon;
    break;

  case kWestboundApproach:
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
    if ((digitalRead(WB_DETECT) == HIGH) && (digitalRead(EB_DETECT) == HIGH)) {
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


///////////////////////////////////////////////////////////////////
//
//  SD Card and WAV Playback
//

SdReader card;  // SDCard Interface
FatVolume vol;  // Partition on the card
FatReader root; // Root directory on the partition
FatReader f;    // File we're going to play off the root directory
WaveHC wave;    // WAV File Player

#define	PANIC(x)	do {Serial.println(x); while (1);} while(0)

void setupSDCard()
{
  //if (!card.init(true)) { //play with 4 MHz spi if 8MHz isn't working for you
  if (!card.init()) {         //play with 8 MHz spi (default faster!)  
    PANIC(F("Card init. failed!"));  // Something went wrong, lets print out hy
  }

  // enable optimize read - some cards may timeout. Disable if you're having problems
  card.partialBlockRead(true);

  // Now we will look for a FAT partition, we have 5 spots to check
  uint8_t part;
  for (part = 0; part < 5; part++) {
    if (vol.init(card, part)) 
      break; // we found one, lets bail
  }
  if (part == 5) {
    // if we ended up not finding one  :(
    PANIC(F("No valid FAT partition!"));
  }

  // Try to open the root directory
  if (!root.openRoot(vol)) {
    PANIC(F("Can't open root dir!"));
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

///////////////////////////////////////////////////////////////////
//
// LED Animation Routines
//

enum {
  kLEDsOff = 0,
  kLEDLeftOn,
  kLEDRightOn
};


int ledState = kLEDsOff;
int ledTimer = 0;

void turnOffLEDs(void) {
  digitalWrite(LED_RIGHT,HIGH);
  digitalWrite(LED_LEFT,HIGH);
  ledState = kLEDsOff;
}

void animateLEDs(void) {

  if (ledTimer)
    ledTimer--;

  switch (ledState) {

  case kLEDsOff:
    ledTimer = 0;
    // fall through

  case kLEDRightOn:
    if (!ledTimer) {
      ledState = kLEDLeftOn;
      ledTimer = 10000;
      digitalWrite(LED_LEFT,LOW);
      digitalWrite(LED_RIGHT,HIGH);
    } 
    break;

  case kLEDLeftOn:
    if (!ledTimer) {
      ledState = kLEDRightOn;
      ledTimer = 10000;
      digitalWrite(LED_LEFT,HIGH);
      digitalWrite(LED_RIGHT,LOW);
    }
    break;
  }
}

