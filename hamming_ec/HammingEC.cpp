//
// Created by robert on 5/8/20.
//

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include "HammingEC.hpp"

typedef uint64_t slice_t;

HammingEC::HammingEC(unsigned _blockCount, unsigned _blockSize) noexcept :
    blockCount(_blockCount), blockSize(_blockSize / sizeof(slice_t))
{
    if(blockCount < 3) abort();
    if(_blockSize % sizeof(slice_t) != 0) abort();

    parityCount = 0;
    while((1u << parityCount) <= blockCount)
        parityCount++;
}

void HammingEC::parity(void **blocks) const {
    for(unsigned p = 0; p < parityCount; p++) {
        const unsigned mask = 1u << p;

        auto pblock = (slice_t*) blocks[mask - 1];
        for(unsigned i = 0; i < blockSize; i++) {
            pblock[i] = 0;
        }
        for (unsigned b = mask + 1; b <= blockCount; b++) {
            if((b & mask) != 0u) {
                auto block = (slice_t*) blocks[b - 1];
                for(unsigned i = 0; i < blockSize; i++) {
                    pblock[i] ^= block[i];
                }
            }
        }
    }
}

bool HammingEC::repair(void **blocks, const bool *present) const {
    unsigned moff[parityCount];

    unsigned missing = 0;
    for(unsigned i = 0; i < blockCount; i++) {
        if(!present[i]) {
            if(missing >= parityCount)
                return false;

            moff[missing++] = i + 1;
        }
    }

    // no-missing blocks
    if(missing == 0)
        return true;

    // single missing block is trivial
    if(missing == 1) {
        // clear missing block
        auto mblock = (slice_t*) blocks[moff[0] - 1];
        for(unsigned i = 0; i < blockSize; i++) {
            mblock[i] = 0;
        }

        // compute parity mask
        unsigned mask = 0;
        for(unsigned p = 0; p < parityCount; p++) {
            mask = 1u << p;
            if((moff[0] & mask) != 0u)
                break;
        }

        // reconstruct block using parity
        for (unsigned b = 1; b <= blockCount; b++) {
            if(b == moff[0])
                continue;

            if((b & mask) != 0u) {
                auto block = (slice_t*) blocks[b - 1];
                for(unsigned i = 0; i < blockSize; i++) {
                    mblock[i] ^= block[i];
                }
            }
        }
    }

    return false;
}
