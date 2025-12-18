/**
 * @file drivers.c
 * @brief Inicialización de periféricos y rutinas de hardware base.
 * [cite: 297-453, 591-599]
 */

#include "../include/config.h"
#include "../include/global_vars.h"
#include <xc.h>

#define _XTAL_FREQ 20000000UL // [cite: 591] Requerido para _delay_us

// [cite: 297]
void inicializar_pines(void) {
    // ENTRADAS [cite: 300]
    TRISAbits.TRISA4 = 1; // V_BAT
    TRISBbits.TRISB0 = 1; // AHORRO

    // SALIDAS [cite: 302-306]
    TRISBbits.TRISB4 = 0; // AIRE
    TRISBbits.TRISB5 = 0; // BUZZER
    TRISBbits.TRISB6 = 0; // LECTURA
    TRISBbits.TRISB7 = 0; // SYNC_OSC
    TRISCbits.TRISC3 = 0; // SPI_SCK
    TRISCbits.TRISC4 = 0; // SPI_SDI
    TRISCbits.TRISC5 = 0; // SPI_SDO

    // VALORES INICIALES [cite: 308-313]
    LATBbits.LATB4 = 0;
    LATBbits.LATB5 = 0;
    LATBbits.LATB6 = 0;
    LATBbits.LATB7 = 0;
    LATCbits.LATC3 = 0;
    LATCbits.LATC4 = 0;
    LATCbits.LATC5 = 0;
}

// [cite: 317]
void inicializar_puertos(void) {
    TRISA = 0b00111111; // RA0..RA5 entradas [cite: 320]
    LATA = 0x00;
    TRISB = 0b00001101; // RB0, RB2, RB3 entradas [cite: 323]
    LATB = 0x00;
    TRISC = 0b01000000; // RC6 entrada [cite: 329]
    LATC = 0x00;
}

// [cite: 331]
void inicializar_adc(void) {
    // AN0-AN4, AN8, AN9 analógicos
    ADCON1 = 0b00000101; // [cite: 334]
    ADCON2 = 0b00000101; // Justificación izq, Fosc/16, Tacq manual [cite: 338]
}

// [cite: 340]
void inicializar_interrupciones(void) {
    RCONbits.IPEN = 0;      // Deshabilito prioridades
    INTCONbits.GIE = 1;     // Habilitación global
    INTCONbits.PEIE = 1;
    PIE1bits.TMR2IE = 1;    // Interrupción Timer2
    PIR1 = 0x00;
    PIR2 = 0x00;
}

// [cite: 350]
void inicializar_spi(void) {
    SSPCON1 = 0b00000010; // SPI Fosc/64 [cite: 353]
    PIR1bits.SSPIF = 0;
    SSPSTATbits.SMP = 0;
    SSPSTATbits.CKE = 1;
    PIE1bits.SSPIE = 0;
}

// [cite: 359]
void inicializar_pwm_timer2(void) {
    CCP1CON = 0b00001100; // Modo PWM [cite: 362]
    CCP2CON = 0b00001100;
    PR2 = 255;            // Periodo 19.53kHz [cite: 364]
    T2CON = 0b00000000;   // Prescaler 1:1, Timer2 apagado
}

// [cite: 368]
void inicializar_variables(void) {
    // Arranque suave [cite: 370-374]
    INICIO_0 = 0x00;
    INICIO_1 = 0x64; // 100
    AA = 20;
    V_MAX_0 = 0x02;  // 675 (~676 en comentarios)
    V_MAX_1 = 0xA3;

    // Escalas lazo control [cite: 378-380]
    U = 100;
    U_0 = 0x1F;
    U_1 = 0x40;

    // Salidas físicas apagadas [cite: 382-388]
    AIRE = 0; BUZZER = 0; LECTURA = 0; SYNC_OSC = 0;
    SPI_SCK = 0; SPI_SDI = 0; SPI_SDO = 0;

    // PWM a cero [cite: 390-393]
    CCPR1L = 0; CCP1CONbits.DC1B = 0;
    CCPR2L = 0; CCP2CONbits.DC2B = 0;

    // Generación senoidal [cite: 395-405]
    CICLO_0 = 0; CICLO_1 = 0;
    TEMP1 = 0; TEMPO = 0;
    V_PICO_0 = 0; V_PICO_1 = 0;
    NN = 1; K = 0;
    SENO_0 = 0; SENO_1 = 0;

    // PID Refs [cite: 407-412]
    REF0 = 0x02; REF1 = 0xA4;
    pidStat1 = 0; percent_err = 0; REF_ERR = 128;

    // Protecciones [cite: 414-421]
    I_MINIMA = 0; CUENTA = 0; C_MAXIMA = 10;
    V_SALIDA = 0; I_SALIDA = 0;
    I_MAX = 242;
    PP = 0; PP_MAX = 5;

    // Térmicas [cite: 423-433]
    T_DISIP = 0; T_DISIP1 = 170; T_DISIP2 = 80;
    HIS_DIS1 = 210; HIS_DIS2 = 120;
    T_TRAFO = 0; T_TRAFO1 = 130; T_TRAFO2 = 20;
    HIS_TRA1 = 170; HIS_TRA2 = 60;

    // Estados [cite: 435-436]
    ESTADO = 0; PREVIO = 0;

    // Limpieza matemática
    AARGB0 = 0; BARGB0 = 0; 
    
    // PID Init (Ahora activo según PDF)
    // 
    PidInitialize();
    kp = 62;
    ki = 54;
    kd = 0;
}

// [cite: 591-599]
void leer_AD(uint8_t canal) {
    LECTURA = 0;            // Marca inicio
    ADCON0 = canal;         // Selecciona canal
    ADCON0bits.ADON = 1;    // Enciende ADC
    _delay_us(3);           // Tiempo adquisición
    ADCON0bits.GO = 1;      // Inicia
    while(ADCON0bits.GO);   // Espera
    ADCON0bits.ADON = 0;    // Apaga
    LECTURA = 1;            // Marca fin
}