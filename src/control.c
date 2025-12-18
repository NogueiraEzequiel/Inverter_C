/**
 * @file control.c
 * @brief Lógica PID y Protecciones.
 * Incluye implementación de funciones matemáticas en el PDF.
 */

#include "../include/config.h"
#include "../include/global_vars.h"
#include <xc.h>

// --- Prototipos de funciones matemáticas ---
void FXM1616U(void);
void FXM2416U(void);
void FXD2416U(void);
void _24_BitAdd(void);
void MagAndSub(void);
void SpecSign(void);
// Prototipos internos PID
void Proportional(void);
void Integral(void);
void Derivative(void);
void GetPidResult(void);
void PidInterrupt(void);

// --- FUNCIONES DE PROTECCIÓN Y ESTADO ---

void i_salida(void) {
    // Leer corriente de salida (AN1)
    leer_AD(0b00000100);  // AN1
    I_SALIDA = ADRESH;

    // Comparación con I_MAX
    if (I_SALIDA < I_MAX)
    {
        PP = 0;  // Corriente normal
        PREVIO &= ~(1 << 1);  // No apagar
        ESTADO &= ~(1 << 4);  // Sin sobrecorriente
        SPI_SDO = 0;  // LED apagado
    }
    else
    {

        // Sobrecorriente detectada
        PP++;
        if (PP >= PP_MAX)
        {
            PREVIO |= (1 << 1);  // Solicita apagado
            ESTADO |= (1 << 4);  // Estado: sobrecorriente
            SPI_SDO = 1;  // LED de sobrecorriente
        }
        else
        {
            PREVIO &= ~(1 << 1);  // Todavía no apaga
            ESTADO &= ~(1 << 4);
            SPI_SDO = 0;
        }
    }
}

void pid(void) {
    pid_1();
    pid_2();
    pid_3();
    pid_4();
}

void encender(void) {
    // Inicialización
    NN = 1;
    V_PICO_0 = INICIO_0;
    V_PICO_1 = INICIO_1;
    TMR2 = 0;  // Reset Timer2
    T2CONbits.TMR2ON = 1;  // Enciende Timer2 (arranca PWM)

    // Bucle de arranque suave
    while (1)
    {
        // Protección por hardware
        if (PORTCbits.RC6 == 0)
        {
            ESTADO |= (1 << 6);
            PREVIO |= (1 << 7);  // Indica que hay que apagar
            return;
        }
        else
        {
            ESTADO &= ~(1 << 6);
        }

        // Inicio de ciclo de 50 Hz
        SYNC_OSC = 1;
        K = 0;
        SYNC_OSC = 0;

        // ¿Llegó a V_MAX? (comparación de 16 bits)
        if ((V_PICO_0 > V_MAX_0) ||
            ((V_PICO_0 == V_MAX_0) && (V_PICO_1 > V_MAX_1)))
        {
            PREVIO &= ~(1 << 7);  // Arranque OK
            return;
        }

        // V_PICO = V_PICO + AA
        {
            uint16_t v = ((uint16_t)V_PICO_0 << 8) | V_PICO_1;
            v += AA;
            V_PICO_0 = (uint8_t)(v >> 8);
            V_PICO_1 = (uint8_t)(v & 0xFF);
        }

        // Protección por corriente
        i_salida();
        if (PREVIO & (1 << 1))
        {
            PREVIO |= (1 << 7);
            return;
        }

        // Protección por temperatura
        temperat();
        if ((PREVIO & (1 << 3)) || (PREVIO & (1 << 5)))
        {
            PREVIO |= (1 << 7);
            return;
        }

        // Protección por batería
        if (V_BAT == 0)
        {
            ESTADO |= (1 << 5);
            PREVIO |= (1 << 7);
            return;
        }
        else
        {
            ESTADO &= ~(1 << 5);
        }

        /* Espera fin del ciclo de 50 Hz */
        while (K != 2)
            ;
    }
}

