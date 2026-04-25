 # **VLSI Smart Home Controller Phase 1: Proof of Concept (PoC)**
<details>
This repository represents the practical implementation phase of my academic course in **Very Large Scale Integration (VLSI)**. It serves as a Level 1 PoC, bridging the gap between discrete logic control and high-power physical systems.

## **🎓 Academic Context**

As part of my VLSI studies, this project explores the interfacing of low-voltage CMOS-level logic (from a host PC) with high-current power electronics. While VLSI focuses on the integration of millions of transistors, this level starts with the fundamental unit: the single NPN transistor switch, understood as a building block for larger integrated systems.  
**Roadmap:** This repo will be updated as the course progresses, transitioning from discrete components to integrated circuits and complex logic arrays.

## **🛠 Project Description**

Smart home automation systems can be implemented using a wide variety of communication protocols depending on the range, power consumption, and data requirements, including **Bluetooth, Wi-Fi, Zigbee, and Z-Wave**.  
This specific project is a hybrid-voltage control system that utilizes the **Bluetooth** protocol to allow a laptop (via Python) to independently control a low-power LED indicator and a high-power 7.4V DC Motor.

### **Key Features:**

* **Mixed-Voltage Logic:** 3.3V Logic and 7.4V Power sharing a Common Ground.  
* **Software Interface:** Python-based logic managed and executed via **VS Code**.  
* **Hardware Interfacing:** Utilization of **USB-to-TTL drivers** (CP210x) to map signals through the laptop's **COM ports**.  
* **Wireless Control:** Integrated Bluetooth (HC-06) for remote triggering via the **Bluetooth Serial Terminal** mobile app.

## **⚡ System Schematic & Wiring BluePrint**

The following diagram illustrates the complete hardware integration. It highlights the use of protection components (Flyback Diode) and torque enhancements (Kick-start Capacitor) within our mixed-voltage architecture.

![Con_BluePrint_v2](Con_BluePrint_v2.jpg)

### **📐 Technical Specifications**
| Component | Function | Value/Spec | 
| :--- | :--- | :--- | 
| **Transistor** | Low-Side Switch (NPN) | 2N2222A | 
| **Diode** | Flyback Protection (Snubber) | 1N4007 | 
| **Capacitor** | Speed-up "Kick" (Stiction Fix) | 10µF | 
| **Logic Source** | USB-to-TTL Bridge | CP2102 (3.3V) | 
| **Power Source** | Lithium-Ion Battery | 7.4V (2S) | 
| **Resistors** | Current Limiting | 1kΩ (Base), 330Ω (LED) |

### **🔌 Official Wiring Table**

| From Component | Pin/Leg | To Component | Connection Type |
| :--- | :--- | :--- | :--- |
| **CP2102** | DTR | LED (+) | Signal Path (LED) |
| **LED (-)** | Cathode | GND Rail | Return Path |
| **CP2102** | RTS | 1kΩ Resistor | Control Path (Motor) |
| **1kΩ Resistor** | (Output) | Transistor Base | Trigger Signal |
| **10µF Cap** | Parallel | Across 1kΩ Resistor | Kick-start Impulse |
| **Battery (+)** | 7.4V Rail | Motor (+) | High Power Path |
| **Diode (1N4007)** | Stripe | Battery (+) | Protection (Clamp) |
| **Motor (-)** | Output | Transistor Collector | Return to Switch |
| **Transistor** | Emitter | GND Rail | System Common Ground |

## **🔬 Theoretical Concepts Applied**

### **1\. UART Protocol & Serial Control**

The system utilizes the Universal Asynchronous Receiver-Transmitter (UART) protocol. Instead of standard TX/RX data pins, we repurpose the **DTR (Data Terminal Ready)** and **RTS (Request to Send)** hardware flow control pins of the CP2102 as manual logic toggles.

### **2\. Low-Side Switching (NPN Logic)**

The 2N2222A transistor is placed on the **Negative (Low) side** of the load.

* **Why?** In an NPN configuration, the Emitter is tied to Ground (0V). This allows a small 3.3V signal from the laptop to easily exceed the V\_{be} threshold (\~0.7V) to saturate the transistor, regardless of the 7.4V supply on the Collector.

### **3\. Inductive Kickback & Flyback Diodes**

