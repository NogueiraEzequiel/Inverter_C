#ifndef VSCODE_FIX_H
#define VSCODE_FIX_H

// Este bloque SOLO lo lee el editor VS Code (IntelliSense).
// El compilador real XC8 lo ignora por completo, así que no afecta al PIC.
#ifdef __INTELLISENSE__
    
    // 1. Definimos el modelo exacto para que cargue los registros (TRISA, PORTB, etc)
    #define _18F2520
    #define __18F2520
    
    // 2. Incluimos la librería general
    #include <xc.h>

    // 3. Truco para que no marque error en la función de interrupción
    #if !defined(__interrupt)
        #define __interrupt()
    #endif

#endif

#endif // VSCODE_FIX_H