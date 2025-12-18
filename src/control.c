/**
 * @file control.c
 * @brief Lógica de control, protecciones y máquinas de estado.
 * Basado en páginas 16 a 27 del documento.
 */

#include "../include/config.h"
#include "../include/global_vars.h"
#include <xc.h>

#define _XTAL_FREQ 20000000UL

// --- Prototipos locales de PID (si no están en header) ---
void pid_1(void);
void pid_2(void);
void pid_3(void); // Cálculo de términos (implícito en PDF)
void pid_4(void); // Suma final (implícito en PDF)

//
void i_salida(void) {
    // Leer corriente de salida (AN1)
    // Nota: El PDF dice AN1 en el texto, pero en el código a veces usa AN4 o máscara.
    // Usaremos la máscara binaria 0b00000100 que corresponde a AN1 (Canal 1).
    leer_AD(0b00000100); 
    I_SALIDA = ADRESH;

    // Comparación con I_MAX
    if (I_SALIDA < I_MAX) {
        PP = 0; // Corriente normal
        PREVIO &= ~(1 << 1); // No apagar (Bit 1 en 0)
        ESTADO &= ~(1 << 4); // Sin sobrecorriente (Bit 4 en 0)
        SPI_SDO = 0;         // LED apagado
    } else {
        // Sobrecorriente detectada
        PP++;
        if (PP >= PP_MAX) {
            PREVIO |= (1 << 1); // Solicita apagado
        }
        ESTADO |= (1 << 4); // Estado: sobrecorriente
        SPI_SDO = 1;        // LED de sobrecorriente
    }
}

//
void pid(void) {
    pid_1();
    pid_2();
    pid_3();
    pid_4();
}

//
void encender(void) {
    // Inicialización
    NN = 1;
    V_PICO_0 = INICIO_0;
    V_PICO_1 = INICIO_1;
    TMR2 = 0;              // Reset Timer2
    T2CONbits.TMR2ON = 1;  // Enciende Timer2 (arranca PWM)

    // Bucle de arranque suave
    while (1) {
        // Protección por hardware
        if (PORTCbits.RC6 == 0) {
            ESTADO |= (1 << 6);
            PREVIO |= (1 << 7); // Indica que hay que apagar
            return;
        } else {
            ESTADO &= ~(1 << 6);
        }

        // Inicio de ciclo de 50 Hz
        SYNC_OSC = 1;
        K = 0;
        SYNC_OSC = 0;

        // ¿Llegó a VMAX? (comparación de 16 bits)
        if ((V_PICO_0 > V_MAX_0) || ((V_PICO_0 == V_MAX_0) && (V_PICO_1 > V_MAX_1))) {
            PREVIO &= ~(1 << 7); // Arranque OK (Bit 7 en 0)
            return;
        }

        // V_PICO = V_PICO + AA
        uint16_t v = ((uint16_t)V_PICO_0 << 8) | V_PICO_1;
        v += AA;
        V_PICO_0 = (uint8_t)(v >> 8);
        V_PICO_1 = (uint8_t)(v & 0xFF);

        // Protección por corriente
        i_salida();
        if (PREVIO & (1 << 1)) {
            PREVIO |= (1 << 7);
            return;
        }

        // Protección por temperatura
        temperat();
        if ((PREVIO & (1 << 3)) || (PREVIO & (1 << 5))) {
            PREVIO |= (1 << 7);
            return;
        }

        // Protección por batería
        if (V_BAT == 0) {
            ESTADO |= (1 << 5);
            PREVIO |= (1 << 7);
            return;
        } else {
            ESTADO &= ~(1 << 5);
        }

        /* Espera fin del ciclo de 50 Hz */
        while (K != 2);
    }
}