Motors are inductive loads. When current is suddenly cut, the magnetic field collapses, generating a high-voltage spike (Back-EMF) that can reach 100V+.

* **The 1N4007 Diode** is placed in parallel to the motor to provide a safe recirculation path for this energy, protecting the transistor and the laptop's USB port.

### **4\. Capacitor Kick-Starting (Overcoming Stiction)**

To overcome the static friction (stiction) of the motor, a capacitor is placed in parallel with the base resistor. This creates a high-current pulse upon activation, "kicking" the motor into motion automatically.

## **💻 Software Implementation**

The current level of this project is driven by **Python 3**.

* **Development Environment:** The code is managed and executed using **Visual Studio Code (VS Code)**.  
* **Library Dependency:** The core of the serial communication is handled by the **pyserial** library, which allows Python to read/write directly to the system's hardware COM ports.  
* **Serial Communication:** The script monitors the **COM ports** assigned to the USB-to-TTL adapters and handles incoming data from the Bluetooth serial buffer.

### 📦 Prerequisites

**1. Software Dependencies**
To run the Python controller, install the required serial library via pip:

pip install pyserial

2. Hardware Drivers (CP2102)
This project uses a CP2102 USB-to-TTL adapter to bridge the laptop to the physical circuit. Your computer must have the correct Virtual COM Port (VCP) drivers installed to recognize the device.

Download: Get the official CP210x USB to UART Bridge VCP Drivers directly from Silicon Labs.

Verify: After installation, plug in the module and check your Device Manager (Windows) or System Information (Mac/Linux) to confirm it is recognized and note which COM port it has been assigned.

### **Commands:**

* A: Toggles LED (DTR Pin)  
* B: Toggles Motor (RTS Pin)  
* 0: Emergency System Shutdown (All pins High/OFF)

## **🚀 Future Roadmap**

As the course advances, we will move toward a real full hardware environment and more complex sensor integration:

* **communication protocol transition:** begin using other communication protocols(WIFI,ZIGBEE).
* **Begin using a real MicroController:** starting with Esp32 or maybe Arduino UNO.  
* **Firmware Transition:** Developing **C code (firmware)** to replace the Python-based laptop control.  
* **Hardware Transition:** Migration to a dedicated Microcontroller unit, specifically the **ATmega128A**.  
* **Flashing Process:** The firmware will be compiled and flashed using **Microchip Studio**.  
* **Peripherals:** Integration of **Servo Motors**, **LCD Displays**, and **Ultrasonic Sensors**.

</details>


# **Smart Home Controller Phase 2: Decoupled Control & Wi-Fi Integration**
<details>

## ** Phase 2 Abstract**

This document details the second phase of the VLSI Smart Home Controller Proof of Concept (PoC). Building upon the hardware foundations established in Phase 1, this iteration introduces a massive architectural upgrade: **Network Standardization** via TCP/IP Wi-Fi, and a **Decoupled State-Monitoring** approach for hybrid lighting control.

Additionally, this document outlines the theoretical design for an *upcoming* **Fail-Safe Dual-Power** architecture (Hardware OR Gate) that will simulate the robust, fault-tolerant topology of commercial, enterprise-grade IoT smart switches.

### **1.1 Phase 2 System Schematic (Wiring Diagram)**

*Figure 1: Complete wiring diagram for Phase 2, illustrating the separation of sensor inputs from logical load outputs.*

![Phase2](Phase2.png)

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
</details>

# **Smart Home Controller Phase 3: ESP32 Multi-Protocol OS**
<details>

## **📖 Overview**

This phase marks the migration from a PC-hosted Python script to a bare-metal embedded Operating System designed for the **ESP32 Microcontroller**. It acts as a centralized Local Area Network (LAN) hub, running a custom state-machine that allows multiple users to simultaneously connect and control physical hardware via **Wi-Fi (TCP/IP)**, **Bluetooth Serial**, and **USB Serial**.

The system features a polymorphic, non-blocking input engine, preventing terminal lockups and ensuring real-time multi-user interaction across 6 distinct built-in applications.

![Final_Phase](Final_Phase.png)

## **✨ Key Features**