void apagar(void) {
    // Buzzer ON
    BUZZER = 1;
    NN = 1;
    while (1)
    {
         // Protección por hardware
        if (PORTCbits.RC6 == 0)
        {
            ESTADO |= (1 << 6);  // protección hardware activa
            T2CONbits.TMR2ON = 0;  // apago Timer2 (PWM)

            // Espera reset manual
            while (PORTCbits.RC6 == 0)
                ;
            goto rearme;
        }
        else
        {
            ESTADO &= ~(1 << 6);
        }

        // Sincronismo ciclo 50 Hz
        SYNC_OSC = 1;
        K = 0;
        SYNC_OSC = 0;

         // ¿V_PICO > AA?
         uint16_t v_pico = ((uint16_t)V_PICO_0 << 8) | V_PICO_1;
        if (v_pico > AA)
        {
            v_pico -= AA;
            V_PICO_0 = (v_pico >> 8) & 0xFF;
            V_PICO_1 = v_pico & 0xFF;

            // Esperar fin del ciclo de 50 Hz
            while (K != 2)
                ;
        }
        else
        {

             // Apagado duro del puente H
             PORTCbits.RC0 = 1;   // dispara FF D ->apaga IR2110
            while (PORTCbits.RC6 == 0)
                ;

            PORTCbits.RC0 = 0;  // libera FF D
            T2CONbits.TMR2ON = 0;
            break;  // salgo del soft-stop
        }
    }

     // Evaluación de rearme
     if (PREVIO & (1 << 1))
    {
        // Apagado por sobrecorriente -> esperar reset
        while (1)
        {
            if (PORTCbits.RC6 == 1)
                break;
        }
    }

rearme:
    // Verificaciones antes de reencender
    do
    {
        temperat();
    } while ((PREVIO & (1 << 3)) || (PREVIO & (1 << 5)));
    if (!V_BAT)
    {
        ESTADO |= (1 << 5);
        goto rearme;
    }

    // Apago buzzer
    BUZZER = 0;

    // Reset del flip-flop D
    PORTBbits.RB1 = 1;
    while (PORTCbits.RC6 == 0)
        ;
    PORTBbits.RB1 = 0;

    // Intento reencender
    encender();

    // Si falló, repetir apagado
    if (PREVIO & (1 << 7))
        apagar();
}

void apagar_1(void) {
    uint16_t v_pico;
    NN = 1;
    while (1)
    {

        // Protección por hardware
        if (PORTCbits.RC6 == 0)
        {
            ESTADO |= (1 << 6);  // Protección hardware activa
            PREVIO |= (1 << 7);  // Indica que debe ir a apagar()
            return;
        }
        else
        {
            ESTADO &= ~(1 << 6);
        }

        // Sincronización a 50 Hz
        SYNC_OSC = 1;
        K = 0;
        SYNC_OSC = 0;

        // ¿V_PICO > AA?
        v_pico = ((uint16_t)V_PICO_0 << 8) | V_PICO_1;
        if (v_pico <= AA)
        {
            return;
        }

        // Decremento suave de amplitud
        v_pico -= AA;
        V_PICO_0 = (v_pico >> 8) & 0xFF;
        V_PICO_1 = v_pico & 0xFF;

        // Espera fin del ciclo de 50 Hz
        while (K != 2)
            ;
    }
}

