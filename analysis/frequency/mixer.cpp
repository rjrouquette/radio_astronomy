//
// Created by robert on 6/4/20.
//

#include "mixer.h"

mixer::mixer(const pipe &_inA, const pipe &_inB) :
    inA(_inA), inB(_inB)
{

}

void mixer::doStep() {
    out.setValue(inA.getValue() * inB.getValue());
}
