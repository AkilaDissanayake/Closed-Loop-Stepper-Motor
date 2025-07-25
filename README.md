# Closed-Loop-Stepper-Motor
Closed-loop stepper motor control system using Field-Oriented Control (FOC) and Space Vector Modulation (SVM) for precise, smooth, and efficient motion. Encoder feedback enables real-time position correction to eliminate step loss under dynamic loads.


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

- **Field-Oriented Control (FOC)**: Includes Clarke and Park transforms, PI controllers for Id/Iq current regulation
- **Speed Control Loop**: Outer PI loop regulating speed with real-time feedback from encoder
- **Space Vector Modulation (SVM)**: Efficient PWM generation for smooth voltage vector application
- **Dead-Time Insertion**: Prevents shoot-through in MOSFET drivers during switching transitions
- **Encoder Interface (eQEP)**: Reads position and calculates speed using quadrature encoder signals
- **Current Sensing and Calibration**: Reads phase currents using ADC with offset and scaling
- **Interrupt-Driven Architecture**: High-speed real-time control using ADC and timer interrupts
- **PWM Generation**: Synchronized PWM signals using ePWM modules configured for 10kHz switching
- **Custom Utility Functions**: Includes IQmath-based sign detection, safe clamping, and loop timing
- **Dead-time configuration in PWM channels**

---

## Control Modes

The system supports three primary modes of operation, implemented using a cascaded control structure:

- **Torque Mode**  
  Directly regulates motor torque via the **Q-axis current (Iq)** using a PI controller. Suitable for applications where torque control is required without concern for speed or position.

- **Speed Mode**  
  Implements a **speed PI controller** that sets the torque-producing current (`Iq_ref`) based on the difference between desired and measured speed. The current loop (FOC) ensures precise torque output.

- **Position Mode** *(optional/extendable)*  
  Position can be controlled using an outer position loop that feeds a speed reference into the speed controller. Though not explicitly shown in this code, it can be added by tracking the encoder position and applying a position error PI controller.

Each mode uses real-time encoder feedback and integrates with the FOC and SVM layers to achieve smooth, accurate motor control.


## Enclosure
![Image](https://github.com/user-attachments/assets/d901d41d-3a4a-47de-bba0-d745e491a199)
![Image](https://github.com/user-attachments/assets/c8f5a225-2fad-4fe8-bc09-63f01f6fe618)

## PCB
 - *The PCB was designed using a **4-layer** stackup to meet the electrical and thermal demands of FOC, SVM, and real-time encoder feedback control.*

## References

- [Field Oriented Control ](https://www.ti.com/video/6296584406001)
- [TI Application Notes](https://www.tij.co.jp/jp/lit/ug/spru556/spru556.pdf)
- [TI Application Notes](https://www.ti.com/lit/pdf/sprui11)