void out_fija(void) {
    // Inicialización
    CUENTA = 0;  // contador de ciclos de 50 Hz
    NN = 1;
    PREVIO &= ~(1 << 6);   // PREVIO<6>=0 -> seguir en Bajo Consumo

    // Fijo la amplitud de salida
    V_PICO_0 = V_MAX_0;
    V_PICO_1 = V_MAX_1;
    while (1)
    {

        // Protección por hardware
        if (PORTCbits.RC6 == 0)
        {
            ESTADO |= (1 << 6);
            PREVIO |= (1 << 7);  // forzar apagado total
            return;
        }
        else
        {
            ESTADO &= ~(1 << 6);
        }

        // Sincronización a 50 Hz
        SYNC_OSC = 1;
        K = 0;
        SYNC_OSC = 0;

        // Protección por corriente
        i_salida();
        if (PREVIO & (1 << 1))
        {
            PREVIO |= (1 << 7);  // sobrecorriente
            return;
        }

        // Protección por temperatura
        temperat();
        if ((PREVIO & (1 << 3)) || (PREVIO & (1 << 5)))
        {
            PREVIO |= (1 << 7);  // sobretemperatura
            return;
        }

        // Chequeo de batería
        if (!V_BAT)
        {
            ESTADO |= (1 << 5);
            PREVIO |= (1 << 7);
            return;
        }
        else
        {
            ESTADO &= ~(1 << 5);
        }

        // Medición de corriente para Bajo Consumo
        leer_AD(1);  // AN1 -> corriente de salida
        I_SALIDA = ADRESH;
        if (I_SALIDA >= I_MINIMA)
        {
            PREVIO |= (1 << 6);   // hay carga -> salir de Bajo Consumo
        }

        // Espera fin del ciclo de 50 Hz
        while (K != 2)
            ;

        // Incremento del contador
        CUENTA++;
        if (CUENTA == C_MAXIMA)
        {
            PREVIO &= ~(1 << 7);  // salida normal, sin falla
            return;
        }
    }
}

void prueba(void) {
    PORTCbits.RC7 = 1;  // Mantiene descargado el capacitor del RC

inicio_prueba:
    apagar_1();  // Apagado suave por software

    // Si hubo sobrecorriente durante apagar_1 -> apagar total
    if (PREVIO & (1 << 7))
    {
        apagar();
    }

    T2CONbits.TMR2ON = 0;  // Apaga el Timer2 (PWM detenido)
    PORTCbits.RC7 = 0;  // Comienza la carga del capacitor RC

    // Espera hasta que la tensión del RC supere el umbral
    do
    {
        leer_AD(9);  // AN9: tensión del capacitor RC
    }
    while (ADRESH <= 140);

    encender();  // Vuelve a encender el puente H

    // Si encender detectó falla -> apagar total
    if (PREVIO & (1 << 7))
    {
        apagar();
    }

    out_fija();  // Mantiene salida fija y evalúa carga

    // Si out_fija detectó falla -> apagar total
    if (PREVIO & (1 << 7))
    {
        apagar();
    }

    // Si se detectó carga suficiente -> salir de Bajo Consumo
    if (PREVIO & (1 << 6))
    {
        ESTADO &= ~(1 << 1);  // Limpia flag Bajo Consumo activo
        return;
    }

    // Si Bajo Consumo sigue habilitado -> repetir ciclo
    if (AHORRO)
    {
        goto inicio_prueba;
    }

    // Bajo Consumo deshabilitado desde el panel
    ESTADO &= ~(1 << 0);  // Bajo Consumo deshabilitado
    ESTADO &= ~(1 << 1);  // No en Bajo Consumo
}

