#include "types.h"

void kPrintString(int iX, int iY, const char* pcString);

void _start( void ) {
    kPrintString(0, 7, "C Language Kernel Started~!!!");

    while(1) ;
}

void kPrintString(int iX, int iY, const char* pcString) {
    CHARACTER* pstScreen = (CHARACTER*) 0xB8000;
    
    pstScreen += ( iY * 80 ) + iX;
    
    for (int i = 0; pcString[i] != 0; i++) {
        pstScreen[i].bCharactor = pcString[i];
    }
}
