//
// Created by robert on 5/27/20.
//

#ifndef RGB_MATRIX_AVR_NOP_H
#define RGB_MATRIX_AVR_NOP_H

inline void nop() {
    asm("nop");
}

inline void nop2() {
    nop();
    nop();
}

inline void nop4() {
    nop2();
    nop2();
}

#endif //RGB_MATRIX_AVR_NOP_H
