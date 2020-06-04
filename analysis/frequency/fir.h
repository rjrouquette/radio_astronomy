//
// Created by robert on 6/4/20.
//

#ifndef FREQUENCY_RESPONSE_FIR_H
#define FREQUENCY_RESPONSE_FIR_H


#include "module.h"

// delay-tap FIR filter
class fir : public module {
private:
    const pipe &in;
    const unsigned len;
    unsigned step;
    const double *weights;
    double *taps;

public:
    fir(const pipe &in, const double *weights, unsigned length);
    ~fir() override ;

    void doStep() override ;
    void reset() override ;
};


#endif //FREQUENCY_RESPONSE_FIR_H