void temperat(void) {
    // Lectura de temperaturas
    leer_AD(2);  // AN2 -> temperatura disipador
    T_DISIP = ADRESH;
    leer_AD(4);  // AN4 -> temperatura transformador
    T_TRAFO = ADRESH;

    // Control de temperatura del disipador
    if (PREVIO & (1 << 3))  // Puente apagado por disipador
    {
        if (T_DISIP > HIS_DIS2)
        {
            AIRE = 1;
            ESTADO |= (1 << 3);
            SPI_SDI = 1;
        }
        else if (T_DISIP > HIS_DIS1)
        {
            AIRE = 1;
            PREVIO &= ~(1 << 3);
            PREVIO |=  (1 << 2);
        }
        else
        {
            AIRE = 0;
            PREVIO &= ~((1 << 3) | (1 << 2));
            ESTADO &= ~(1 << 3);
            SPI_SDI = 0;
        }
    }
    else if (PREVIO & (1 << 2))  // Fan ON, puente ON
    {
        if (T_DISIP > HIS_DIS1)
        {
            AIRE = 1;
        }
        else
        {
            AIRE = 0;
            PREVIO &= ~(1 << 2);
        }
    }
    else  // Estado normal
    {
        if (T_DISIP > T_DISIP2)
        {
            AIRE = 1;
            PREVIO |= (1 << 3);
            ESTADO |= (1 << 3);
            SPI_SDI = 1;
        }
        else if (T_DISIP > T_DISIP1)
        {
            AIRE = 1;
            PREVIO |= (1 << 2);
        }
        else
        {
            AIRE = 0;
            PREVIO &= ~((1 << 3) | (1 << 2));
            ESTADO &= ~(1 << 3);
            SPI_SDI = 0;
        }
    }

    // Control de temperatura del transformador
    if (PREVIO & (1 << 5))  // Puente apagado por trafo
    {
        if (T_TRAFO > HIS_TRA2)
        {
            AIRE = 1;
            ESTADO |= (1 << 2);
            SPI_SCK = 1;
        }
        else if (T_TRAFO > HIS_TRA1)
        {
            AIRE = 1;
            PREVIO &= ~(1 << 5);
            PREVIO |=  (1 << 4);
        }
        else
        {
            PREVIO &= ~((1 << 5) | (1 << 4));
            ESTADO &= ~(1 << 2);
            SPI_SCK = 0;
        }
    }
    else if (PREVIO & (1 << 4))  // Fan ON, puente ON
    {
        if (T_TRAFO > HIS_TRA1)
        {
            AIRE = 1;
        }
        else
        {
            PREVIO &= ~(1 << 4);
        }
    }
    else  // Estado normal
    {
        if (T_TRAFO > T_TRAFO2)
        {
            AIRE = 1;
            PREVIO |= (1 << 5);
            ESTADO |= (1 << 2);
            SPI_SCK = 1;
        }
        else if (T_TRAFO > T_TRAFO1)
        {
            AIRE = 1;
            PREVIO |= (1 << 4);
        }
        else
        {
            PREVIO &= ~((1 << 5) | (1 << 4));
            ESTADO &= ~(1 << 2);
            SPI_SCK = 0;
        }
    }

}

void enviar(void) {
    // Enviar V_SALIDA
    PIR1bits.SSPIF = 0;  // Limpio flag SPI
    SSPBUF = V_SALIDA;  // Cargo el byte a transmitir
    while (!PIR1bits.SSPIF);  // Espero fin de transmisión
    PIR1bits.SSPIF = 0;

    // Enviar I_SALIDA
    SSPBUF = I_SALIDA;
    while (!PIR1bits.SSPIF);
    PIR1bits.SSPIF = 0;

    // Enviar temperatura del disipador
    SSPBUF = T_DISIP;
    while (!PIR1bits.SSPIF);
    PIR1bits.SSPIF = 0;

    // Enviar temperatura del transformador
    SSPBUF = T_TRAFO;
    while (!PIR1bits.SSPIF);
    PIR1bits.SSPIF = 0;

    // Enviar registro de estado
    SSPBUF = ESTADO;
    while (!PIR1bits.SSPIF);
    PIR1bits.SSPIF = 0;
}

// --- LÓGICA PID ---

// [cite: 1162]
void pid_1(void) {
    if (V_SALIDA == REF_ERR)
    {
        percent_err = 0;
        pidStat1.err_sign = 1;
        return;
    }
    if (V_SALIDA > REF_ERR)
    {
        percent_err = V_SALIDA - REF_ERR;
        pidStat1.err_sign = 0;  // error negativo
    }
    else
    {
        percent_err = REF_ERR - V_SALIDA;
        pidStat1.err_sign = 1;  // error positivo
    }

    // Saturación a 100
    if (percent_err > 100)
    {
        percent_err = 100;
    }
}

