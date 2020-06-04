//
// Created by robert on 6/4/20.
//

#ifndef FREQUENCY_RESPONSE_MIXER_H
#define FREQUENCY_RESPONSE_MIXER_H


#include "pipe.h"
#include "module.h"

class mixer : public module {
private:
    const pipe &inA, &inB;

public:
    mixer(const pipe &inA, const pipe &inB);
    ~mixer() override = default;

    void doStep() override ;
};


#endif //FREQUENCY_RESPONSE_MIXER_H
