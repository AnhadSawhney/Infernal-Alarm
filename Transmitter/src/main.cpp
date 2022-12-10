#include <Arduino.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define BUTTON 8
#define LED 18
#define HOLD_TIME_S 5
#define TIME_BETWEEN_TRANSMISSIONS 10
#define RETRIES 3

const int pinCE = 9; //This pin is used to set the nRF24 to standby (0) or active mode (1)
const int pinCSN = 10; //This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out
byte counter = 0; //used to count the packets sent
RF24 radio(pinCE, pinCSN); // Create your nRF24 object or wireless SPI connection
const uint64_t aAddress = 0xB00B1E5000LL;  //Create a pipe addresses for the 2 nodes to communicate over, the "LL" is for LongLong type
const uint64_t bAddress = 0xB00B1E6000LL;  //Create a pipe addresses for the 2 nodes to communicate over, the "LL" is for LongLong type
bool flash = false;
bool lightstate = false;
bool go = false; // ready for button to be pressed
unsigned long transmission_sent_at = 0, flash_start = 0;

void setup() {
  Serial.begin(9600);   //start serial to communicate process
  radio.begin();            //Start the nRF24 module
  radio.setAutoAck(1);                    // Ensure autoACK is enabled so rec sends ack packet to let you know it got the transmit packet payload
  radio.enableAckPayload();               // Allow optional ack payloads
  radio.setPALevel(RF24_PA_HIGH);
  radio.openWritingPipe(aAddress);        // pipe address that we will communicate over, must be the same for each nRF24 module
  radio.openReadingPipe(1, bAddress);
  radio.startListening();  
  randomSeed(analogRead(0));    //use random ADC value to seed random number algorithm
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
}

void end_flash() {
  flash = false;
  digitalWrite(LED, LOW);
  go = false;
}

void loop() {
  if (digitalRead(BUTTON) == LOW && !go) {
    int hz = 2;
    unsigned long t = millis();
    while(digitalRead(BUTTON) == LOW) {
      lightstate = !lightstate;
      digitalWrite(LED, lightstate);
      delay(1000/hz);
      hz += 1;
      if (millis() - t > (HOLD_TIME_S * 1000)) {
        go = true;
        break;
      }
    }
    digitalWrite(LED, LOW);

    if (go) { // completed successfully
      transmission_sent_at = millis();
      radio.stopListening();        //transmitter so stop listening for data
      int r = 0;
      while (!radio.write("G", 1 ) && r < RETRIES) { //send the packet
        Serial.print("packet delivery failed. Retrying ");
        Serial.print(RETRIES-r-1);
        Serial.println(" times");
        r++;
      }
      if (r == RETRIES) {
        go = false;
      }
      radio.startListening();
    }
  }

  if (millis() - transmission_sent_at > TIME_BETWEEN_TRANSMISSIONS * 1000) {
    go = false;
  }

  while(radio.available()) {
    char t;
    radio.read(&t, 1);
    Serial.print("Got: ");
    Serial.println(t);
    if(t == 'S') {
      flash = true;
      flash_start = millis();
    }
    else if(t == 'T') {
      end_flash();
    }
  }

  if(flash) {
    lightstate = !lightstate;
    digitalWrite(LED, lightstate);
    delay(100);
    if(millis() - flash_start > 20000) {
      end_flash();
    }
  }
}