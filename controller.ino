#include "MIDIUSB.h"

// Configuration
// These constants explicitly point the Arduino core to the ADC channels
const int potPins[] = {A0, A1, A2, A3, A6, A7, A8, A9, A10};
const int DEADZONE = 2; // Prevents "MIDI jitter"
int lastPotValues[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

void setup() {}

void loop() {
  for (int i = 0; i < 9; i++) {
    int rawValue = analogRead(potPins[i]);
    int midiValue = rawValue / 8; // Scale to 0-127

    if (abs(midiValue - lastPotValues[i]) > DEADZONE) {
      controlChange(0, i + 1, midiValue);
      MidiUSB.flush();
      lastPotValues[i] = midiValue;
    }
  }
  delay(5); 
}