//
void apagar(void) {
    BUZZER = 1; // Buzzer ON
    NN = 1;
    
    // Bucle de apagado suave (Soft-Stop)
    while (1) {
        // Protección por hardware
        if (PORTCbits.RC6 == 0) {
            ESTADO |= (1 << 6); // protección hardware activa
            T2CONbits.TMR2ON = 0; // apago Timer2 (PWM)
            
            // Espera reset manual
            while (PORTCbits.RC6 == 0);
            goto rearme;
        } else {
            ESTADO &= ~(1 << 6);
        }

        // Sincronismo ciclo 50 Hz
        SYNC_OSC = 1;
        K = 0;
        SYNC_OSC = 0;

        // V_PICO > AA?
        uint16_t v_pico = ((uint16_t)V_PICO_0 << 8) | V_PICO_1;
        if (v_pico > AA) {
            v_pico -= AA;
            V_PICO_0 = (v_pico >> 8) & 0xFF;
            V_PICO_1 = v_pico & 0xFF;
            // Esperar fin del ciclo de 50 Hz
            while (K != 2);
        } else {
            // Apagado duro del puente H
            PORTCbits.RC0 = 1; // dispara FF D -> apaga IR2110
            while (PORTCbits.RC6 == 0);
            PORTCbits.RC0 = 0; // libera FF D
            T2CONbits.TMR2ON = 0;
            break; // salgo del soft-stop
        }
    }

    // Evaluación de rearme
    if (PREVIO & (1 << 1)) {
        // Apagado por sobrecorriente -> esperar reset
        while (1) {
            if (PORTCbits.RC6 == 1) { // Nota: PDF dice RC6==1 para salir, verificar lógica pulsador
                 // Se asume que espera acción de usuario
                 // Si es pulsador momentáneo, la lógica puede variar
                 // Copiamos tal cual PDF que parece esperar estado alto estable
            }
             // Bucle infinito hasta reset? El PDF es ambiguo aquí.
             // Asumimos break si se cumple condición, o loop
             if (PORTCbits.RC6 == 1) break; 
        }
    }

rearme:
    // Verificaciones antes de reencender
    do {
        temperat();
    } while ((PREVIO & (1 << 3)) || (PREVIO & (1 << 5)));

    if (!V_BAT) {
        ESTADO |= (1 << 5);
        goto rearme;
    }

    // Apago buzzer
    BUZZER = 0;
    
    // Reset del flip-flop D
    PORTBbits.RB1 = 1;
    while (PORTCbits.RC6 == 0);
    PORTBbits.RB1 = 0;

    // Intento reencender
    encender();
    
    // Si falló, repetir apagado
    if (PREVIO & (1 << 7))
        apagar();
}

//
void apagar_1(void) {
    uint16_t v_pico;
    NN = 1;
    while (1) {
        if (PORTCbits.RC6 == 0) {
            ESTADO |= (1 << 6);
            PREVIO |= (1 << 7);
            return;
        } else {
            ESTADO &= ~(1 << 6);
        }

        SYNC_OSC = 1;
        K = 0;
        SYNC_OSC = 0;

        v_pico = ((uint16_t)V_PICO_0 << 8) | V_PICO_1;
        if (v_pico <= AA) {
            return;
        }
        
        // Decremento suave
        v_pico -= AA;
        V_PICO_0 = (v_pico >> 8) & 0xFF;
        V_PICO_1 = v_pico & 0xFF;

        while (K != 2);
    }
}

//
void out_fija(void) {
    CUENTA = 0;
    NN = 1;
    PREVIO &= ~(1 << 6); // seguir en Bajo Consumo
    
    V_PICO_0 = V_MAX_0;
    V_PICO_1 = V_MAX_1;

    while (1) {
        if (PORTCbits.RC6 == 0) {
            ESTADO |= (1 << 6);
            PREVIO |= (1 << 7); // forzar apagado
            return;
        } else {
            ESTADO &= ~(1 << 6);
        }

        SYNC_OSC = 1;
        K = 0;
        SYNC_OSC = 0;

        i_salida();
        if (PREVIO & (1 << 1)) {
            PREVIO |= (1 << 7); // sobrecorriente
            return;
        }

        temperat();
        if ((PREVIO & (1 << 3)) || (PREVIO & (1 << 5))) {
            PREVIO |= (1 << 7); // sobretemperatura
            return;
        }

        if (!V_BAT) {
            ESTADO |= (1 << 5);
            PREVIO |= (1 << 7);
            return;
        } else {
            ESTADO &= ~(1 << 5);
        }

        // Medición de corriente para Bajo Consumo
        leer_AD(1); // AN1
        I_SALIDA = ADRESH;
        if (I_SALIDA >= I_MINIMA) {
            PREVIO |= (1 << 6); // hay carga -> salir de Bajo Consumo
        }

        while (K != 2);

        CUENTA++;
        if (CUENTA == C_MAXIMA) {
            PREVIO &= ~(1 << 7); // salida normal
            return;
        }
    }
}

//
void prueba(void) {
    PORTCbits.RC7 = 1; // Mantiene descargado el capacitor RC

inicio_prueba:
    apagar_1(); // Apagado suave por software
    if (PREVIO & (1 << 7)) {
        apagar();
    }

    T2CONbits.TMR2ON = 0; // PWM detenido
    PORTCbits.RC7 = 0;    // Carga del capacitor RC
    
    // Espera tensión umbral
    do {
        leer_AD(9); // AN9
    } while (ADRESH <= 140);

    encender();
    if (PREVIO & (1 << 7)) {
        apagar();
    }

    out_fija();
    if (PREVIO & (1 << 7)) {
        apagar();
    }

    if (PREVIO & (1 << 6)) {
        ESTADO &= ~(1 << 1); // Limpia flag Bajo Consumo activo
        return;
    }

    if (AHORRO) {
        goto inicio_prueba;
    }

    ESTADO &= ~(1 << 0);
    ESTADO &= ~(1 << 1);
}

