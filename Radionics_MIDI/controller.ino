#include "MIDIUSB.h"

// Configuration
const int potPins[] = {A0, A1, A2, A3, A6, A7, A8, A9, A10};
int lastMidiValues[9];       // Track the LAST SENT 7-bit MIDI value (0-127)

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

void setup() {
  // Prime the array with the initial 7-bit values on boot
  for (int i = 0; i < 9; i++) {
    int rawValue = analogRead(potPins[i]);
    lastMidiValues[i] = rawValue >> 3; // Convert 10-bit to 7-bit immediately
  }
}

void loop() {
  for (int i = 0; i < 9; i++) {
    int rawValue = analogRead(potPins[i]);
    
    // 1. Convert to 7-bit MIDI scale immediately
    int currentMidiValue = rawValue >> 3; 

    // 2. Only respond if the actual 7-bit MIDI value has moved
    if (currentMidiValue != lastMidiValues[i]) { 
      
      // Update our history cache
      lastMidiValues[i] = currentMidiValue;
      
      // Send CC message (Channel 0, Controller numbers 1 through 9)
      controlChange(0, i + 1, currentMidiValue);
    }
  }
  
  MidiUSB.flush(); 
  delay(10); // Slightly increased delay to give the ADC more settling time
}
