//
// Created by robert on 5/8/20.
//

#ifndef HAMMING_EC_HAMMINGEC_H
#define HAMMING_EC_HAMMINGEC_H

#include <cstddef>

class HammingEC {
public:
    const unsigned blockCount;
    const unsigned blockSize;

    HammingEC(unsigned blockCount, unsigned blockSize) noexcept ;
    virtual ~HammingEC() = default ;

    void parity(void **blocks) const ;
    void repair(void **blocks, bool *present) const ;
};


#endif //HAMMING_EC_HAMMINGEC_H
