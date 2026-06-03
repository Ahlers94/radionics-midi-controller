#include "MIDIUSB.h"
#include <avr/wdt.h>

// ─── WATCHDOG BOOTLOADER SAFETY VALVE ────────────────────────────────────────
// Clears the watchdog status immediately upon MCU power-up. This prevents the
// standard Caterina/Pro Micro bootloader from getting trapped in an infinite
// reset loop when the watchdog fires upon a USB disconnect.
void disableWatchdogOnBoot(void) __attribute__((naked)) __attribute__((section(".init3")));
void disableWatchdogOnBoot(void) {
  MCUSR = 0;
  wdt_disable();
}

// ─── PIN CONFIGURATION ────────────────────────────────────────────────────────
// Adjusted for your hardware fix: Pin 7 completely removed. 
// Digital Pin 4 (A6) and Digital Pin 6 (A7) added.
// Array Index:     0   1   2   3   4   5   6   7   8
// Physical Label: A0, A1, A2, A3,  4,  6,  8,  9, 10
const int potPins[9] = {A0, A1, A2, A3, A6, A7, A8, A9, A10};

// MIDI CC Assignments (1-indexed for MODEP MIDI learn convenience)
const byte midiCC[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
const byte midiChannel = 0; // MIDI Channel 1

// ─── TUNING CONSTANTS ─────────────────────────────────────────────────────────
const int   NUM_POTS          = 9;
const int   OVERSAMPLE_BITS   = 4;       // 16 samples averaged per read loop
const int   SETTLE_US         = 250;     // Time for internal MUX capacitor to drain voltage
const int   INIT_WINDOW       = 80;      // Cold-start baseline safety window for vintage 50k pots
const int   DEADBAND_COUNTS   = 35;      // Cuts out physical line jitter at the 0 and 5V track limits
const int   HYSTERESIS        = 2;       // Required change in MIDI steps to broadcast a new value
const int   LOOP_DELAY_MS     = 15;      
const int   SMOOTH_SHIFT      = 3;       // EMA Smoothing Weight (Higher = cleaner signal, more lag)

// ─── INTERNAL STATE ──────────────────────────────────────────────────────────
int  potMin[NUM_POTS];
int  potMax[NUM_POTS];
int  lastMidiValues[NUM_POTS];
long smoothed[NUM_POTS];        
bool initialized[NUM_POTS];

void initUSBWatchdog() {
  wdt_enable(WDTO_2S); // 2-second heartbeat
}

void kickUSBWatchdog() {
  // Natively monitors the 32U4 USB core status. If the cable is pulled from the 
  // Raspberry Pi, USBDevice.configured() drops or the Suspend Interrupt (SUSPI)
  // flag goes high. We stop kicking the watchdog, letting it reset the chip so 
  // it instantly re-enumerates when plugged back into Patchbox OS.
  if (USBDevice.configured() && !(UDINT & (1 << SUSPI))) {
    wdt_reset(); 
  }
}

// ─── HARDWARE STABILIZED ADC READ ────────────────────────────────────────────
int getStabilizedRead(int pin) {
  // Discard read: Force internal MUX pointer update and dump stale track charge
  analogRead(pin);
  delayMicroseconds(SETTLE_US);
  
  long total = 0;
  int  count = (1 << OVERSAMPLE_BITS);
  for (int i = 0; i < count; i++) {
    total += analogRead(pin);
    delayMicroseconds(25); // Brief delay cuts correlated high-frequency rail noise
  }
  return (int)(total >> OVERSAMPLE_BITS);
}

// ─── FIXED EXPONENTIAL MOVING AVERAGE ────────────────────────────────────────
int applyEMA(int idx, int rawValue) {
  if (!initialized[idx]) {
    smoothed[idx] = (long)rawValue << SMOOTH_SHIFT;
    initialized[idx] = true;
  } else {
    // Parentheses explicitly grouped to prevent operator precedence math cancellation
    smoothed[idx] = smoothed[idx] + ((((long)rawValue << SMOOTH_SHIFT) - smoothed[idx]) >> SMOOTH_SHIFT);
  }
  return (int)(smoothed[idx] >> SMOOTH_SHIFT);
}

void sendCC(byte channel, byte cc, byte value) {
  midiEventPacket_t event = {0x0B, (byte)(0xB0 | channel), cc, value};
  MidiUSB.sendMIDI(event);
}

void setup() {
  initUSBWatchdog();
  
  // 1. Force headers into high-impedance input mode
  for (int i = 0; i < NUM_POTS; i++) {
    pinMode(potPins[i], INPUT);
  }
  
  // 2. Initialize internal state variables
  for (int i = 0; i < NUM_POTS; i++) {
    initialized[i] = false;
    lastMidiValues[i] = -1;
    
    // Warm up the ADC MUX
    analogRead(potPins[i]);
    delayMicroseconds(500);
    
    int baseline = getStabilizedRead(potPins[i]);
    applyEMA(i, baseline); 
    
    // WIDE-OPEN DEFAULT CALIBRATION: Eliminates endpoint lockouts entirely
    // This forces the loop to accept the full range immediately on boot
    potMin[i] = 150;  
    potMax[i] = 870;  
  }
}

void loop() {
  kickUSBWatchdog();
  
  for (int i = 0; i < NUM_POTS; i++) {
    int raw     = getStabilizedRead(potPins[i]);
    int smoothV = applyEMA(i, raw);
    
    // 1. Expand calibration bounds dynamically when pots are turned
    if (smoothV < potMin[i]) potMin[i] = smoothV;
    if (smoothV > potMax[i]) potMax[i] = smoothV;
    
    // 2. FORGIVING SAFETY GATE: Prevents divide-by-zero but allows raw signals through
    int currentSpan = potMax[i] - potMin[i];
    if (currentSpan < 10) currentSpan = 10; 
    
    // 3. Compute active mapping windows safely
    int activeMin = potMin[i] + DEADBAND_COUNTS;
    int activeMax = potMax[i] - DEADBAND_COUNTS;
    if (activeMax <= activeMin) activeMax = activeMin + 1;
    
    int midiVal = map(smoothV, activeMin, activeMax, 0, 127);
    midiVal = constrain(midiVal, 0, 127);
    
    // 4. Hysteresis & Endpoint Snapping
    bool endpointSnap = (midiVal == 0 || midiVal == 127);
    bool movedEnough  = (abs(midiVal - lastMidiValues[i]) >= HYSTERESIS);
    
    if (midiVal != lastMidiValues[i] && (movedEnough || endpointSnap)) {
      lastMidiValues[i] = midiVal;
      sendCC(midiChannel, midiCC[i], (byte)midiVal);
    }
  }
  
  MidiUSB.flush();
  delay(LOOP_DELAY_MS);
}
