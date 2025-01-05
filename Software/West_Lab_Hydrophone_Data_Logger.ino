// This sketch is for the WEST lab hydrophone datalogger REV1.

#include <Arduino.h>
#include <Bounce.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <stdint.h>
#include <TimeLib.h>

// Timing parameters, seconds
#define RECORD_PERIOD 60 //60 = 1 min
#define IDLE_PERIOD 60  //840 = 14min


AudioInputI2S i2s1;     
AudioRecordQueue queue1;  
AudioConnection patchCord(i2s1, 0, queue1, 0);
AudioControlSGTL5000 sgtl5000_1;  
int myInput;

// Use these with the Teensy Audio Shield
#define SDCARD_CS_PIN 10
#define SDCARD_MOSI_PIN 7  // Teensy 4 ignores this, uses pin 11
#define SDCARD_SCK_PIN 14  // Teensy 4 ignores this, uses pin 13

// Pin defs
int userLED = 17;
int errorLED = 16;
int bootConfig_power = 9;
int bootConfig_0 = 2;
int bootConfig_1 = 3;
int bootConfig_2 = 4;
int bootConfig_3 = 5;

// Config globals for file name
int gain_level;
int input_type; // 0 = MIC, 1 = Line in

// Recording flag
int waiting = 0;

// Create an IntervalTimer object for recording
IntervalTimer myTimer;

// The file where data is recorded
File fptr;
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
void setup() 
{

  pinMode(userLED, OUTPUT);
  pinMode(errorLED, OUTPUT);
  pinMode(bootConfig_power, OUTPUT);

  pinMode(bootConfig_0, INPUT);
  pinMode(bootConfig_1, INPUT);
  pinMode(bootConfig_2, INPUT);
  pinMode(bootConfig_3, INPUT);
  
  // Audio connections require memory, and the record queue
  // uses this memory to buffer incoming audio.
  AudioMemory(120);
  // Enable the audio shield
  sgtl5000_1.enable();

  // Configure the audio shield
  bootConfig();
  SPI.setClockDivider(SPI_CLOCK_DIV4);
  // Initialize the SD card
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);

  setSyncProvider(getTeensy3Time);

  if (!(SD.begin(SDCARD_CS_PIN))) {
    // stop here if no SD card
    while (1) {
      blinkLED(errorLED, 5, 500);
    }
  }
  delay(1000); // give the SD card some time to initilize
}

