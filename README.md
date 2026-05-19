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

## License
MIT License - Feel free to hack, adapt, or build your own.