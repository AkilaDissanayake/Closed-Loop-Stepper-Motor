#include "F2806x_Device.h"
#include "F2806x_Examples.h"
#include "IQmathLib.h"

// Define constants
#define M_PI 3.14159265358979323846
#define POLE_PAIRS 50           // NEMA 17: 200 steps/rev = 50 pole pairs
#define RS 2.0                  // Stator resistance (ohms)
#define LS 0.002                // Stator inductance (H)
#define VDC 12.0                // DC bus voltage (V)
#define PWM_PERIOD 4500         // 10kHz PWM (90MHz / (2 * 4500))
#define CURRENT_MAX 2.0         // Max current (A)
#define SPEED_MAX 1000.0        // Max speed (RPM)
#define ADC_OFFSET 2048         // 12-bit ADC midpoint
#define ADC_GAIN 0.1            // Current sensor gain (A/V)

// PI controller gains
#define KP_D 0.5                // D-axis current proportional gain
#define KI_D 0.01               // D-axis current integral gain
#define KP_Q 0.5                // Q-axis current proportional gain
#define KI_Q 0.01               // Q-axis current integral gain
#define KP_SPEED 0.1            // Speed proportional gain
#define KI_SPEED 0.001          // Speed integral gain

// Global variables
_iq Id_ref = _IQ(0.0);          // D-axis current reference
_iq Iq_ref = _IQ(0.0);          // Q-axis current reference
_iq speed_ref = _IQ(100.0);     // Speed reference (RPM)
_iq theta_e = _IQ(0.0);         // Electrical angle
_iq Ia, Ib;                     // Phase currents
_iq Ialpha, Ibeta;              // Clarke transform outputs
_iq Id, Iq;                     // Park transform outputs
_iq Vd, Vq;                     // Voltage outputs
_iq Valpha, Vbeta;              // Inverse Park transform outputs
_iq speed = _IQ(0.0);           // Measured speed
Uint32 encoder_pos;             // Encoder position
static Uint32 last_pos = 0;     // Previous encoder position
Uint16 led_state = 0;           // LED state
Uint32 loop_count = 0;          // Loop counter for LED

// Function prototypes
void init_system(void);
void init_adc(void);
void init_eqep(void);
void init_pwm(void);
void read_currents(void);
void read_encoder(void);
void clarke_transform(void);
void park_transform(void);
void pi_current_control(void);
void pi_speed_control(void);
void inv_park_transform(void);
void svgen_pwm(void);
void toggle_led(void);
_iq _IQsgn(_iq value);

// Interrupt service routines
__interrupt void adc_isr(void);
__interrupt void timer0_isr(void);

// Main function
void main(void) {
    init_system();
    init_adc();
    init_eqep();
    init_pwm();

    // Enable interrupts
    EINT;
    ERTM;

    while(1) {
        // Blink LED every 500ms
        if (loop_count++ >= 450000) { // 90MHz * 500ms
            toggle_led();
            loop_count = 0;
        }
    }
}

// System initialization
void init_system(void) {
    InitSysCtrl(); // Initialize system clock (90 MHz)
    DINT;

    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;

    InitPieVectTable();
    EALLOW;
    PieVectTable.ADCINT1 = &adc_isr;
    PieVectTable.TINT0 = &timer0_isr;
    EDIS;

    // Configure LED (GPIO34)
    EALLOW;
    GpioCtrlRegs.GPBMUX1.bit.GPIO34 = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO34 = 1;
    GpioDataRegs.GPBCLEAR.bit.GPIO34 = 1;
    EDIS;
}

// ADC initialization
void init_adc(void) {
    InitAdc();
    EALLOW;
    AdcRegs.ADCCTL1.bit.INTPULSEPOS = 1;
    AdcRegs.ADCCTL1.bit.ADCBGPWD = 1;
    AdcRegs.ADCCTL1.bit.ADCPWDN = 1;

    AdcRegs.ADCSOC0CTL.bit.CHSEL = 0; // ADCINA0 (Ia)
    AdcRegs.ADCSOC1CTL.bit.CHSEL = 1; // ADCINA1 (Ib)
    AdcRegs.ADCSOC0CTL.bit.ACQPS = 6;
    AdcRegs.ADCSOC1CTL.bit.ACQPS = 6;
    AdcRegs.ADCSOC0CTL.bit.TRIGSEL = 5; // ePWM1 SOCA
    AdcRegs.ADCSOC1CTL.bit.TRIGSEL = 5;

    AdcRegs.ADCINTSOCSEL1.bit.SOC1 = 1; // SOC1 triggers ADCINT1
    AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
    AdcRegs.INTSEL1N2.bit.INT1E = 1; // Enable ADCINT1
    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;
    EDIS;

    AdcRegs.ADCSOCFRC1.all = 0x0003; // Force SOC0 and SOC1
}

