# The Radionics MIDI Interface

A custom-engineered MIDI controller repurposed from a radionics "energy alignment" device.

## The Concept

Instead of mass-produced plastic, this controller uses a flea-market-find enclosure featuring 9 original 50K potentiometers. By integrating an ATmega32U4, I've transformed a relic of historical pseudoscience into a high-fidelity digital instrument for live audio performance.

## Hardware Specs

* **Microcontroller:** ATmega32U4 (Arduino Leonardo / Pro Micro architecture)
* **Interface:** 9x 50K Analog Potentiometers
* **Connectivity:** USB Class-Compliant MIDI
* **Compatibility:** Linux/Raspberry Pi (MODEP/Jack/ALSA/Patchbox OS)

## Architecture

This build utilizes native USB MIDI, bypassing the need for serial-to-MIDI bridging. The 9 potentiometers map directly to analog-to-digital converter (ADC) channels, providing granular control over real-time effects chains.

## Firmware

The custom C++ firmware implements an Exponential Moving Average (EMA) filter to suppress physical and thermal noise from the vintage component tracks, alongside a dynamic auto-calibration loop that scales the 10-bit raw ADC bounds perfectly to smooth 7-bit MIDI CC values.

```cpp
#include "MIDIUSB.h"

// Hardware Configuration - Exact Leonardo/32U4 Pinout Mapping
// Skips Digital 7 (No ADC) and preserves A4/A5 for I2C lines.
const int potPins[] = {A0, A1, A2, A3, A6, A7, A8, A9, A10};
const int NUM_POTS = 9;

// Filtering & Calibration Parameters
const float EMA_ALPHA = 0.25;    // Smoothing factor (0.1 = heavy filtering, 1.0 = raw)
float filteredValues[NUM_POTS];  // Smooth rolling average array
int lastMidiValues[NUM_POTS];    // Cached 7-bit MIDI positions to prevent data flooding

// Dynamic Range Bounds (Initialized wide open to prevent boot lockouts)
int potMin[NUM_POTS];
int potMax[NUM_POTS];

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

void setup() {
  // Prime baseline readings and open up default calibration limits
  for (int i = 0; i < NUM_POTS; i++) {
    int initialRead = analogRead(potPins[i]);
    filteredValues[i] = initialRead;
    lastMidiValues[i] = -1; 
    
    potMin[i] = 1023; // Set high so any real reading lowers it
    potMax[i] = 0;    // Set low so any real reading raises it
  }
}

void loop() {
  for (int i = 0; i < NUM_POTS; i++) {
    int rawValue = analogRead(potPins[i]);

    // 1. Dynamic Auto-Calibration Tracking
    if (rawValue < potMin[i]) potMin[i] = rawValue;
    if (rawValue > potMax[i]) potMax[i] = rawValue;

    // Safety check to avoid division by zero if a pot hasn't moved
    if (potMax[i] == potMin[i]) continue;

    // 2. Exponential Moving Average (EMA) Digital Low-Pass Filter
    filteredValues[i] = (EMA_ALPHA * rawValue) + ((1.0 - EMA_ALPHA) * filteredValues[i]);

    // 3. Map the smoothed 10-bit range to 7-bit MIDI CC (0-127)
    int constrainedRaw = constrain((int)filteredValues[i], potMin[i], potMax[i]);
    int midiValue = map(constrainedRaw, potMin[i], potMax[i], 0, 127);

    // 4. Output matching only on true value mutations (Hysteresis/Noise Shield)
    if (midiValue != lastMidiValues[i]) {
      lastMidiValues[i] = midiValue;
      controlChange(0, i + 1, midiValue); // Channels mapped CC 1 through 9
    }
  }
  
  MidiUSB.flush(); // Consolidated single packet USB push
  delay(5);        // Low-latency loop timing
}
