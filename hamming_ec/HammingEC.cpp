//
// Created by robert on 5/8/20.
//

#include <cstdlib>
#include <iostream>
#include "HammingEC.hpp"

HammingEC::HammingEC(unsigned _blockCount, unsigned _blockSize) noexcept :
    blockCount(_blockCount), blockSize(_blockSize / sizeof(slice_t))
{
    if(blockCount < 4) abort();
    if(_blockSize % sizeof(slice_t) != 0) abort();

    parityCount = 0;
    while((1u << parityCount) < blockCount)
        parityCount++;
}

void HammingEC::clearBlock(slice_t *block) const {
    for(unsigned i = 0; i < blockSize; i++) {
        block[i] = 0;
    }
}

void HammingEC::xorBlock(slice_t *a, const slice_t *b) const {
    for(unsigned i = 0; i < blockSize; i++) {
        a[i] ^= b[i];
    }
}

void HammingEC::parity(void **blocks) const {
    // sub-parity blocks
    for(unsigned p = 0; p < parityCount; p++) {
        const auto mask = 1u << p;
        auto pblock = (slice_t*) blocks[mask];
        clearBlock(pblock);
        for (unsigned b = mask + 1; b < blockCount; b++) {
            if((b & mask) != 0u) {
                xorBlock(pblock, (slice_t*) blocks[b]);
            }
        }
    }

    // parity block
    auto pblock = (slice_t*) blocks[0];
    clearBlock(pblock);
    for (unsigned b = 1; b < blockCount; b++) {
        xorBlock(pblock, (slice_t*) blocks[b]);
    }
}

bool HammingEC::repair(void **blocks, bool *present) const {
    unsigned moff[3];

    unsigned missing = 0;
    for(unsigned i = 0; i < blockCount; i++) {
        if(!present[i]) {
            if(missing >= 3)
                return false;
            moff[missing++] = i;
        }
    }

    // no-missing blocks
    if(missing == 0)
        return true;

    // three missing blocks
    if(missing >= 3) {
        return false;
    }

    // two missing blocks
    if(missing >= 2) {
        auto diff = moff[0] ^ moff[1];
        unsigned mask = 0;
        for(unsigned p = 0; p < parityCount; p++) {
            if((diff >> p) & 1u) {
                mask = 1u << p;
                break;
            }
        }

        // flip repair order if necessary
        if((moff[1] & mask) == 0) {
            auto temp = moff[0];
            moff[0] = moff[1];
            moff[1] = temp;
        }

        // clear missing block
        auto mblock = (slice_t*) blocks[moff[1]];
        clearBlock(mblock);

        // reconstruct block using parity mask
        for (unsigned b = 1; b < blockCount; b++) {
            if(present[b]) {
                if ((b & mask) != 0u) {
                    xorBlock(mblock, (slice_t *) blocks[b]);
                }
            }
        }

        // block is no-longer missing
        present[moff[1]] = true;
    }

    // fix missing block using overall parity
    auto mblock = (slice_t*) blocks[moff[0]];
    clearBlock(mblock);

    // reconstruct block using parity
    for (unsigned b = 0; b < blockCount; b++) {
        if (present[b]) {
            xorBlock(mblock, (slice_t*) blocks[b]);
        }
    }

    // block is no-longer missing
    present[moff[0]] = true;

    return true;
}
