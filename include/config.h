/**
 * @file config.h
 * @brief Bits de configuración del PIC18F2520.
 * [cite_start]Copia fiel del documento de referencia [cite: 18-61].
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <xc.h>

// --- ESCUDO PARA VS CODE ---
// VS Code no entiende los #pragma de Microchip y marca error.
// Con este #ifndef, le decimos que ignore estas líneas en el editor.
// El compilador (XC8) SÍ las leerá y configurará el PIC correctamente.
#ifndef __INTELLISENSE__

    // CONFIG1H
    [cite_start]#pragma config OSC = HS      // Oscilador externo de alta velocidad [cite: 19]
    [cite_start]#pragma config FCMEN = OFF   // Fail-Safe Clock Monitor deshabilitado [cite: 20]
    [cite_start]#pragma config IESO = OFF    // Oscilador interno/externo switchover deshabilitado [cite: 21]

    // CONFIG2L
    [cite_start]#pragma config PWRT = ON     // Power-up Timer habilitado [cite: 26]
    [cite_start]#pragma config BOREN = OFF   // Brown-out Reset deshabilitado [cite: 26]
    [cite_start]#pragma config BORV = 2      // Voltaje BOR (aunque esté OFF) [cite: 26]

    // CONFIG2H
    [cite_start]#pragma config WDT = OFF     // Watchdog Timer apagado [cite: 29]
    [cite_start]#pragma config WDTPS = 2048  // Postscaler WDT [cite: 29]

    // CONFIG3H
    [cite_start]#pragma config CCP2MX = PORTC // CCP2 en RC1 [cite: 31]
    [cite_start]#pragma config PBADEN = OFF   // PORTB<4:0> digitales al inicio [cite: 31]
    [cite_start]#pragma config LPT1OSC = OFF  // Timer1 oscilador baja potencia OFF [cite: 32]
    [cite_start]#pragma config MCLRE = ON     // MCLR pin habilitado [cite: 32]

    // CONFIG4L
    [cite_start]#pragma config STVREN = OFF   // Stack Overflow Reset OFF [cite: 35]
    [cite_start]#pragma config LVP = OFF      // Low Voltage Programming OFF [cite: 36]
    [cite_start]#pragma config XINST = OFF    // Extended Instruction Set OFF [cite: 37]
    [cite_start]#pragma config DEBUG = ON     // Debug habilitado [cite: 38]

    [cite_start]// CONFIG5L..CONFIG7H [cite: 40-61]
    #pragma config CP0 = OFF
    #pragma config CP1 = OFF
    #pragma config CPB = OFF
    #pragma config CPD = OFF
    #pragma config WRT0 = OFF
    #pragma config WRT1 = OFF
    #pragma config WRTB = OFF
    #pragma config WRTC = OFF
    #pragma config WRTD = OFF
    #pragma config EBTR0 = OFF
    #pragma config EBTR1 = OFF
    #pragma config EBTRB = OFF

#endif // Fin del escudo __INTELLISENSE__

// Incluimos el parche de definiciones para el resto del código
#include "vscode_fix.h"

#endif // CONFIG_H