// eQEP initialization
void init_eqep(void) {
    InitEQep1Gpio();
    EALLOW;
    EQep1Regs.QEPCTL.bit.QPEN = 1;
    EQep1Regs.QDECCTL.bit.QSRC = 0;
    EQep1Regs.QPOSMAX = 200 * POLE_PAIRS - 1;
    EQep1Regs.QPOSINIT = 0;
    EQep1Regs.QEPCTL.bit.PCRM = 0;
    EQep1Regs.QEINT.bit.PCE = 1; // Position counter error interrupt
    EQep1Regs.QCLR.bit.PCE = 1;
    EDIS;
}

// PWM initialization
void init_pwm(void) {
    InitEPwm1Gpio();
    InitEPwm2Gpio();
    EALLOW;

    EPwm1Regs.TBCTL.bit.CTRMODE = 3;
    EPwm1Regs.TBPRD = PWM_PERIOD;
    EPwm1Regs.CMPA.half.CMPA = 0;
    EPwm1Regs.AQCTLA.bit.CAU = 2;
    EPwm1Regs.AQCTLA.bit.CAD = 1;
    EPwm1Regs.TBCTL.bit.PHSEN = 0;

    EPwm2Regs.TBCTL.bit.CTRMODE = 3;
    EPwm2Regs.TBPRD = PWM_PERIOD;
    EPwm2Regs.CMPA.half.CMPA = 0;
    EPwm2Regs.AQCTLA.bit.CAU = 2;
    EPwm2Regs.AQCTLA.bit.CAD = 1;
    EPwm2Regs.TBCTL.bit.PHSEN = 0;

    EPwm1Regs.ETSEL.bit.SOCAEN = 1;
    EPwm1Regs.ETSEL.bit.SOCASEL = 1;
    EPwm1Regs.ETPS.bit.SOCAPRD = 1;
    EDIS;

    InitCpuTimers();
    ConfigCpuTimer(&CpuTimer0, 90, 100); // 10kHz
    CpuTimer0Regs.TCR.bit.TSS = 0;
}

// Read phase currents
void read_currents(void) {
    Ia = _IQmpy(_IQ(AdcResult.ADCRESULT0 - ADC_OFFSET), _IQ(ADC_GAIN));
    Ib = _IQmpy(_IQ(AdcResult.ADCRESULT1 - ADC_OFFSET), _IQ(ADC_GAIN));
}

// Read encoder and compute speed
void read_encoder(void) {
    _iq delta_pos;

    encoder_pos = EQep1Regs.QPOSCNT;
    theta_e = _IQmpy(_IQ(encoder_pos), _IQ(2.0 * M_PI / (200.0 * POLE_PAIRS)));
    delta_pos = _IQ(encoder_pos - last_pos);
    speed = _IQmpy(delta_pos, _IQ(60.0 / (0.0001 * 200.0 * POLE_PAIRS)));
    last_pos = encoder_pos;
}

// Clarke transform
void clarke_transform(void) {
    Ialpha = Ia;
    Ibeta = _IQmpy(_IQ(0.57735026919), Ia) + _IQmpy(_IQ(1.15470053838), Ib);
}

// Park transform
void park_transform(void) {
    _iq cos_theta = _IQcos(theta_e);
    _iq sin_theta = _IQsin(theta_e);
    Id = _IQmpy(Ialpha, cos_theta) + _IQmpy(Ibeta, sin_theta);
    Iq = _IQmpy(Ibeta, cos_theta) - _IQmpy(Ialpha, sin_theta);
}