void pid_2(void) {
    // Escalado del error: 0..100 -> 0..10000
    error = (uint16_t)U * (uint16_t)percent_err;

    // ¿Error nulo?
    if (error == 0)
    {
        pidStat1.err_z = 1;
        return;
    }
    pidStat1.err_z = 0;

    // Cálculo integral y derivativo
    PidInterrupt();
}

//  Completada
void pid_3(void) {
    Proportional(); // [cite: 1203]
    Integral();     // [cite: 1204]
    Derivative();   // [cite: 1205]
}

// [cite: 1209] Implementación con GetPidResult
void pid_4(void) {
    int32_t ref;
    int32_t pid_out;
    int32_t result;

    // Calcula P + I + D y define el signo
    GetPidResult();

    // Armar valores de 16/24 bits
    ref = ((int32_t)REF0 << 8) | REF1;
    pid_out = ((int32_t)pidOut1 << 8) | pidOut2;

    // Suma o resta según signo del PID
    if (pidStat1 & (1 << pid_sign))
    {
        result = ref + pid_out;
    }
    else
    {
        result = ref - pid_out;
    }

    // Guardar resultado
    TEMP0 = (uint8_t)(result >> 8);
    TEMP1 = (uint8_t)(result & 0xFF);
}

void PidInitialize(void) {
    // Limpieza de errores
    error0 = 0;
    error1 = 0;
    a_Error0 = 0;
    a_Error1 = 0;
    a_Error2 = 0;
    p_Error0 = 0;
    p_Error1 = 0;
    d_Error0 = 0;
    d_Error1 = 0;

    // Limpieza de términos PID
    prop0 = 0;
    prop1 = 0;
    prop2 = 0;
    integ0 = 0;
    integ1 = 0;
    integ2 = 0;
    deriv0 = 0;
    deriv1 = 0;
    deriv2 = 0;

    // Ganancias
    kp = 0;
    ki = 0;
    kd = 0;

    // Salida del PID
    pidOut0 = 0;
    pidOut1 = 0;
    pidOut2 = 0;

    // Variables auxiliares de multiplicación/acumulación
    AARGB0 = 0;
    AARGB1 = 0;
    AARGB2 = 0;
    BARGB0 = 0;
    BARGB1 = 0;
    BARGB2 = 0;

    // Inicialización del contador derivativo
    derivCount = derivCountVal;  // derivCountVal = 0.10 (según comentario)

    // Inicialización de banderas
    pidStat1.err_z = 0;  // error distinto de cero
    pidStat1.a_err_z = 1;  // error acumulado = 0
    pidStat2.d_err_z = 1;  // error derivativo = 0
    pidStat1.p_err_sign = 1;  // error previo positivo
    pidStat1.a_err_sign = 1;  // error acumulado positivo
}

void Proportional(void) {
    // Preparar operandos para la multiplicación
    BARGB0 = 0;
    BARGB1 = kp;
    AARGB0 = error0;
    AARGB1 = error1;

    FXM1616U();  // Multiplicación 16x16

    // Guardar resultado proporcional
    prop0 = AARGB1;
    prop1 = AARGB2;
    prop2 = AARGB3;
}

void Integral(void) {
    // ¿Error acumulado = 0?
    if (pidStat1.a_err_z)
        goto integral_zero;

    // Preparar multiplicación
    BARGB0 = 0;
    BARGB1 = ki;
    AARGB0 = a_Error0;
    AARGB1 = a_Error1;
    AARGB2 = a_Error2;

    FXM2416U();  // Ki × a_Error

    // Copiar resultado
    integ0 = AARGB2;
    integ1 = AARGB3;
    integ2 = AARGB4;
    return;

    // Código legado
    AARGB0 = integ0;
    AARGB1 = integ1;
    AARGB2 = integ2;
    AARGB3 = 0;
    AARGB4 = 0;
    BARGB0 = 0;
    BARGB1 = 10;
    FXD2416U();
    integ0 = AARGB0;
    integ1 = AARGB1;
    integ2 = AARGB2;
    return;
integral_zero:
    integ0 = 0;
    integ1 = 0;
    integ2 = 0;
}

