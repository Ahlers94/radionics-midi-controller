# Contributing to the Radionics MIDI Interface

Thank you for checking out the project! This instrument is an open-source experiment in turning vintage hardware enclosures into modern, class-compliant USB MIDI controllers. 

We welcome contributions of all kinds—from code optimizations to hardware modifications.

---

## 🛠 Hardware Configuration & Wiring

The core of this project runs on an **Arduino Leonardo (ATmega32U4)** to utilize its native USB HID capabilities. If you are building your own version or modifying the potentiometer layout, please note the following hardware constraints:

### Potentiometer Mapping
* **Analog Inputs:** By default, the potentiometers map to the standard analog array. If you change or expand the knobs, update the `potPins` array inside `controller.ino`.
* **The Digital 7 Constraint:** Physical Pin 7 on the Arduino Leonardo does *not* route to an internal Analog-to-Digital Converter (ADC) channel. If you are wiring 9 knobs, ensure none are tied to Digital 7 for analog reading; utilize pins like **4 (A6)** or **12 (A11)** instead.

---

## 🔮 Future Roadmap & Experimental Features

The enclosure features several legacy components that are currently isolated or strictly aesthetic. We highly encourage contributors to experiment with bringing these elements online:

### 1. Capacitive Touch (Antenna & Crystal Array)
The external antenna and crystal array are currently passive. Future contributors are encouraged to wire these elements to digital pins using the `CapacitiveSensor` library to trigger:
* MIDI Note On/Off commands (acting as an expressive theremin-style touch plate).
* MIDI CC modulation layers.

### 2. Audio Pass-Through & Sync (3.5mm Jacks)
The 3.5mm input and output jacks are currently unlinked. Ideas for expansion include:
* Wiring the jacks to digital lines for hardware LED synchronization or CV (Control Voltage) integration.
* Routing basic analog expression elements through the jacks.

---

## 💻 Software Contributions

### Code Style & Filtering
This project utilizes an **Exponential Moving Average (EMA)** filter combined with a dynamic auto-calibration window to handle older, high-resistance (e.g., 50k) vintage carbon track potentiometers. 

When modifying the code:
* Keep filtering efficiency in mind to prevent latent lag in the MIDI stream.
* Ensure the auto-calibration constraints (`potMin` and `potMax`) initialize with wide-open defaults to prevent endpoint lockouts on boot.

### How to Submit a Pull Request
1. Fork the repository.
2. Create a feature branch (`git checkout -b feature/AmazingNewFeature`).
3. Commit your changes with clear, descriptive messages (`git commit -m 'Add capacitive antenna support'`).
4. Push to the branch (`git push origin feature/AmazingNewFeature`).
5. Open a Pull Request detailing your changes, schematic adjustments, or code optimizations.
