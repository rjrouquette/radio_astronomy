//
// Created by robert on 6/4/20.
//

#include "fir.h"

fir::fir(const pipe &_in, const double *_weights, unsigned length) :
in(_in), len(length)
{
    step = 0;
    weights = _weights;
    taps = new double[len];
    for(unsigned i = 0; i < len; i++) {
        taps[i] = 0;
    }
}

fir::~fir() {
    delete[] taps;
}

void fir::doStep() {
    if(step == 0)
        step = len - 1;
    else
        --step;

    taps[step] = in.getValue();
    double acc = 0;
    for(unsigned i = 0; i < len; i++) {
        acc += weights[i] * taps[(i + step) % len];
    }
    out.setValue(acc);
}

void fir::reset() {
    step = 0;
    for(unsigned i = 0; i < len; i++) {
        taps[i] = 0;
    }
}
