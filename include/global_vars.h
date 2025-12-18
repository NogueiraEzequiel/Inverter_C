/**
 * @file global_vars.h
 * @brief Declaración de variables globales del sistema.
 * Basado en las páginas 2 a 7 del documento.
 */

#ifndef GLOBAL_VARS_H
#define GLOBAL_VARS_H

#include <stdint.h>

// --- Definiciones de Pines (Pág 7-8) ---
#define V_BAT       PORTAbits.RA4   // [cite: 274]
#define AHORRO      PORTBbits.RB0   // [cite: 277]
#define AIRE        LATBbits.LATB4  // [cite: 280]
#define BUZZER      LATBbits.LATB5  // [cite: 282]
#define LECTURA     LATBbits.LATB6  // [cite: 283]
#define SYNC_OSC    LATBbits.LATB7  // [cite: 284]
#define SPI_SCK     LATCbits.LATC3  // [cite: 286]
#define SPI_SDI     LATCbits.LATC4  // [cite: 286]
#define SPI_SDO     LATCbits.LATC5  // [cite: 288]

// --- Variables Externas (Volatile) ---
// Usamos 'extern' para que se definan realmente en main.c

// PWM y Seno [cite: 80-95]
extern volatile uint8_t V_PICO_0;
extern volatile uint8_t V_PICO_1;
extern volatile uint8_t V_MAX_0;
extern volatile uint8_t V_MAX_1;
extern volatile uint8_t SENO_0;
extern volatile uint8_t SENO_1;
extern volatile uint8_t CICLO_0;
extern volatile uint8_t CICLO_1;
extern volatile uint8_t INICIO_0; // [cite: 68]
extern volatile uint8_t INICIO_1;
extern volatile uint8_t AA;       // [cite: 71]
extern volatile uint8_t TEMPO;    // [cite: 72]
extern volatile uint8_t TEMP1;

// Contadores [cite: 75-76]
extern volatile uint8_t NN;
extern volatile uint8_t K;

// PID Referencias [cite: 97-104]
extern volatile uint8_t REF_ERR;
extern volatile uint8_t REF0;
extern volatile uint8_t REF1;
extern volatile uint8_t U;
extern volatile uint8_t U_0;
extern volatile uint8_t U_1;

// Medición y Protección [cite: 106-121]
extern volatile uint8_t CUENTA;
extern volatile uint8_t C_MAXIMA;
extern volatile uint8_t I_MINIMA;
extern volatile uint8_t V_SALIDA;
extern volatile uint8_t I_SALIDA;
extern volatile uint8_t I_MAX;
extern volatile uint8_t PP;
extern volatile uint8_t PP_MAX;

// Temperaturas [cite: 121-136]
extern volatile uint8_t T_DISIP;
extern volatile uint8_t T_DISIP1;
extern volatile uint8_t T_DISIP2;
extern volatile uint8_t HIS_DIS1;
extern volatile uint8_t HIS_DIS2;
extern volatile uint8_t T_TRAFO;
extern volatile uint8_t T_TRAFO1;
extern volatile uint8_t T_TRAFO2;
extern volatile uint8_t HIS_TRA1; // [cite: 138]
extern volatile uint8_t HIS_TRA2;

// Estados [cite: 140]
extern volatile uint8_t PREVIO;
extern volatile uint8_t ESTADO;

// Variables Matemáticas (Argumentos) [cite: 155-169]
extern volatile uint8_t AARGB0;
extern volatile uint8_t AARGB1;
extern volatile uint8_t AARGB2;
extern volatile uint8_t AARGB3;
extern volatile uint8_t BARGB0;
extern volatile uint8_t BARGB1;
extern volatile uint8_t BARGB2;
extern volatile uint8_t BARGB3;

// Variables de Resguardo ISR [cite: 184-198]
extern volatile uint8_t TEMPW;   // [cite: 250]
extern volatile uint8_t TEMPST;  // [cite: 251]
extern volatile uint8_t TEMP_A0;
extern volatile uint8_t TEMP_A1;
extern volatile uint8_t TEMP_B0;
extern volatile uint8_t TEMP_B1;
// ... (se pueden agregar el resto si el código las usa explícitamente)

// PID Variables Internas [cite: 203-248]
extern volatile uint8_t error0;
extern volatile uint8_t error1;
extern volatile uint8_t pidStat1;
extern volatile uint8_t pidStat2;
extern volatile uint8_t percent_err;

// --- Prototipos de Funciones (Tal cual el PDF) ---
void inicializar_pines(void);      // [cite: 297]
void inicializar_puertos(void);    // [cite: 317]
void inicializar_adc(void);        // [cite: 331]
void inicializar_interrupciones(void); // [cite: 340]
void inicializar_spi(void);        // [cite: 350]
void inicializar_pwm_timer2(void); // [cite: 359]
void inicializar_variables(void);  // [cite: 368]

void leer_AD(uint8_t canal);       // [cite: 591]
void i_salida(void);               // [cite: 601]
void temperat(void);               // [cite: 961]
void encender(void);               // [cite: 650]
void apagar(void);                 // [cite: 726]
void pid(void);                    // [cite: 641]
void prueba(void);                 // [cite: 914]
void out_fija(void);               // [cite: 843]
void apagar_1(void);               // [cite: 806]
void calculos_sinusoide(void);     // [cite: 1104]
void ccpr1(void);                  // [cite: 1141]

// Funciones auxiliares mencionadas pero no definidas explícitamente en el PDF
// Las definiremos nosotros siguiendo la lógica del PDF
uint8_t leeseno0(void);
uint8_t leeseno1(void);
void pid_1(void);
void pid_2(void);
void pid_3(void);
void pid_4(void);

#endif // GLOBAL_VARS_H