* **Multi-Protocol Concurrency:** Connect up to 4 Wi-Fi users, 1 Bluetooth user, and 1 local USB terminal simultaneously. All users share the same live state.  
* **Non-Blocking I/O:** Characters are buffered asynchronously, completely ignoring invisible terminal garbage bytes (like ANSI escape codes from PuTTY).  
* **Hardware Multiplexing:** Complex features (like editing time or triggering strobe lights) are achieved using only two physical buttons via precise Long-Press/Short-Press timing algorithms.  
* **Non-Volatile Storage (NVS):** Network credentials and custom IP addresses are saved permanently to the ESP32's internal flash memory.  
* **Hardware Failsafe:** A physical button sequence during boot instantly wipes corrupted memory and restores factory default settings.

## **🛠️ Hardware Requirements & Wiring**

| Component | ESP32 GPIO Pin | Details / Protection |
| :---- | :---- | :---- |
| **LED 1 (Relay 1\)** | GPIO 4 | Wired with 220Ω Resistor |
| **LED 2 (Relay 2\)** | GPIO 5 | Wired with 220Ω Resistor |
| **Button 1 (Dot/Up)** | GPIO 6 | Active LOW (Internal Pull-Up) |
| **Button 2 (Dash/Down)** | GPIO 7 | Active LOW (Internal Pull-Up) |
| **Menu Button (Switch)** | GPIO 9 | Active LOW (Internal Pull-Up) |
| **DC Motor PWM** | GPIO 10 | Low-side switched via TIP120 (NPN), 1kΩ base resistor, and 1N4007 flyback diode. Powered by external 7.4V battery. |
| **RGB Smart LED** | GPIO 48 | Built-in WS2812B NeoPixel |

## **🚀 The Operating System Apps**

### **\[1\] Smart Home Controller**

Controls two digital outputs. Features a network-triggered or long-press-triggered **Police Mode** which overrides states to flash the LEDs rapidly. Interrupt-safe.

### **\[2\] Telegraph (1-Button Mode)**

Simulates a classic straight key. Uses a millisecond threshold logic (\<300ms \= Dot, \>300ms \= Dash) to decode taps into characters and broadcasts them live to the network.

### **\[3\] Telegraph (2-Button Mode)**

Simulates an Iambic Paddle. Button 1 types Dots, Button 2 types Dashes. Pauses dictate character and word spacing.

### **\[4\] Motor Speed Controller**

Provides 0-100% PWM speed control. Features a **Software Kickstart Burst** (a 50ms 100% power surge to break static friction before settling into lower speeds).

### **\[5\] Wi-Fi Configurator**

Allows live editing of the ESP32's SoftAP Network Name (SSID), Password, and Gateway IP address via terminal. Commits to NVS and safely reboots the system.

### **\[6\] Network Digital Clock**

A background timing engine that tracks Day, Month, Year, Hour, Minute, and Second. Projects a virtual LCD onto all connected terminal screens. Fully configurable via network commands or hardware button multiplexing.

## **💻 Setup & Installation (PlatformIO)**

This project requires **Visual Studio Code** with the **PlatformIO** extension.

1. Clone this repository.  
2. Open the folder in VS Code.  
3. Ensure your platformio.ini includes the required libraries:  
   \[env:esp32dev\]  
   platform \= espressif32  
   board \= esp32dev  
   framework \= arduino  
   monitor\_speed \= 115200  
   lib\_deps \=   
       adafruit/Adafruit NeoPixel @ ^1.11.0

4. Build and Upload to your ESP32.

## **📡 Connecting to the Hub**

* **Via USB:** Open PlatformIO Serial Monitor (Baud: 115200).  
* **Via Bluetooth:** Pair your phone with ESP32\_Batmobile\_BT and open a Bluetooth Serial Terminal app.  
* **Via Wi-Fi:** Connect to the ESP32\_Master\_Node Wi-Fi network (Pass: adminpassword). Open PuTTY or a TCP app and connect to 192.168.4.1 on Port 8080 (Raw/Telnet).

## **⚠️ Factory Reset Rescue**

If you accidentally misconfigure the Wi-Fi (e.g., set an impossible IP address or forget the password):

1. Press the RST (Reset) button on the ESP32.  
2. Immediately press and hold the **Menu Button (GPIO 9\)** on your breadboard.  
3. The built-in RGB LED will violently flash **RED** 15 times, indicating the NVS memory has been wiped. The system will reboot with the default ESP32\_Master\_Node credentials.
</details>

