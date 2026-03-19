# **Smart Home Controller Phase 2: Decoupled Control & Wi-Fi Integration**

**Author:** Project 2 Group

**Institution:** Higher Institute of Computer Science and Information Technology – El Shorouk Academy

**Supervisor:** TA. Ahmed Mahfouz

**Environment:** Windows 11, VS Code

**Core Technology:** Python (pyserial, socket), TCP/IP Wi-Fi

## **1\. Phase 2 Abstract**

This document details the second phase of the VLSI Smart Home Controller Proof of Concept (PoC). Building upon the hardware foundations established in Phase 1, this iteration introduces a massive architectural upgrade: **Network Standardization** via TCP/IP Wi-Fi, and a **Decoupled State-Monitoring** approach for hybrid lighting control.

Additionally, this document outlines the theoretical design for an *upcoming* **Fail-Safe Dual-Power** architecture (Hardware OR Gate) that will simulate the robust, fault-tolerant topology of commercial, enterprise-grade IoT smart switches.

## **2\. Major Architectural Upgrades (Phase 1 vs. Phase 2\)**

### **2.1 The Network Shift: Bluetooth to Wi-Fi (TCP/IP)**

Phase 1 utilized an HC-06 Bluetooth module for direct point-to-point serial communication. Phase 2 eliminates this physical module entirely, replacing it with a software-defined **Raw TCP/IP Socket** (server\_socket in Python) hosted over a local Wi-Fi network.

**Engineering Advantages of Wi-Fi for Smart Homes:**

* **Industrial Standard:** Real-world industrial and commercial IoT products rely on Wi-Fi protocols, not Bluetooth Classic.  
* **Wider Coverage:** Wi-Fi routes through the home's primary network infrastructure, allowing control from anywhere in the building.  
* **Higher Bandwidth & Reliability:** TCP/IP provides robust packet delivery verification and handles multiple simultaneous connections effortlessly.

### **2.2 Decoupled State Monitoring & The Upcoming "Fail-Safe" Requirement**

In the current Phase 2 decoupled design, the physical button only sends a signal to the smart controller via sensor pins (CTS, DSR). The software then commands the LED to turn on via logic pins (DTR, RTS). However, **this introduces a critical single point of failure:** if the Python server crashes or the PC reboots, there is no way to control the lights manually.

To solve this in future iterations, the architecture proposes a **Dual-Power Source Architecture**:

1. **Logical Source:** Controlled by the smart controller app via USB-to-TTL logic pins (DTR and RTS).  
2. **Physical Source:** Powered directly from the 5V and GND pins of the USB-to-TTL module, controlled by physical tactile wall buttons.

Through this upcoming **State Monitoring Approach**, the system will tap the sensor pins into the physical button circuit. When a user presses the physical button, the code will instantly detect the hardware state change, while logical state changes remain monitored directly within the software.

### **2.3 \[Upcoming Feature\] The Hardware OR Gate: Mandatory Diode Protection**

When the dual-power system is implemented, the LED could be powered by either the physical button (5V) or the logic pin (DTR/RTS). This introduces a severe hardware risk: if one power source is LOW (0V) and the other is HIGH (5V), current will flow backward into the LOW pin, permanently damaging the USB-to-TTL module.

To prevent this, the future architecture **MUST use 2 Diodes for each LED** (one for each power source). The diodes will force the current to flow in only one direction. They will act as electronic one-way valves, isolating the two power sources from each other and protecting the CP2102 chip from any reverse-current damage.

## **3\. Software Logic & Implementation**

### **3.1 The Python Server Socket**

The core of the communication runs on Python's built-in socket library. The script binds to 0.0.0.0, allowing the PC to act as a local web server. It uses non-blocking socket reads to ensure the hardware polling loop isn't paused while waiting for network data, enabling instant disconnect detection and graceful error handling.

### **3.2 Dynamic Logging**

Every state change generates a formatted log string containing the targeted LED, its new state, and the source of the command. This ensures perfect synchronization between the physical room and the mobile dashboard:

LED 1 state:ON \- last used : (logical app)

LED 1 state:OFF \- last used : (physical button)

## **4\. Bill of Materials (BOM)**

### **Hardware Requirements**

* **1x Breadboard:** Central hub for physical circuitry.  
* **1x CP2102 USB-to-TTL Module:** Bridges the PC to the physical world and provides 5V power.  
* **2x Tactile Push Buttons:** Acts as the physical wall switches.  
* **2x LEDs:** Simulates the room's main lighting.  
* **4x Standard Logic Diodes:** (e.g., 1N4148 or 1N4007) *(Note: Mandatory for the upcoming Hardware OR Gate protection; planned for next phase).*  
* **Assorted Resistors:** 10kΩ pull-down resistors for the active-high sensor pins, and 330Ω current-limiting resistors for the LEDs.  
* **Jumper Wires**

### **Software Requirements**

* **Environment:** Windows 11 running Visual Studio Code (VS Code).  
* **Backend Server:** Python 3 (pyserial for hardware interfacing, socket for Wi-Fi communication).  
* **Mobile Interface:** **Serial WiFi Terminal** (Android application by Kai Morich). Configured to use a "Raw" protocol connection aimed at the host PC's IPv4 address.