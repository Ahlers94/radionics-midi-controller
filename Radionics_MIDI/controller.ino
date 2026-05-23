#include "MIDIUSB.h"

// Configuration
const int potPins[] = {A0, A1, A2, A3, A6, A7, A8, A9, A10};
int lastMidiValues[9];       // Track the last sent 7-bit MIDI value

// Calibration arrays to capture the actual physical range of your vintage pots
int potMin[9];
int potMax[9];

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

// Smooths out high-frequency noise by averaging multiple rapid readings
int getSmoothedRead(int pin) {
  long total = 0;
  for (int i = 0; i < 8; i++) { // Average 8 samples
    total += analogRead(pin);
  }
  return total >> 3; // Divide by 8 using bit-shifting
}

void setup() {
  // Initialize calibration limits based on baseline boots
  for (int i = 0; i < 9; i++) {
    int baseline = getSmoothedRead(potPins[i]);
    potMin[i] = baseline - 20; // Set initial safety floors
    potMax[i] = baseline + 20; // Set initial safety ceilings
    if (potMin[i] < 0) potMin[i] = 0;
    if (potMax[i] > 1023) potMax[i] = 1023;
    
    lastMidiValues[i] = -1; // Force initial update transmission
  }
}

void loop() {
  for (int i = 0; i < 9; i++) {
    int rawValue = getSmoothedRead(potPins[i]);

    // 1. Auto-Calibration: Dynamically learn the hardware limits as you twist the knobs
    if (rawValue < potMin[i]) potMin[i] = rawValue;
    if (rawValue > potMax[i]) potMax[i] = rawValue;

    // Prevent divide-by-zero if a pot isn't moving
    if (potMax[i] == potMin[i]) continue;

    // 2. Map the actual constrained hardware range directly to the full 0-127 MIDI range
    int currentMidiValue = map(rawValue, potMin[i], potMax[i], 0, 127);
    currentMidiValue = constrain(currentMidiValue, 0, 127); // Safety clamp

    // 3. State Check: Only send if the processed MIDI integer actually changes
    if (currentMidiValue != lastMidiValues[i]) {
      lastMidiValues[i] = currentMidiValue;
      controlChange(0, i + 1, currentMidiValue);
    }
  }
  
  MidiUSB.flush(); 
  delay(8); // Stabilizing window for the ADC multiplexer
}
