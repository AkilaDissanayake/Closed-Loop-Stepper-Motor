# Closed-Loop-Stepper-Motor
Closed-loop stepper motor control system using Field-Oriented Control (FOC) and Space Vector Modulation (SVM) for precise, smooth, and efficient motion. Encoder feedback enables real-time position correction to eliminate step loss under dynamic loads.
# Closed-Loop Stepper Motor Control System

## Overview

This project implements an advanced closed-loop stepper motor control system utilizing **Field-Oriented Control (FOC)** and **Space Vector Modulation (SVM)**. It features real-time **encoder feedback**, **dead-time insertion** for power stage safety, and supports **three control modes**: **torque**, **speed**, and **position**. The system is designed for high-performance motion control applications requiring smooth, precise, and reliable motor behavior under varying loads.

---

## Features

- **Closed-loop control** with real-time encoder feedback
- **Field-Oriented Control (FOC)** for smooth, efficient torque generation
- **Space Vector Modulation (SVM)** for optimized PWM output
- **Dead-time insertion** for safe switching of driver IC
- **Three control modes**:
  - **Torque mode**
  - **Speed mode**
  - **Position mode**
-  Real-time compensation for load disturbances and step loss

---

## Hardware Used

- Stepper Motor -Nemma17 with magnetic encoder
- Rotary Encoder -Quadrature
- Microcontroller-TMS320F28069M
- Motor Driver -Toshiba TB67H400A
- Power Supply -24V SMPS power supply
  
- LAUNCHXL F28069M development board
- XDS110 debugger

---

## Software Components

- FOC algorithm implementation (Clarke, Park, PI controllers)
- SVM signal generation
- Encoder interface (incremental)
- Mode selection logic for torque/speed/position control
- Dead-time configuration in PWM channels

---

## Control Modes

- **Torque Mode**: Direct control of motor torque (Iq current loop)
- **Speed Mode**: Outer speed loop controlling inner torque loop
- **Position Mode**: Outer position loop → speed → torque cascade control

---




## References

- [Field Oriented Control Basics – STMicroelectronics](https://www.st.com/en/motor-control/field-oriented-control.html)
- [Space Vector Modulation – TI Application Notes](https://www.ti.com/lit/an/spra524/spra524.pdf)
- [Battery University – Encoder Feedback](https://www.batteryuniversity.com/)
