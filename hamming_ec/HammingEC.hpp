//
// Created by robert on 5/8/20.
//

#ifndef HAMMING_EC_HAMMINGEC_H
#define HAMMING_EC_HAMMINGEC_H

#include <cstddef>

class HammingEC {
private:
    unsigned parityCount;

public:
    const unsigned blockCount;
    const unsigned blockSize;

    HammingEC(unsigned blockCount, unsigned blockSize) noexcept ;
    virtual ~HammingEC() = default ;

    unsigned getParityCount() const { return parityCount; }

    void parity(void **blocks) const ;
    bool repair(void **blocks, const bool *present) const ;
};


#endif //HAMMING_EC_HAMMINGEC_H
