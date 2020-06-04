//
// Created by robert on 6/4/20.
//

#include <cmath>
#include "sine.h"

sine::sine(double _stepRads, double _offsetRads) :
stepRads(_stepRads), offsetRads(_offsetRads)
{
    step = 0;
}

void sine::doStep() {
    out.setValue(std::sin((step * stepRads) + offsetRads));
    ++step;
}

void sine::reset() {
    step = 0;
}
