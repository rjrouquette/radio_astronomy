//
// Created by robert on 6/4/20.
//

#ifndef FREQUENCY_RESPONSE_MODULE_H
#define FREQUENCY_RESPONSE_MODULE_H

#include "pipe.h"

class module {
protected:
    pipe out;

public:
    module() = default;
    virtual ~module() = default;

    const pipe& getOutput();
    virtual void doStep() = 0;
    virtual void reset();
};

#endif //FREQUENCY_RESPONSE_MODULE_H
