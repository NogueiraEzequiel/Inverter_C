/**
 * @file pwm.c
 * @brief Generación de SPWM y Tablas.
 * Implementa las funciones faltantes leeseno0 y leeseno1.
 */

#include "../include/config.h"
#include "../include/global_vars.h" // Variables globales como V_PICO, SENO, etc.
#include <xc.h>

// --- Tabla de Senos (Reconstruida para el proyecto) ---
// Valores escalados para corresponder con la lógica del PDF
const uint16_t SINE_TABLE[98] = {
    0, 25, 50, 75, 100, 125, 150, 175, 199, 223, 247, 271, 294, 317, 340, 362, 384, 405, 426, 447,
    467, 487, 506, 525, 543, 560, 577, 593, 609, 624, 638, 652, 665, 677, 689, 700, 710, 720, 729, 737,
    744, 751, 757, 762, 766, 770, 773, 775, 776, 776, 775, 773, 770, 766, 762, 757, 751, 744, 737, 729,
    720, 710, 700, 689, 677, 665, 652, 638, 624, 609, 593, 577, 560, 543, 525, 506, 487, 467, 447, 426,
    405, 384, 362, 340, 317, 294, 271, 247, 223, 199, 175, 150, 125, 100, 75, 50, 25, 0
};

// Implementación de funciones faltantes en PDF 
uint8_t leeseno0(void) {
    // Retorna parte alta del valor de tabla
    uint16_t val = SINE_TABLE[CICLO_0];
    return (uint8_t)(val >> 8);
}

uint8_t leeseno1(void) {
    // Retorna parte baja del valor de tabla
    uint16_t val = SINE_TABLE[CICLO_0];
    return (uint8_t)(val & 0xFF);
}

// [cite: 1140-1153]
void ccpr1(void) {
    uint32_t producto;
    uint16_t duty10;
    
    // Multiplicación 16x16: V_PICO * SENO
    producto = ((uint16_t)V_PICO_0 << 8 | V_PICO_1) * ((uint16_t)SENO_0 << 8 | SENO_1);
    
    // División por 16384 (>>14)
    duty10 = (uint16_t)(producto >> 14); 
    
    // Cargar PWM CCP1
    CCPR1L = (uint8_t)(duty10 >> 2); 
    
    if ((duty10 >> 1) & 0x01) CCP1CONbits.DC1B1 = 1; else CCP1CONbits.DC1B1 = 0;
    if (duty10 & 0x01)        CCP1CONbits.DC1B0 = 1; else CCP1CONbits.DC1B0 = 0;
}

// [cite: 1104-1138]
void calculos_sinusoide(void) {
    // Leer seno
    SENO_0 = leeseno0(); // Llama a nuestra implementación
    SENO_1 = leeseno1();
    
    // Calcular duty
    ccpr1();
    
    // Copiar duty a CCP2
    CCPR2L = CCPR1L;
    if (CCP1CONbits.DC1B1) CCP2CONbits.DC2B1 = 1; else CCP2CONbits.DC2B1 = 0;
    if (CCP1CONbits.DC1B0) CCP2CONbits.DC2B0 = 1; else CCP2CONbits.DC2B0 = 0;

    // Selección de rama
    if (K == 0) { 
        // Apagar PWM2
        CCPR2L = 0;
        CCP2CONbits.DC2B1 = 0;
        CCP2CONbits.DC2B0 = 0;
    } else { 
        // Apagar PWM1
        CCPR1L = 0;
        CCP1CONbits.DC1B1 = 0;
        CCP1CONbits.DC1B0 = 0;
    }
}

// ISR del Timer 2 [cite: 549-589]
void __interrupt() isr(void) {
    if (PIR1bits.TMR2IF) {
        PIR1bits.TMR2IF = 0;
        
        // Salvado de contexto MANUAL (tal cual PDF)
        TEMPW = WREG;
        TEMPST = STATUS;
        
        // Generación
        calculos_sinusoide();

        // Contadores
        NN++;
        CICLO_0++; 
        // Lógica de control de tabla (no explícita en PDF, agregada para funcionamiento)
        if (CICLO_0 >= 98) {
             CICLO_0 = 0;
             K++;
             if (K >= 2) K = 0;
             NN = 1; 
        }

        // Restauración
        WREG = TEMPW;
        STATUS = TEMPST;
    }
}