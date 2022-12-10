#include <Arduino.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Adafruit_NeoPixel.h>
#include <PinChangeInterrupt.h>

#include "pitches.h"

#define SPKR 18
#define LED 19
#define NUMPIXELS 9

Adafruit_NeoPixel pixels(NUMPIXELS, LED, NEO_GRB + NEO_KHZ800);

#define CE 9 //This pin is used to set the nRF24 to standby (0) or active mode (1)
#define CSN 10 //This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out
byte gotByte = 0; //used to store payload from transmit module
volatile int count = 0; //tracks the number of interrupts from IRQ
int pCount = 0; //tracks what last count value was so know when count has been updated
RF24 radio(CE, CSN); // Declare object from nRF24 library (Create your wireless SPI) 
const uint64_t aAddress = 0xB00B1E5000LL;  //Create a pipe addresses for the 2 nodes to communicate over, the "LL" is for LongLong type
const uint64_t bAddress = 0xB00B1E6000LL;  //Create a pipe addresses for the 2 nodes to communicate over, the "LL" is for LongLong type
//bool alarm = false;
unsigned long alarm_time = 0;

//This is the function called when the interrupt occurs (pin 2 goes high)
//this is often referred to as the interrupt service routine or ISR
//This cannot take any input arguments or return anything
/*void interruptFunction() {
    count++; //up the receive counter
    Serial.println("ISR");
    while(radio.available()) { //get data sent from transmit
        radio.read( &gotByte, 1 ); //read one byte of data and store it in gotByte variable
        Serial.print("Got: ");
        Serial.println(gotByte);
    }
    alarm = true;
}*/

void sound_alarm() {
    radio.stopListening();
    if (!radio.write("S", 1 )) { 
        Serial.println("packet delivery failed: S");  
    }
    int p = 0;
    for(int i = 0; i < 100; i++) {
        pixels.clear();
        for(int j=0; j<(NUMPIXELS*(100-i)/100); j++) {
            pixels.setPixelColor(j, pixels.Color(255, 0, 0));
        }
        pixels.show();
        p++;
        if(p == NUMPIXELS+1) {
            p = 0;
        }
        pixels.show();
        tone(SPKR, NOTE_B5, 100);
        delay(100);
        pixels.setPixelColor(NUMPIXELS-1, pixels.Color(255, 255, 255)); // last one is the middle
        pixels.show();
        tone(SPKR, NOTE_B6, 100);
        delay(100);
    }
    noTone(SPKR);
    pixels.clear();
    pixels.show();
    if (!radio.write("T", 1 )) { 
        Serial.println("packet delivery failed: T");  
    }
    radio.startListening();  
}

// choose a random time between 5 and 8 and hours after the current time in millis()
void generate_next_alarm_time() {
    unsigned long current_time = millis();
    unsigned long next_alarm_time = random(5*60, 8*60) * 60 * 1000;
    alarm_time = next_alarm_time + current_time;
    Serial.print("Next alarm in (ms): ");
    Serial.println(next_alarm_time);
}

void setup()   {
  radio.begin();  //Start the nRF24 module
  radio.setAutoAck(1);                    // Ensure autoACK is enabled so rec sends ack packet to let you know it got the transmit packet payload
  radio.enableAckPayload();         //allows you to include payload on ack packet
  //radio.maskIRQ(1,1,0);               //mask all IRQ triggers except for receive (1 is mask, 0 is no mask)
  radio.setPALevel(RF24_PA_HIGH); //Set power level to low, won't work well at higher levels (interfer with receiver)
  radio.openWritingPipe(bAddress);
  radio.openReadingPipe(1, aAddress);      //open pipe o for recieving meassages with pipe address
  radio.startListening();                 // Start listening for messages
  //attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(8), interruptFunction, FALLING);
  randomSeed(analogRead(0));    //use random ADC value to seed random number algorithm
  pixels.begin();
  pixels.clear();
  pixels.show();
  Serial.begin(9600);
  // initialize the random seed with analog read on pin 0
  randomSeed(analogRead(0));
  generate_next_alarm_time();
}

void loop() {
   /*if(pCount < count) { //If this is true it means count was increased and another interrupt occurred
         //start serial to communicate process
       Serial.print("Receive packet number ");
       Serial.println(count);
       Serial.end(); //have to end serial since it uses interrupts
       pCount = count;
   }*/
   pixels.clear();
   pixels.show();

   while(radio.available()) {
    char t;
    radio.read(&t, 1);
    Serial.print("Got: ");
    Serial.println(t);
    if(t == 'G') {
      sound_alarm();
      //alarm = true;
    }
  }

   /*if(alarm) {
       sound_alarm();
       alarm = false;
   }*/

   if(millis() > alarm_time) {
       generate_next_alarm_time();
       sound_alarm();
   }
}