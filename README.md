# 🚀 Closed-Loop Stepper Motor Control System  

A **high-performance closed-loop stepper motor system** powered by **Field-Oriented Control (FOC)** and **Space Vector Modulation (SVM)**, delivering **precise**, **smooth**, and **efficient motion control**.  

Real-time **encoder feedback** ensures **zero step loss**, even under heavy dynamic loads – making this system ideal for **robotics**, **CNC machines**, and **advanced automation** applications.  

---

## ✨ Features  

✅ **Closed-Loop Feedback** – Quadrature encoder eliminates step loss by constantly correcting position in real time.  
✅ **FOC (Field-Oriented Control)** – Decoupled torque & flux control for buttery-smooth motion.  
✅ **SVM (Space Vector Modulation)** – Generates efficient, optimized PWM waveforms.  
✅ **Dead-Time Insertion** – Prevents MOSFET shoot-through for safe, reliable switching.  
✅ **Multi-Mode Control**:  
   - 🎯 **Torque Mode** – Direct torque regulation via Q-axis current.  
   - ⚡ **Speed Mode** – Outer PI speed loop for consistent velocity.  
   - 📍 **Position Mode** – Expandable to precision positioning applications.  
✅ **Load Disturbance Compensation** – Maintains performance under sudden mechanical shocks.  

---

## 🛠 Hardware Setup  

- **Stepper Motor**: NEMA17 w/ integrated magnetic encoder  
- **Encoder**: Quadrature rotary encoder  
- **MCU**: TI **TMS320F28069M** (C2000 series – optimized for motor control)  
- **Driver**: Toshiba **TB67H400A** H-bridge driver  
- **Power Supply**: 24V SMPS  
- **Dev Board**: TI **LAUNCHXL-F28069M** + XDS110 Debugger  

---

## 🧠 Software Architecture  

- 🌀 **FOC Engine** – Implements **Clarke & Park transforms** for current regulation.  
- ⚙️ **Current Control Loop** – Inner PI loop for **Id/Iq** current control.  
- 🚀 **Speed Control Loop** – Outer PI loop regulates speed based on encoder feedback.  
- 🔺 **SVM** – Generates smooth PWM signals for precise voltage vector control.  
- ⏱ **Dead-Time Management** – Ensures MOSFET safety during commutation.  
- 🎯 **Encoder Interface (eQEP)** – Real-time speed & position tracking.  
- 📊 **Current Sensing & Calibration** – Phase current monitoring with ADC correction.  
- ⚡ **Interrupt-Driven Control** – High-speed ISR-based execution for ultra-low latency.  
- 🎛 **PWM Generation** – Configured for **10 kHz switching** using TI’s ePWM modules.  
- 🔧 **Utility Layer** – IQmath-based helpers for sign detection, clamping & timing.  

---

## 🎮 Control Modes  

### 🔴 Torque Mode  
Controls motor torque **directly** by regulating Q-axis current. Perfect for force-sensitive applications like **robot arms** or **actuators**.  

### 🟠 Speed Mode  
A **cascaded PI loop** sets torque (Iq) based on speed error – ensuring **steady, ripple-free velocity**.  

### 🟢 Position Mode *(Scalable)*  
An **outer position loop** feeds commands to the speed loop – enabling high-precision motion like **CNC**, **pick-and-place machines**, or **3D printers**.  

---

## 🔬 Prototype  

🖼 *Compact design built for performance testing*  

![Prototype](https://github.com/user-attachments/assets/d901d41d-3a4a-47de-bba0-d745e491a199)  
![Prototype](https://github.com/user-attachments/assets/c8f5a225-2fad-4fe8-bc09-63f01f6fe618)  

---

## ⚡ PCB Design  

A **4-layer PCB** designed for **thermal efficiency**, **signal integrity**, and **high-current handling**.  

- ✅ Optimized power & ground planes for **low-noise FOC**.  
- ✅ Carefully routed encoder & current sense lines for **accurate feedback**.  
- ✅ Built-in dead-time safe switching logic.  

![PCB](https://github.com/user-attachments/assets/f14de196-a4a8-4fa2-bf8c-45a497991b9d)  
![PCB](https://github.com/user-attachments/assets/4b3434ad-3483-46ad-9b8a-935943d19034)  

---

## 📚 References  

📘 [TI – Field Oriented Control Overview](https://www.ti.com/video/6296584406001)  
📘 [TI – C2000 Motor Control User Guide](https://www.tij.co.jp/jp/lit/ug/spru556/spru556.pdf)  
📘 [TI – InstaSPIN FOC Documentation](https://www.ti.com/lit/pdf/sprui11)  
