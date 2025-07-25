#include "F2806x_Device.h"
#include "F2806x_Examples.h"
#include "IQmathLib.h"

// Define constants
#define POLE_PAIRS 50           // NEMA 17: 200 steps/rev = 50 pole pairs
#define ENCODER_PPR 200         // Encoder pulses per revolution (adjust if 1000 PPR)
#define STEPS_PER_REV 200       // NEMA 17 steps per revolution (1x stepping)
#define DEFAULT_RPM 100.0       // Default speed (RPM)
#define SYSCLK 90e6             // System clock (Hz)
#define MAX_INPUT_LEN 32        // Max Serial input length

// Global variables
_iq speed_ref = _IQ(DEFAULT_RPM); // Speed reference (RPM)
_iq speed = _IQ(0.0);           // Measured speed (RPM)
int32 target_pos = 0;           // Target position (steps)
int32 current_pos = 0;          // Current position (steps)
Uint16 dir_state = 0;           // Direction (0: forward, 1: reverse)
Uint32 encoder_pos;             // Encoder position
static Uint32 last_pos = 0;     // Previous encoder position
Uint16 led_state = 0;           // LED state
Uint32 loop_count = 0;          // Loop counter for LED
char rx_buffer[MAX_INPUT_LEN];  // Serial input buffer
Uint16 rx_index = 0;            // Buffer index
volatile Uint16 rx_ready = 0;   // Flag for complete input

// Function prototypes
void init_system(void);
void init_eqep(void);
void init_pwm(void);
void init_scia(void);
void read_encoder(void);
void update_stepper(void);
void toggle_led(void);
void scia_msg(char *msg);
void parse_input(void);

// Interrupt service routines
__interrupt void timer0_isr(void);
__interrupt void scia_rx_isr(void);

// Main function
void main(void) {
    init_system();
    init_eqep();
    init_pwm();
    init_scia();

    // Enable interrupts
    EINT;
    ERTM;

    // Send initial prompt
    scia_msg("Enter position (steps) and direction (0=forward, 1=reverse), e.g., 'P1000 D0'\r\n");

    while(1) {
        // Blink LED every 500ms
        if (loop_count++ >= 450000) { // 90MHz * 500ms
            toggle_led();
            loop_count = 0;
        }
        // Process Serial input
        if (rx_ready) {
            parse_input();
            rx_ready = 0;
        }
    }
}

// System initialization
void init_system(void) {
    InitSysCtrl(); // Initialize system clock (90 MHz)
    DINT;

    // Disable watchdog
    EALLOW;
    SysCtrlRegs.WDCR = 0x0068;
    EDIS;

    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;

    InitPieVectTable();
    EALLOW;
    PieVectTable.TINT0 = &timer0_isr;
    PieVectTable.SCIRXINTA = &scia_rx_isr;
    EDIS;

    // Configure LED (GPIO34)
    EALLOW;
    GpioCtrlRegs.GPBMUX1.bit.GPIO34 = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO34 = 1;
    GpioDataRegs.GPBCLEAR.bit.GPIO34 = 1;

    // Configure DIR pin (GPIO7)
    GpioCtrlRegs.GPAMUX1.bit.GPIO7 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO7 = 1;
    GpioDataRegs.GPACLEAR.bit.GPIO7 = 1; // Forward
    EDIS;
}

// eQEP initialization
void init_eqep(void) {
    InitEQep1Gpio();
    EALLOW;
    EQep1Regs.QEPCTL.bit.QPEN = 1; // Enable eQEP
    EQep1Regs.QDECCTL.bit.QSRC = 0; // Quadrature count mode
    EQep1Regs.QPOSMAX = ENCODER_PPR * 4 * POLE_PAIRS - 1; // 39999 for 200 PPR
    EQep1Regs.QPOSINIT = 0;
    EQep1Regs.QEPCTL.bit.PCRM = 0; // Position counter reset on index
    EQep1Regs.QEINT.bit.PCE = 0; // Disable PCE interrupt
    EQep1Regs.QCLR.bit.PCE = 1;
    EDIS;
}

// PWM initialization for STEP pulses
void init_pwm(void) {
    EALLOW;
    InitEPwm4Gpio(); // GPIO6 (ePWM4A) for STEP
    EPwm4Regs.TBCTL.bit.CTRMODE = 0; // Up-count mode
    EPwm4Regs.TBPRD = 13499; // 3333.33 Hz for 100 RPM (90 MHz / (200 * 100 / 60))
    EPwm4Regs.CMPA.half.CMPA = 6750; // 50% duty cycle
    EPwm4Regs.AQCTLA.bit.ZRO = 2; // Set high at TBCTR = 0
    EPwm4Regs.AQCTLA.bit.CAU = 1; // Clear low at CMPA
    EPwm4Regs.TBCTL.bit.PHSEN = 0; // Disable phase sync
    EDIS;

    InitCpuTimers();
    ConfigCpuTimer(&CpuTimer0, 90, 100); // 10 kHz ISR
    CpuTimer0Regs.TCR.bit.TSS = 0; // Start timer
}