//
void temperat(void) {
    // Lectura
    leer_AD(2); T_DISIP = ADRESH; // AN2 disipador
    leer_AD(4); T_TRAFO = ADRESH; // AN4 trafo

    // Disipador
    if (PREVIO & (1 << 3)) { // Puente apagado por disipador
        if (T_DISIP > HIS_DIS2) {
            AIRE = 1; ESTADO |= (1 << 3); SPI_SDI = 1;
        } else if (T_DISIP > HIS_DIS1) {
            AIRE = 1; PREVIO &= ~(1 << 3); PREVIO |= (1 << 2);
        } else {
            AIRE = 0; PREVIO &= ~((1 << 3) | (1 << 2)); ESTADO &= ~(1 << 3); SPI_SDI = 0;
        }
    } else if (PREVIO & (1 << 2)) { // Fan ON, puente ON
        if (T_DISIP > HIS_DIS1) {
            AIRE = 1;
        } else {
            AIRE = 0; PREVIO &= ~(1 << 2);
        }
    } else { // Normal
        if (T_DISIP > T_DISIP2) {
            AIRE = 1; PREVIO |= (1 << 3); ESTADO |= (1 << 3); SPI_SDI = 1;
        } else if (T_DISIP > T_DISIP1) {
            AIRE = 1; PREVIO |= (1 << 2);
        } else {
            AIRE = 0; PREVIO &= ~((1 << 3) | (1 << 2)); ESTADO &= ~(1 << 3); SPI_SDI = 0;
        }
    }

    // Trafo (Lógica similar)
    if (PREVIO & (1 << 5)) {
        if (T_TRAFO > HIS_TRA2) {
            AIRE = 1; ESTADO |= (1 << 2); SPI_SCK = 1;
        } else if (T_TRAFO > HIS_TRA1) {
            AIRE = 1; PREVIO &= ~(1 << 5); PREVIO |= (1 << 4);
        } else {
            PREVIO &= ~((1 << 5) | (1 << 4)); ESTADO &= ~(1 << 2); SPI_SCK = 0;
        }
    } else if (PREVIO & (1 << 4)) {
        if (T_TRAFO > HIS_TRA1) AIRE = 1;
        else PREVIO &= ~(1 << 4);
    } else {
        if (T_TRAFO > T_TRAFO2) {
            AIRE = 1; PREVIO |= (1 << 5); ESTADO |= (1 << 2); SPI_SCK = 1;
        } else if (T_TRAFO > T_TRAFO1) {
            AIRE = 1; PREVIO |= (1 << 4);
        } else {
            PREVIO &= ~((1 << 5) | (1 << 4)); ESTADO &= ~(1 << 2); SPI_SCK = 0;
        }
    }
}

//
void enviar(void) {
    PIR1bits.SSPIF = 0;
    SSPBUF = V_SALIDA; while (!PIR1bits.SSPIF); PIR1bits.SSPIF = 0;
    SSPBUF = I_SALIDA; while (!PIR1bits.SSPIF); PIR1bits.SSPIF = 0;
    SSPBUF = T_DISIP;  while (!PIR1bits.SSPIF); PIR1bits.SSPIF = 0;
    SSPBUF = T_TRAFO;  while (!PIR1bits.SSPIF); PIR1bits.SSPIF = 0;
    SSPBUF = ESTADO;   while (!PIR1bits.SSPIF); PIR1bits.SSPIF = 0;
}

//
void pid_1(void) {
    if (V_SALIDA == REF_ERR) {
        percent_err = 0;
        // pidStat1.err_sign = 1; // Simulado con bit
        pidStat1 |= (1 << 2); // Error positivo o cero
        return;
    }
    if (V_SALIDA > REF_ERR) {
        percent_err = V_SALIDA - REF_ERR;
        // pidStat1.err_sign = 0;
        pidStat1 &= ~(1 << 2); // Error negativo
    } else {
        percent_err = REF_ERR - V_SALIDA;
        // pidStat1.err_sign = 1;
        pidStat1 |= (1 << 2); // Error positivo
    }
    // Saturación
    if (percent_err > 100) percent_err = 100;
}

//
void pid_2(void) {
    // Escalado error = U * percent_err
    // Nota: error se declara como uint16_t en PDF para esta cuenta?
    // Usamos variables de 16 bits implícitamente o variables globales error0/1
    uint16_t err_calc = (uint16_t)U * (uint16_t)percent_err;
    error0 = (uint8_t)(err_calc >> 8);
    error1 = (uint8_t)(err_calc & 0xFF);

    if (err_calc == 0) {
        pidStat1 |= (1 << 0); // err_z = 1
        return;
    }
    pidStat1 &= ~(1 << 0); // err_z = 0
}

// Implementación dummy de pid_3 y pid_4 para completar la llamada en main
// El PDF no muestra el código, pero el main lo llama.
void pid_3(void) { /* Cálculo Proporcional, Integral, Derivativo */ }
void pid_4(void) { /* Suma final de términos */ }