//
// Created by robert on 5/8/20.
//

#ifndef HAMMING_EC_HAMMINGEC_H
#define HAMMING_EC_HAMMINGEC_H

#include <cstddef>
#include <cstdint>

class HammingEC {
private:
    typedef uint64_t slice_t;

    unsigned parityCount;

    void clearBlock(slice_t *block) const ;
    void xorBlock(slice_t *a, const slice_t *b) const ;

public:
    const unsigned blockCount;
    const unsigned blockSize;

    HammingEC(unsigned blockCount, unsigned blockSize) noexcept ;
    virtual ~HammingEC() = default ;

    unsigned getParityCount() const { return parityCount; }

    void parity(void **blocks) const ;
    bool repair(void **blocks, bool *present) const ;
};


#endif //HAMMING_EC_HAMMINGEC_H