void loop() 
{
    waiting = 0;
    noInterrupts();
    myTimer.begin(timerCB,  (RECORD_PERIOD * 1000000));
    interrupts();
    startRecording();
    while (waiting == 0) 
    {
      continueRecording();
    }
    stopRecording();
    delay(10);
    setWakeupCallandSleep(IDLE_PERIOD);
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void startRecording() 
{
  char name_buffer[50] = {0};
  sprintf(name_buffer, "RECORD.%d.%d-%d-%d-%d-%d-%d-%d.RAW", input_type, gain_level, year(), month(), day(), hour(), minute(), second());
  fptr = SD.open(name_buffer, FILE_WRITE);
  if(fptr) 
  {
    queue1.begin();
  }
  else
  {
    blinkLED(errorLED, 8, 250);
  }
}

void continueRecording() 
{
  if (queue1.available() >= 2) 
  {
    byte buffer[512];
    memcpy(buffer, queue1.readBuffer(), 256);
    queue1.freeBuffer();
    memcpy(buffer + 256, queue1.readBuffer(), 256);
    queue1.freeBuffer();
    // write all 512 bytes to the SD card
    fptr.write(buffer, 512);
  }
}

void stopRecording() 
{
  queue1.end();
  while (queue1.available() > 0) 
  {
    fptr.write((byte*)queue1.readBuffer(), 256);
    queue1.freeBuffer();
  }
  fptr.close();
  delay(100);
}

// Call back to stop recording.
void timerCB() 
{
  waiting = 1;
}

void blinkLED(int led, int blinks, int period)
{
    for(int i = blinks; i > 0; i--)
    {
      digitalWrite(led, HIGH);
      delay((int)(period / 2));
      digitalWrite(led, LOW);
      delay((int)(period / 2));
    }
}

// Pull GPIO 5,4,3,2 after powering using GPIO 9.
// Choose configuration based on pin states.
void bootConfig()
{
  // Turn on the pull-up
  digitalWrite(bootConfig_power, HIGH);
  delay(100);

  // Pin 2 choose 1 = MIC or 0 = LINE IN

  if(digitalRead(bootConfig_2) == 0) 
  {
    myInput = AUDIO_INPUT_LINEIN;
    input_type = 1;
    blinkLED(userLED, 4, 250);
  }
  else
  {
    myInput = AUDIO_INPUT_MIC;
    input_type = 0;
    blinkLED(userLED, 2, 250);
  }
  // Set the input
  sgtl5000_1.inputSelect(myInput);
  delay(1000); // Delay for user

  // Pin 0 and Pin 1 form binary code to select gain
  int gain = 0;
  gain = gain | digitalRead(bootConfig_0);
  gain = gain | (digitalRead(bootConfig_1) << 1);

  switch(gain)
  {
    case 0:
        sgtl5000_1.micGain(40); // 40db
        sgtl5000_1.lineInLevel(15, 15); // 15: 0.24 Volts p-p
        gain_level = 4;
        blinkLED(userLED, gain_level, 250);
      break;
    case 1:
        sgtl5000_1.micGain(20); // 20db
        sgtl5000_1.lineInLevel(5, 5);//  5: 1.33 Volts p-p
        gain_level = 2;
        blinkLED(userLED, gain_level, 250);
      break;
    case 2:  
        sgtl5000_1.micGain(30); // 30db
        sgtl5000_1.lineInLevel(10, 10);// 10: 0.56 Volts p-p
        gain_level = 3;
        blinkLED(userLED, gain_level, 250);
      break;
    case 3:
        sgtl5000_1.micGain(0); // 0db
        sgtl5000_1.lineInLevel(0, 0);//  0: 3.12 Volts p-p
        gain_level = 1;
        blinkLED(userLED, gain_level, 250);
      break;
    default:    
        sgtl5000_1.micGain(0);
        sgtl5000_1.lineInLevel(0, 0);
        blinkLED(userLED, 1, 250);
      break;
  }

 // Pin 3 selects mode of operation.
  if(digitalRead(bootConfig_3) == 0)
  {
    sgtl5000_1.volume(1);
    passThroughMode();
  }
  else
  {
      sgtl5000_1.volume(0);
      sgtl5000_1.muteLineout();
      sgtl5000_1.muteHeadphone();
      
  }
  // Turn off the pull-up
  digitalWrite(bootConfig_power, LOW);
}


void passThroughMode()
{
  digitalWrite(userLED, HIGH);
  AudioOutputI2S i2s2;     
  AudioConnection patchCord1(i2s1, 0, i2s2, 0);
  AudioConnection patchCord2(i2s1, 0, i2s2, 1);
  while(1);
}

// The sleep routines found below were taken from the below link.
// https://forum.pjrc.com/index.php?threads/audio-library-wakes-up-teensy-from-hibernate.45034/post-345963

#define SNVS_LPCR_LPTA_EN_MASK          (0x2U)    ///< mask to put MCU to hibernate

// see also https://forum.pjrc.com/threads/58484-issue-to-reporogram-T4-0?highlight=hibernate
/**
  * @brief Set the Wakeup Call object
  *
  * @param nsec number of seconds to sleep
  */
void setWakeupCall(uint32_t nsec)
{
  uint32_t tmp = SNVS_LPCR; // save control register

  SNVS_LPSR |= 1;

  // disable alarm
  SNVS_LPCR &= ~SNVS_LPCR_LPTA_EN_MASK;
  while (SNVS_LPCR & SNVS_LPCR_LPTA_EN_MASK);

  __disable_irq();

  //get Time:
  uint32_t lsb, msb;
  do {
    msb = SNVS_LPSRTCMR;
    lsb = SNVS_LPSRTCLR;
  } while ( (SNVS_LPSRTCLR != lsb) | (SNVS_LPSRTCMR != msb) );
  uint32_t secs = (msb << 17) | (lsb >> 15);

  //set alarm
  secs += nsec;
  SNVS_LPTAR = secs;
  while (SNVS_LPTAR != secs);

  // restore control register and set alarm
  SNVS_LPCR = tmp | SNVS_LPCR_LPTA_EN_MASK;
  while (!(SNVS_LPCR & SNVS_LPCR_LPTA_EN_MASK));

  __enable_irq();
}

/**
  * @brief Shut down Tessnsy
  *
  */
void powerDown(void)
{
  SNVS_LPCR |= (1 << 6); // turn off power
  while (1) asm("wfi");     
}

/**
  * @brief Set the Wakeup Call and Sleep object
  *
  * @param nsec number of seconds to sleep
  */
void setWakeupCallandSleep(uint32_t nsec)
{
  setWakeupCall(nsec);
  powerDown();
}

time_t getTeensy3Time() 
{
  return Teensy3Clock.get();
}