void Derivative(void) {
    // ¿Delta de error = 0?
    if (pidStat2.d_err_z)
        goto derivative_zero;

    // Preparar multiplicación
    BARGB1 = d_Error1;
    BARGB0 = d_Error0;
    AARGB1 = kd;
    AARGB0 = 0;

    FXM1616U();  // Kd × d_Error

    // Guardar resultado
    deriv0 = AARGB1;
    deriv1 = AARGB2;
    deriv2 = AARGB3;
    return;
derivative_zero:
    deriv0 = 0;
    deriv1 = 0;
    deriv2 = 0;
}

// [cite: 1354]
void GetPidResult(void) {
    // Cargar Prop en AARGB
    AARGB0 = prop0;
    AARGB1 = prop1;
    AARGB2 = prop2;

    // Cargar Integ en BARGB
    BARGB0 = integ0;
    BARGB1 = integ1;
    BARGB2 = integ2;

    pidStat2.selecinteg = 0;  // SpecSign trabaja con pid_sign

    SpecSign();  // Suma P + I

    // ¿Signo = 0?
    if (pidStat2.signo == 0)
        goto add_derivative;

    // Determinar magnitud
    if (pidStat1.mag == 0)
        goto integ_mag;
    else
        goto prop_mag;

integ_mag:
    pidStat1.pid_sign = 0;
    if (pidStat1.a_err_sign)
        pidStat1.pid_sign = 1;
    goto add_derivative;
prop_mag:
    pidStat1.pid_sign = 0;
    if (pidStat1.err_sign)
        pidStat1.pid_sign = 1;
add_derivative:
    // Cargar Derivativo
    BARGB0 = deriv0;
    BARGB1 = deriv1;
    BARGB2 = deriv2;

    tempReg = pidStat1 & 0xC0;  // Preparar verificación de signos

    if (tempReg == 0x00)
    {
        _24_BitAdd();
        goto scale_down;
    }
    if (tempReg == 0xC0)
    {
        _24_BitAdd();
        goto scale_down;
    }

    MagAndSub();  // Signos distintos

    if (pidStat1.mag == 0)
        goto deriv_mag;
    goto scale_down;

deriv_mag:
    pidStat1.pid_sign = 0;
    if (pidStat1.d_err_sign)
        pidStat1.pid_sign = 1;
scale_down:
    // División final
    BARGB0 = U_0;
    BARGB1 = U_1;

    FXD2416U();

    // Saturación a 340 (0x0154)
    if ((AARGB2 > 0x54) ||
        (AARGB2 == 0x54 && AARGB1 >= 0x01))
    {
        pidOut2 = 0x54;
        pidOut1 = 0x01;
    }
    else
    {
        pidOut2 = AARGB2;
        pidOut1 = AARGB1;
    }
    pidOut0 = 0;
}

// --- IMPLEMENTACIÓN DE SUBRUTINAS MATEMÁTICAS "FALTANTES" ---
// Estas funciones usan operadores C para emular las rutinas de Assembler que faltan

void FXM1616U(void) {
    // Multiplica AARGB0:1 * BARGB0:1 -> Resultado en AARGB0:3
    uint16_t a = ((uint16_t)AARGB0 << 8) | AARGB1;
    uint16_t b = ((uint16_t)BARGB0 << 8) | BARGB1;
    uint32_t res = (uint32_t)a * b;
    
    AARGB0 = (uint8_t)(res >> 24);
    AARGB1 = (uint8_t)(res >> 16);
    AARGB2 = (uint8_t)(res >> 8);
    AARGB3 = (uint8_t)(res & 0xFF);
}

