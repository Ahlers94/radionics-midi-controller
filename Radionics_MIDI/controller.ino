#include "MIDIUSB.h"

// Configuration
const int potPins[] = {A0, A1, A2, A3, A6, A7, A8, A9, A10};
int lastMidiValues[9];       // Track the last sent 7-bit MIDI value

// Calibration arrays
int potMin[9];
int potMax[9];

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

// Oversampling filter: averages 8 samples to flatten raw noise spikes
int getSmoothedRead(int pin) {
  long total = 0;
  for (int i = 0; i < 8; i++) {
    total += analogRead(pin);
  }
  return total >> 3; 
}

void setup() {
  for (int i = 0; i < 9; i++) {
    int baseline = getSmoothedRead(potPins[i]);
    potMin[i] = baseline - 20; 
    potMax[i] = baseline + 20; 
    if (potMin[i] < 0) potMin[i] = 0;
    if (potMax[i] > 1023) potMax[i] = 1023;
    
    lastMidiValues[i] = -1; 
  }
}

void loop() {
  for (int i = 0; i < 9; i++) {
    int rawValue = getSmoothedRead(potPins[i]);

    // 1. Dynamic Calibration
    if (rawValue < potMin[i]) potMin[i] = rawValue;
    if (rawValue > potMax[i]) potMax[i] = rawValue;

    if (potMax[i] == potMin[i]) continue;

    // 2. Deadbanding: Add a software buffer zone (~2% of 10-bit range) to the endpoints
    int activeMin = potMin[i] + 15;
    int activeMax = potMax[i] - 15;

    // 3. Map to MIDI using the padded active range
    int currentMidiValue = map(rawValue, activeMin, activeMax, 0, 127);
    currentMidiValue = constrain(currentMidiValue, 0, 127); // Clamps noisy endpoints strictly to 0 or 127

    // 4. Value Hysteresis: Only transmit if the change is stable and distinct
    if (currentMidiValue != lastMidiValues[i]) {
      // Prevent rapid oscillation back and forth between two adjacent integers
      if (abs(currentMidiValue - lastMidiValues[i]) > 1 || currentMidiValue == 0 || currentMidiValue == 127) {
        lastMidiValues[i] = currentMidiValue;
        controlChange(0, i + 1, currentMidiValue);
      }
    }
  }
  
  MidiUSB.flush(); 
  delay(10); 
}
