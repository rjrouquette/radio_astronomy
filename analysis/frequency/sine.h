//
// Created by robert on 6/4/20.
//

#ifndef FREQUENCY_RESPONSE_SINE_H
#define FREQUENCY_RESPONSE_SINE_H


#include <cstdint>
#include "pipe.h"
#include "module.h"

class sine : public module {
private:
    const double stepRads, offsetRads;
    uint64_t step;

public:
    sine(double stepRads, double offsetRads);
    ~sine() override = default;

    void doStep() override ;
    void reset() override ;
};


#endif //FREQUENCY_RESPONSE_SINE_H
