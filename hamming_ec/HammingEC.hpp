//
// Created by robert on 5/8/20.
//

#ifndef HAMMING_EC_HAMMINGEC_H
#define HAMMING_EC_HAMMINGEC_H

#include <cstddef>

namespace HammingEC_56 {
    unsigned getBlockCount();
    unsigned getParityCount();
    unsigned getDataCount();

    void initLuts();
    void parity(unsigned blockSize, void **blocks);
    bool repair(unsigned blockSize, void **blocks, bool *present);
}

#endif //HAMMING_EC_HAMMINGEC_H
