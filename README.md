# The Radionics MIDI Interface

A custom-engineered MIDI controller repurposed from a radionics "energy alignment" device.

## The Concept
Instead of mass-produced plastic, this controller uses a flea-market-find enclosure featuring 9 original 50K potentiometers. By integrating an ATmega32U4, I've transformed a relic of historical pseudoscience into a high-fidelity digital instrument for live audio performance.

## Hardware Specs
- **Microcontroller:** ATmega32U4 (Pro Micro)
- **Interface:** 9x 50K Analog Potentiometers
- **Connectivity:** USB Class-Compliant MIDI
- **Compatibility:** Linux/Raspberry Pi (MODEP/Jack/ALSA)

## Architecture
This build utilizes native USB MIDI, bypassing the need for serial-to-MIDI bridging. The 9 potentiometers map directly to analog-to-digital converter (ADC) channels, providing granular control over real-time effects chains.

## Firmware
The custom C++ firmware implements raw 10-bit hysteresis (deadzone logic) to suppress thermal noise from the vintage components before scaling the signal down to 7-bit MIDI CC values. 

```cpp
#include "MIDIUSB.h"

// Configuration
const int potPins[] = {A0, A1, A2, A3, A6, A7, A8, A9, A10};
const int RAW_DEADZONE = 6; // Stable threshold on the 10-bit scale (0-1023)
int lastPotValues[9];       // Cached raw positions

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

void setup() {
  // Prime baseline readings to prevent startup message bursts
  for (int i = 0; i < 9; i++) {
    lastPotValues[i] = analogRead(potPins[i]);
  }
}

void loop() {
  for (int i = 0; i < 9; i++) {
    int rawValue = analogRead(potPins[i]);

    // Hysteresis verification on raw ADC data
    if (abs(rawValue - lastPotValues[i]) > RAW_DEADZONE) { 
      lastPotValues[i] = rawValue;
      
      // Convert 10-bit raw data to 7-bit MIDI CC (0-127)
      int midiValue = rawValue >> 3; 
      controlChange(0, i + 1, midiValue);
    }
  }
  MidiUSB.flush(); // Consolidated single packet push
  delay(5); 
}
```

## License
MIT License - Feel free to hack, adapt, or build your own.
