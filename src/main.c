/**
 * @file main.c
 * @brief Programa principal e instanciación de variables globales.
 * Basado en las páginas 12, 13 y 14 del documento.
 */

#include "../include/config.h"
#include "../include/global_vars.h"
#include <xc.h>

// --- Instanciación de Variables Globales (Memoria Real) ---
// [cite: 68-248] Se definen aquí las variables declaradas en global_vars.h

// PWM y Generación
volatile uint8_t INICIO_0;
volatile uint8_t INICIO_1;
volatile uint8_t AA;
volatile uint8_t TEMPO, TEMP1;
volatile uint8_t NN;
volatile uint8_t K;
volatile uint8_t V_PICO_0;
volatile uint8_t V_PICO_1;
volatile uint8_t V_MAX_0;
volatile uint8_t V_MAX_1;
volatile uint8_t SENO_0;
volatile uint8_t SENO_1;
volatile uint8_t CICLO_0;
volatile uint8_t CICLO_1;

// PID
volatile uint8_t REF_ERR;
volatile uint8_t REF0, REF1;
volatile uint8_t U;
volatile uint8_t U_0, U_1;
volatile uint8_t pidStat1;
volatile uint8_t pidStat2;
volatile uint8_t percent_err;
volatile uint8_t error0; // Añadida por contexto del PID
volatile uint8_t error1;

// Protección y Medición
volatile uint8_t CUENTA;
volatile uint8_t C_MAXIMA;
volatile uint8_t I_MINIMA;
volatile uint8_t V_SALIDA;
volatile uint8_t I_SALIDA;
volatile uint8_t I_MAX;
volatile uint8_t I_PICO;
volatile uint8_t PP;
volatile uint8_t PP_MAX;

// Térmico
volatile uint8_t T_DISIP;
volatile uint8_t T_DISIP1;
volatile uint8_t T_DISIP2;
volatile uint8_t HIS_DIS1;
volatile uint8_t HIS_DIS2;
volatile uint8_t T_TRAFO;
volatile uint8_t T_TRAFO1;
volatile uint8_t T_TRAFO2;
volatile uint8_t HIS_TRA1;
volatile uint8_t HIS_TRA2;

// Estados
volatile uint8_t PREVIO;
volatile uint8_t ESTADO;

// Matemáticas y Temporales
volatile uint8_t AARGB0; volatile uint8_t AARGB1; volatile uint8_t AARGB2; volatile uint8_t AARGB3;
volatile uint8_t BARGB0; volatile uint8_t BARGB1; volatile uint8_t BARGB2; volatile uint8_t BARGB3;
volatile uint8_t TEMPW; volatile uint8_t TEMPST;
volatile uint8_t TEMP_A0; volatile uint8_t TEMP_A1;
volatile uint8_t TEMP_B0; volatile uint8_t TEMP_B1; 
// (Agrega las demás variables temporales si el compilador las pide, como TEMP_PH, TEMP_PL, etc.)

/**
 * @brief Función principal del Inversor.
 * [cite: 455] Implementa el bucle infinito y la gestión de estados.
 */
void main(void) {
    // Inicialización del sistema [cite: 457-463]
    inicializar_pines();
    inicializar_puertos();
    inicializar_adc();
    inicializar_interrupciones();
    inicializar_spi();
    inicializar_pwm_timer2();
    inicializar_variables();

    while (1) { // [cite: 464]
        // Inicialización de seguridad [cite: 466-470]
        PORTCbits.RC0 = 0;      // Garantiza Q del FF = 0
        PORTBbits.RB1 = 1;      // Reset del flip-flop U14
        while (PORTCbits.RC6 == 0); // Espera a que el hardware confirme reset
        PORTBbits.RB1 = 0;      // Libera reset del FF

        // Lectura referencia de tensión [cite: 472-473]
        leer_AD(3); // AN3
        REF_ERR = ADRESH;

        // Preparación para encendido [cite: 475-477]
        TEMPO = V_MAX_0;
        TEMP1 = V_MAX_1;
        encender(); // Arranque suave

        if (PREVIO & (1 << 7)) { // ¿Salió por sobrecarga? [cite: 478]
            apagar();
            continue;
        }

        // Inicio del ciclo principal [cite: 483]
        NN = 1;

        while (1) { // [cite: 486]
            SYNC_OSC = 1;
            LECTURA = 1;
            K = 0;
            SYNC_OSC = 0;
            
            V_PICO_0 = TEMPO;
            V_PICO_1 = TEMP1;

            // Lectura de tensión de salida [cite: 495]
            leer_AD(0); // AN0
            V_SALIDA = ADRESH;

            pid();      // Lazo de control [cite: 497]
            i_salida(); // Protección por corriente [cite: 498]
            
            if (PREVIO & (1 << 1)) // [cite: 498]
                break;

            temperat(); // Protección térmica [cite: 500]
            if ((PREVIO & (1 << 3)) || (PREVIO & (1 << 5))) // [cite: 501]
                break;

            // Supervisión batería [cite: 502-511]
            if (V_BAT == 0) {
                ESTADO |= (1 << 5);
                break;
            } else {
                ESTADO &= ~(1 << 5);
            }

            // Bajo consumo [cite: 513]
            if (AHORRO) {
                ESTADO |= (1 << 0);
                leer_AD(8); // AN8
                I_MINIMA = ADRESH;

                if (I_SALIDA < I_MINIMA) { // [cite: 518]
                    ESTADO |= (1 << 1); // Entró en bajo consumo
                    LECTURA = 0;
                    while (K != 2); // Espera fin del ciclo de 50 Hz

                    // Prueba de carga [cite: 525]
                    prueba();
                    
                    // Restaurar valores para volver a generar [cite: 527]
                    TEMPO = V_MAX_0;
                    TEMP1 = V_MAX_1;
                    continue;
                }
            } else {
                ESTADO &= ~(1 << 0); // [cite: 534]
            }

            LECTURA = 0;
            // Espera fin del ciclo de 50 Hz [cite: 538]
            while (K != 2) {
                // Protección hardware externa [cite: 540]
                if (PORTCbits.RC6 == 0)
                    break;
            }
            if (PORTCbits.RC6 == 0) break; // Salida del while si hubo fallo
        }

        // Apagado del inversor [cite: 544]
        apagar();
        TEMPO = V_MAX_0;
        TEMP1 = V_MAX_1;
    }
}