//
// Created by robert on 5/27/20.
//

#ifndef RGB_MATRIX_AVR_GPSDO_H
#define RGB_MATRIX_AVR_GPSDO_H

void initGPSDO();

uint8_t isPllLocked();
float getPllError();
float getPllErrorRms();
float getPllFeedback();
float getPllTemperature();


#endif //RGB_MATRIX_AVR_GPSDO_H
