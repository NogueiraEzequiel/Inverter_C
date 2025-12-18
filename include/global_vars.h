/**
 * @file global_vars.h
 * @brief Variables globales y definiciones del PDF.
 * Incluye máscaras de PID y prototipos "antes del main" [cite: 258-271, 1278-1531].
 */

#ifndef GLOBAL_VARS_H
#define GLOBAL_VARS_H

#include <stdint.h>

// --- MÁSCARAS DE BITS PID (Registro pidStat1) [cite: 258-260] ---
#define PID_ERR_Z       (1 << 0) // Error actual es cero
#define PID_A_ERR_Z     (1 << 1) // Error acumulado es cero
#define PID_ERR_SIGN    (1 << 2) // Error actual positivo
#define PID_A_ERR_SIGN  (1 << 3) // Error acumulado positivo
#define PID_P_ERR_SIGN  (1 << 4) // Error previo positivo
#define PID_MAG         (1 << 5) // AARGB > BARGB
#define PID_D_ERR_SIGN  (1 << 6) // Derivada positiva
#define PID_SIGN        (1 << 7) // Salida PID positiva

// --- MÁSCARAS DE BITS PID (Registro pidStat2) [cite: 264-268] ---
#define PID2_D_ERR_Z    (1 << 0)
#define PID2_SIGNO      (1 << 1)
#define PID2_SELECINTEG (1 << 2)

// --- CONSTANTES MATEMÁTICAS [cite: 270-271] ---
#define LSB 0
#define MSB 7

// --- DEFINICIONES DE PINES [cite: 274-288] ---
#define V_BAT       PORTAbits.RA4
#define AHORRO      PORTBbits.RB0
#define AIRE        LATBbits.LATB4
#define BUZZER      LATBbits.LATB5
#define LECTURA     LATBbits.LATB6
#define SYNC_OSC    LATBbits.LATB7
#define SPI_SCK     LATCbits.LATC3
#define SPI_SDI     LATCbits.LATC4
#define SPI_SDO     LATCbits.LATC5

// --- VARIABLES EXTERNAS (Volatile) ---
// (Lista idéntica a la anterior para compatibilidad)
extern volatile uint8_t V_PICO_0; extern volatile uint8_t V_PICO_1;
extern volatile uint8_t V_MAX_0; extern volatile uint8_t V_MAX_1;
extern volatile uint8_t SENO_0; extern volatile uint8_t SENO_1;
extern volatile uint8_t CICLO_0; extern volatile uint8_t CICLO_1;
extern volatile uint8_t INICIO_0; extern volatile uint8_t INICIO_1;
extern volatile uint8_t AA;
extern volatile uint8_t TEMPO; extern volatile uint8_t TEMP1;
extern volatile uint8_t NN; extern volatile uint8_t K;

// PID Referencias y Constantes
extern volatile uint8_t REF_ERR;
extern volatile uint8_t REF0; extern volatile uint8_t REF1;
extern volatile uint8_t U; extern volatile uint8_t U_0; extern volatile uint8_t U_1;
extern volatile uint8_t kp; extern volatile uint8_t ki; extern volatile uint8_t kd;

// PID Variables Internas
extern volatile uint8_t error0; extern volatile uint8_t error1;
extern volatile uint8_t a_Error0; extern volatile uint8_t a_Error1; extern volatile uint8_t a_Error2;
extern volatile uint8_t p_Error0; extern volatile uint8_t p_Error1;
extern volatile uint8_t d_Error0; extern volatile uint8_t d_Error1;
extern volatile uint8_t prop0; extern volatile uint8_t prop1; extern volatile uint8_t prop2;
extern volatile uint8_t integ0; extern volatile uint8_t integ1; extern volatile uint8_t integ2;
extern volatile uint8_t deriv0; extern volatile uint8_t deriv1; extern volatile uint8_t deriv2;
extern volatile uint8_t pidOut0; extern volatile uint8_t pidOut1; extern volatile uint8_t pidOut2;
extern volatile uint8_t pidStat1; extern volatile uint8_t pidStat2;
extern volatile uint8_t percent_err;
extern volatile uint8_t derivCount;

// Medición y Protección
extern volatile uint8_t CUENTA; extern volatile uint8_t C_MAXIMA;
extern volatile uint8_t I_MINIMA;
extern volatile uint8_t V_SALIDA; extern volatile uint8_t I_SALIDA;
extern volatile uint8_t I_MAX; extern volatile uint8_t PP; extern volatile uint8_t PP_MAX;

// Temperaturas
extern volatile uint8_t T_DISIP; extern volatile uint8_t T_DISIP1; extern volatile uint8_t T_DISIP2;
extern volatile uint8_t HIS_DIS1; extern volatile uint8_t HIS_DIS2;
extern volatile uint8_t T_TRAFO; extern volatile uint8_t T_TRAFO1; extern volatile uint8_t T_TRAFO2;
extern volatile uint8_t HIS_TRA1; extern volatile uint8_t HIS_TRA2;

// Estados
extern volatile uint8_t PREVIO; extern volatile uint8_t ESTADO;

// Matemáticas (Argumentos)
extern volatile uint8_t AARGB0; extern volatile uint8_t AARGB1; extern volatile uint8_t AARGB2; extern volatile uint8_t AARGB3; extern volatile uint8_t AARGB4;
extern volatile uint8_t BARGB0; extern volatile uint8_t BARGB1; extern volatile uint8_t BARGB2; extern volatile uint8_t BARGB3;
extern volatile uint8_t TEMPW; extern volatile uint8_t TEMPST; // Resguardo ISR

// --- PROTOTIPOS DE FUNCIONES (Antes del Main) ---
void inicializar_pines(void);
void inicializar_puertos(void);
void inicializar_adc(void);
void inicializar_interrupciones(void);
void inicializar_spi(void);
void inicializar_pwm_timer2(void);
void inicializar_variables(void);

void leer_AD(uint8_t canal);
void i_salida(void);
void temperat(void);
void encender(void);
void apagar(void);
void pid(void);
void prueba(void);
void out_fija(void);
void apagar_1(void);
void enviar(void);

// Subrutinas PID y Matemáticas (Necesarias globalmente por "Prototipos antes del Main")
void PidInitialize(void);
void pid_1(void);
void pid_2(void);
void pid_3(void);
void pid_4(void);
void calculos_sinusoide(void);
void ccpr1(void);

// Funciones Matemáticas Auxiliares (Implementadas en control.c)
void FXM1616U(void);
void FXM2416U(void);
void FXD2416U(void);
void _24_BitAdd(void);
void MagAndSub(void);
void SpecSign(void);

#endif // GLOBAL_VARS_H