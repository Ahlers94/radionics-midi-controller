#include "MIDIUSB.h"

// Configuration
const int potPins[] = {A0, A1, A2, A3, A6, A7, A8, A9, A10};
const int RAW_DEADZONE = 6; // Stable threshold on the 10-bit scale (0-1023)
int lastPotValues[9];       // Initialized dynamically in setup()

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

void setup() {
  // Prime the array with actual baseline readings on boot.
  // This prevents a sudden burst of ghost MIDI messages when plugging the box into USB.
  for (int i = 0; i < 9; i++) {
    lastPotValues[i] = analogRead(potPins[i]);
  }
}

void loop() {
  for (int i = 0; i < 9; i++) {
    int rawValue = analogRead(potPins[i]);

    // Hysteresis comparison on the raw 10-bit sensor data
    if (abs(rawValue - lastPotValues[i]) > RAW_DEADZONE) { 
      
      // Cache the new raw baseline immediately
      lastPotValues[i] = rawValue;
      
      // Compress 0-1023 down to 0-127 via bit shifting (drops 3 least significant bits)
      int midiValue = rawValue >> 3; 
      
      // Send CC message (Channel 0, Controller numbers 1 through 9)
      controlChange(0, i + 1, midiValue);
    }
  }
  
  // Single global flush updates the hardware buffer efficiently
  MidiUSB.flush(); 
  delay(5); 
}
