//
// Created by robert on 5/27/20.
//

#ifndef RGB_MATRIX_AVR_GPSDO_H
#define RGB_MATRIX_AVR_GPSDO_H

void initGPSDO();

uint8_t isPllLocked();
int32_t getPllError();
int32_t getPllErrorVar();


#endif //RGB_MATRIX_AVR_GPSDO_H
