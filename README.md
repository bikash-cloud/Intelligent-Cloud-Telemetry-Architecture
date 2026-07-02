# Intelligent Cloud Telemetry Architecture

An advanced ESP32-based Intelligent Cloud Telemetry Architecture designed for real-time battery monitoring using the Blynk IoT Cloud. The system follows an event-driven telemetry model, transmitting data only during state changes, anomalies, or threshold violations instead of periodic polling, resulting in efficient cloud communication.

---

## Wokwi Simulation

https://wokwi.com/projects/468426032458565633

---

## Project Overview

This project demonstrates a smart embedded IoT telemetry system capable of:

- Real-time monitoring of four battery cells
- Event-driven cloud telemetry
- WiFi automatic reconnection
- Blynk Cloud integration
- Event queue synchronization
- Battery anomaly detection
- Voltage threshold monitoring
- Runtime state management
- LCD status display
- Relay protection
- Buzzer fault indication
- RSSI signal quality monitoring
- Asynchronous cloud communication

The embedded firmware continues operating normally even if the cloud connection is unavailable, ensuring fault tolerance and system reliability.

---

## Features

- ESP32 based architecture
- Blynk IoT Cloud
- Event-driven telemetry
- Battery voltage monitoring
- Four analog channels
- WiFi reconnect mechanism
- Cloud synchronization queue
- Runtime state monitoring
- RSSI monitoring
- LCD status interface
- Relay control
- Fault buzzer
- Low voltage detection
- High voltage detection
- Cell imbalance detection
- Cloud offline protection
- Non-blocking firmware architecture
- ---

# Hardware Used

| Component | Quantity |
|-----------|---------:|
| ESP32 DevKit V1 | 1 |
| 16×2 LCD (I2C) | 1 |
| Relay Module | 1 |
| Active Buzzer | 1 |
| Potentiometers (Battery Cell Simulation) | 4 |
| WiFi Network | 1 |
| Blynk Cloud Platform | 1 |

---

# Circuit Diagram

The project is simulated using the Wokwi simulator.

**Simulation Link**

https://wokwi.com/projects/468426032458565633

---

# Pin Connections

| ESP32 Pin | Device |
|-----------|--------|
| GPIO33 | Cell 1 Voltage |
| GPIO35 | Cell 2 Voltage |
| GPIO34 | Cell 3 Voltage |
| GPIO32 | Cell 4 Voltage |
| GPIO25 | Relay Module |
| GPIO26 | Active Buzzer |
| GPIO21 | LCD SDA |
| GPIO22 | LCD SCL |
| 5V | Relay, LCD |
| 3.3V | Potentiometers |
| GND | Common Ground |

---

# Software Requirements

- Arduino IDE 2.x
- ESP32 Board Package
- Wokwi Simulator
- Blynk IoT Platform
- Internet Connection
- Git

---

# Required Libraries

Install the following libraries from the Arduino Library Manager.

- Blynk
- LiquidCrystal_I2C
- WiFi (ESP32 Built-in)
- Wire (Built-in)

---

# Cloud Platform

This project uses the **Blynk IoT Cloud** for remote telemetry.

Cloud Features:

- Real-time battery monitoring
- Runtime state monitoring
- Fault event logging
- RSSI monitoring
- Queue monitoring
- Cloud synchronization
- Automatic reconnection
- Live dashboard

---

# Battery Parameters Monitored

The firmware continuously monitors:

- Cell 1 Voltage
- Cell 2 Voltage
- Cell 3 Voltage
- Cell 4 Voltage
- Pack Voltage
- Average Voltage
- Cell Imbalance
- Runtime State
- WiFi RSSI
- Queue Status
- Fault Events

---

# Runtime States

The system operates in the following runtime states.

| State | Description |
|--------|-------------|
| NORMAL | Battery operating within limits |
| FAULT | One or more voltage thresholds violated |

During the **FAULT** state:

- Relay is turned OFF
- Buzzer is activated
- LCD displays fault status
- Cloud event is generated
- Fault is stored in the event queue