// SCI-A initialization for Serial
void init_scia(void) {
    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.SCIAENCLK = 1;
    GpioCtrlRegs.GPAMUX2.bit.GPIO28 = 1; // TX
    GpioCtrlRegs.GPAMUX2.bit.GPIO29 = 1; // RX
    GpioCtrlRegs.GPAPUD.bit.GPIO28 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO29 = 0;
    SciaRegs.SCICCR.all = 0x0007; // 8-bit, no parity, 1 stop bit
    SciaRegs.SCICTL1.all = 0x0003; // Enable TX, RX
    SciaRegs.SCIHBAUD = 0x0000;
    SciaRegs.SCILBAUD = 0x0068; // 9600 baud at 90 MHz
    SciaRegs.SCICTL2.bit.RXBKINTENA = 1; // Enable RX interrupt
    SciaRegs.SCICTL1.bit.SWRESET = 1;
    PieCtrlRegs.PIEIER9.bit.INTx1 = 1; // Enable SCIRXINTA
    IER |= M_INT9; // Enable PIE group 9
    EDIS;
}

// Read encoder and compute speed
void read_encoder(void) {
    _iq delta_pos;
    encoder_pos = EQep1Regs.QPOSCNT;
    delta_pos = _IQ(encoder_pos - last_pos);
    speed = _IQmpy(delta_pos, _IQ(60.0 / (0.0001 * ENCODER_PPR * 4.0 * POLE_PAIRS)));
    current_pos = (int32)(encoder_pos / (ENCODER_PPR * 4.0 / STEPS_PER_REV)); // Steps
    last_pos = encoder_pos;
}

// Update stepper speed and direction
void update_stepper(void) {
    // Stop if target position reached
    if (current_pos == target_pos) {
        EPwm4Regs.TBCTL.bit.CTRMODE = 3; // Disable PWM
        return;
    }

    // Set direction
    GpioDataRegs.GPASET.bit.GPIO7 = dir_state;
    GpioDataRegs.GPACLEAR.bit.GPIO7 = !dir_state;

    // Enable PWM if stopped
    if (EPwm4Regs.TBCTL.bit.CTRMODE == 3) {
        EALLOW;
        EPwm4Regs.TBCTL.bit.CTRMODE = 0; // Up-count mode
        EDIS;
    }

    // Calculate STEP frequency: steps/sec = (steps/rev * RPM) / 60
    float rpm = _IQtoF(speed_ref);
    Uint32 freq = (Uint32)(STEPS_PER_REV * rpm / 60.0);
    Uint16 tbprd = (freq > 0) ? (Uint16)(SYSCLK / freq - 1) : 13499;
    EALLOW;
    EPwm4Regs.TBPRD = tbprd;
    EPwm4Regs.CMPA.half.CMPA = tbprd / 2; // 50% duty
    EDIS;
}

// Toggle LED
void toggle_led(void) {
    led_state = !led_state;
    GpioDataRegs.GPBSET.bit.GPIO34 = led_state;
    GpioDataRegs.GPBCLEAR.bit.GPIO34 = !led_state;
}

// Send Serial message
void scia_msg(char *msg) {
    while (*msg != '\0') {
        while (SciaRegs.SCIFFTX.bit.TXFFST == 16);
        SciaRegs.SCITXBUF = *msg++;
    }
}

// Parse Serial input (e.g., "P1000 D0")
void parse_input(void) {
    char *token;
    char buffer[MAX_INPUT_LEN];
    strcpy(buffer, rx_buffer);

    // Parse position
    token = strtok(buffer, " ");
    if (token != NULL && token[0] == 'P') {
        target_pos = atol(token + 1);
        scia_msg("Target Position: ");
        scia_msg(token + 1);
        scia_msg(" steps\r\n");
    }

    // Parse direction
    token = strtok(NULL, " ");
    if (token != NULL && token[0] == 'D') {
        dir_state = atoi(token + 1);
        scia_msg("Direction: ");
        scia_msg(dir_state ? "Reverse" : "Forward");
        scia_msg("\r\n");
    }

    // Clear buffer
    rx_index = 0;
    rx_buffer[0] = '\0';
}

// SCI-A RX ISR
__interrupt void scia_rx_isr(void) {
    char rx_char = SciaRegs.SCIRXBUF.all;
    if (rx_char == '\r' || rx_char == '\n') {
        rx_buffer[rx_index] = '\0';
        rx_ready = 1;
    } else if (rx_index < MAX_INPUT_LEN - 1) {
        rx_buffer[rx_index++] = rx_char;
    }
    SciaRegs.SCIRXST.bit.RXRDY = 0; // Clear RX flag
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;
}

// Timer ISR
__interrupt void timer0_isr(void) {
    read_encoder();
    update_stepper();

    // Send status every 100 ms
    static Uint16 counter = 0;
    if (counter++ >= 100) { // 10 kHz * 0.1 s
        char buffer[64];
        sprintf(buffer, "Pos: %ld steps, Speed: %.2f RPM\r\n", current_pos, _IQtoF(speed));
        scia_msg(buffer);
        counter = 0;
    }

    CpuTimer0Regs.TCR.bit.TIF = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}