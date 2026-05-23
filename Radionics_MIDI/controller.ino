#include "MIDIUSB.h"

// Configuration
const int potPins[] = {A0, A1, A2, A3, A6, A7, A8, A9, A10};
int lastMidiValues[9];       

// Calibration arrays
int potMin[9];
int potMax[9];

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

// Hardened reader designed to fight ATmega32U4 ADC Cross-Talk
int getHardwareStabilizedRead(int pin) {
  // 1. DISCARD READ: Force the internal MUX to switch to the pin, 
  // then immediately throw away the reading. This lets the residual voltage drain.
  analogRead(pin);
  delayMicroseconds(100); // Give the internal sample-and-hold capacitor time to settle

  // 2. OVERSAMPLING: Take 8 fresh samples and average them to kill thermal line noise
  long total = 0;
  for (int i = 0; i < 8; i++) {
    total += analogRead(pin);
  }
  return total >> 3; 
}

void setup() {
  for (int i = 0; i < 9; i++) {
    int baseline = getHardwareStabilizedRead(potPins[i]);
    potMin[i] = baseline - 30; // Widened initial safety floor for vintage drift
    potMax[i] = baseline + 30; // Widened initial safety ceiling
    if (potMin[i] < 0) potMin[i] = 0;
    if (potMax[i] > 1023) potMax[i] = 1023;
    
    lastMidiValues[i] = -1; 
  }
}

void loop() {
  for (int i = 0; i < 9; i++) {
    int rawValue = getHardwareStabilizedRead(potPins[i]);

    // Dynamic Calibration learning window
    if (rawValue < potMin[i]) potMin[i] = rawValue;
    if (rawValue > potMax[i]) potMax[i] = rawValue;

    if (potMax[i] == potMin[i]) continue;

    // Deadbanding: Pad the boundaries (~3% of range) to swallow noisy endpoints
    int activeMin = potMin[i] + 25; 
    int activeMax = potMax[i] - 25; 

    // Map and clamp to clean MIDI bounds
    int currentMidiValue = map(rawValue, activeMin, activeMax, 0, 127);
    currentMidiValue = constrain(currentMidiValue, 0, 127); 

    // Value Hysteresis: Require a clear 2-unit change to push past line noise,
    // but ALWAYS allow immediate access to absolute 0 or 127.
    if (currentMidiValue != lastMidiValues[i]) {
      if (abs(currentMidiValue - lastMidiValues[i]) > 1 || currentMidiValue == 0 || currentMidiValue == 127) {
        lastMidiValues[i] = currentMidiValue;
        controlChange(0, i + 1, currentMidiValue);
      }
    }
  }
  
  MidiUSB.flush(); 
  delay(15); // Increased macro delay to completely stabilize the power rail between full loops
}