// PI control for currents
void pi_current_control(void) {
    static _iq Id_error_int = _IQ(0.0), Iq_error_int = _IQ(0.0);
    _iq Id_error, Iq_error, Vmax;

    Id_error = Id_ref - Id;
    Iq_error = Iq_ref - Iq;
    Vmax = _IQ(VDC * 0.5);

    Id_error_int += _IQmpy(KI_D, Id_error);
    if (_IQabs(Id_error_int) > Vmax) Id_error_int = _IQmpy(Vmax, _IQsgn(Id_error_int));
    Iq_error_int += _IQmpy(KI_Q, Iq_error);
    if (_IQabs(Iq_error_int) > Vmax) Iq_error_int = _IQmpy(Vmax, _IQsgn(Iq_error_int));

    Vd = _IQmpy(KP_D, Id_error) + Id_error_int;
    Vq = _IQmpy(KP_Q, Iq_error) + Iq_error_int;

    if (_IQabs(Vd) > Vmax) Vd = _IQmpy(Vmax, _IQsgn(Vd));
    if (_IQabs(Vq) > Vmax) Vq = _IQmpy(Vmax, _IQsgn(Vq));
}

// PI control for speed
void pi_speed_control(void) {
    static _iq speed_error_int = _IQ(0.0);
    _iq speed_error;

    speed_error = speed_ref - speed;
    speed_error_int += _IQmpy(KI_SPEED, speed_error);
    if (_IQabs(speed_error_int) > _IQ(CURRENT_MAX)) speed_error_int = _IQmpy(_IQ(CURRENT_MAX), _IQsgn(speed_error_int));

    Iq_ref = _IQmpy(KP_SPEED, speed_error) + speed_error_int;
    Id_ref = _IQ(0.0);

    if (_IQabs(Iq_ref) > _IQ(CURRENT_MAX)) Iq_ref = _IQmpy(_IQ(CURRENT_MAX), _IQsgn(Iq_ref));
}

// Inverse Park transform
void inv_park_transform(void) {
    _iq cos_theta = _IQcos(theta_e);
    _iq sin_theta = _IQsin(theta_e);
    Valpha = _IQmpy(Vd, cos_theta) - _IQmpy(Vq, sin_theta);
    Vbeta = _IQmpy(Vd, sin_theta) + _IQmpy(Vq, cos_theta);
}

// Space vector PWM
void svgen_pwm(void) {
    _iq Va, Vb;
    Uint16 cmpa, cmpb;
    float temp;

    Va = Valpha;
    Vb = _IQmpy(_IQ(-0.5), Valpha) + _IQmpy(_IQ(0.86602540378), Vbeta);

    temp = _IQtoF(_IQmpy(_IQ(Va / VDC), _IQ(PWM_PERIOD / 2)) + _IQ(PWM_PERIOD / 2));
    cmpa = (temp >= 0 && temp <= PWM_PERIOD) ? (Uint16)temp : (temp > PWM_PERIOD ? PWM_PERIOD : 0);

    temp = _IQtoF(_IQmpy(_IQ(Vb / VDC), _IQ(PWM_PERIOD / 2)) + _IQ(PWM_PERIOD / 2));
    cmpb = (temp >= 0 && temp <= PWM_PERIOD) ? (Uint16)temp : (temp > PWM_PERIOD ? PWM_PERIOD : 0);

    EPwm1Regs.CMPA.half.CMPA = cmpa;
    EPwm2Regs.CMPA.half.CMPA = cmpb;
}

// Toggle LED
void toggle_led(void) {
    led_state = !led_state;
    GpioDataRegs.GPBSET.bit.GPIO34 = led_state;
    GpioDataRegs.GPBCLEAR.bit.GPIO34 = !led_state;
}

// Custom IQ sign function
_iq _IQsgn(_iq value) {
    if (_IQtoF(value) > 0) return _IQ(1.0);
    if (_IQtoF(value) < 0) return _IQ(-1.0);
    return _IQ(0.0);
}

// ADC ISR
__interrupt void adc_isr(void) {
    read_currents();
    clarke_transform();
    park_transform();
    pi_current_control();
    inv_park_transform();
    svgen_pwm();

    AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

// Timer ISR
__interrupt void timer0_isr(void) {
    read_encoder();
    pi_speed_control();

    CpuTimer0Regs.TCR.bit.TIF = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}