void FXM2416U(void) {
    // Multiplica AARGB0:2 * BARGB0:1
    uint32_t a = ((uint32_t)AARGB0 << 16) | ((uint16_t)AARGB1 << 8) | AARGB2;
    uint16_t b = ((uint16_t)BARGB0 << 8) | BARGB1;
    uint64_t res = (uint64_t)a * b; // Requiere 64 bits o truncado

    // Mapeo resultado a registros (Simplificado)
    AARGB1 = (uint8_t)(res >> 24);
    AARGB2 = (uint8_t)(res >> 16);
    AARGB3 = (uint8_t)(res >> 8);
    AARGB4 = (uint8_t)(res & 0xFF);
}

void FXD2416U(void) {
    // División AARGB / BARGB
    uint32_t a = ((uint32_t)AARGB0 << 16) | ((uint16_t)AARGB1 << 8) | AARGB2;
    uint16_t b = ((uint16_t)BARGB0 << 8) | BARGB1;
    
    if (b == 0) b = 1; // Evitar div por 0
    uint32_t res = a / b;

    AARGB0 = (uint8_t)(res >> 16);
    AARGB1 = (uint8_t)(res >> 8);
    AARGB2 = (uint8_t)(res & 0xFF);
}

void _24_BitAdd(void) {
    // Suma AARGB + BARGB -> AARGB
    uint32_t a = ((uint32_t)AARGB0 << 16) | ((uint16_t)AARGB1 << 8) | AARGB2;
    uint32_t b = ((uint32_t)BARGB0 << 16) | ((uint16_t)BARGB1 << 8) | BARGB2;
    uint32_t res = a + b;
    
    AARGB0 = (uint8_t)(res >> 16);
    AARGB1 = (uint8_t)(res >> 8);
    AARGB2 = (uint8_t)(res & 0xFF);
}

void MagAndSub(void) {
    // Comparación de magnitudes (24 bits)
    if (AARGB[0] > BARGB[0] ||
       (AARGB[0] == BARGB[0] && AARGB[1] > BARGB[1]) ||
       (AARGB[0] == BARGB[0] && AARGB[1] == BARGB[1] && AARGB[2] >= BARGB[2]))
    {
        // AARGB >= BARGB
        _24_bit_sub();  // AARGB = AARGB - BARGB
        pidStat1_mag = 1;  // AARGB mayor
    }
    else
    {
        // BARGB > AARGB -> swap
        uint8_t temp;

        temp = AARGB[0]; AARGB[0] = BARGB[0]; BARGB[0] = temp;
        temp = AARGB[1]; AARGB[1] = BARGB[1]; BARGB[1] = temp;
        temp = AARGB[2]; AARGB[2] = BARGB[2]; BARGB[2] = temp;

        _24_bit_sub();  // AARGB = BARGB - AARGB
        pidStat1_mag = 0;  // BARGB mayor
    }
}

void SpecSign(void) {
    uint8_t signBits;

    // Set signo flag
    pidStat2.signo = 1;

    // Leer bits 3 y 2 (error y a_error)
    signBits = pidStat1 & 0x0C;
    if (signBits == 0x00)  // ambos negativos
    {
        _24_BitAdd();  // sumar
        if (!pidStat2.selecinteg)
            pidStat1.pid_sign = 0;
        else
            pidStat1.a_err_sign = 0;
    }
    else if (signBits == 0x0C)  // ambos positivos
    {
        _24_BitAdd();  // sumar
        if (!pidStat2.selecinteg)
            pidStat1.pid_sign = 1;
        else
            pidStat1.a_err_sign = 1;
    }
    else  // signos distintos
    {
        pidStat2.signo = 0;
        MagAndSub();  // restar
    }
}

void PidInterrupt(void) {
        // Si el error es cero, no se calcula nada
    if (pidStat1_err_z)
        return;

    // Actualiza el término integral (a_Error)
    GetA_Error();

    // ¿Es momento de calcular la derivada?
    if (--derivCount == 0)
    {
        DeltaError();  // d_Error = error - p_error
        derivCount = derivCountVal;  // recarga contador
    }
}
