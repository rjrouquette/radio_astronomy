//
// Created by robert on 6/4/20.
//

#include "pipe.h"

pipe::pipe() {
    value = 0;
}

double pipe::getValue() const {
    return value;
}

void pipe::setValue(const double _value) {
    value = _value;
}
