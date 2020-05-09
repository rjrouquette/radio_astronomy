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

void HammingEC::repairBlock(void *_block, unsigned int mask, void **blocks, const bool *present) const {
    auto block = (slice_t *) _block;
    clearBlock(block);

    if (mask == -1u) {
        // reconstruct block using overall parity
        for (unsigned b = 0; b < blockCount; b++) {
            if (present[b]) {
                xorBlock(block, (slice_t *) blocks[b]);
            }
        }
    } else {
        // reconstruct block using parity mask
        for (unsigned b = 1; b < blockCount; b++) {
            if (present[b]) {
                if ((b & mask) != 0u) {
                    xorBlock(block, (slice_t *) blocks[b]);
                }
            }
        }
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

    // one missing block
    if(missing == 1) {
        repairBlock(blocks[moff[0]], -1u, blocks, present);
        present[moff[0]] = true;
        return true;
    }

    // two missing blocks
    if(missing == 2) {
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

        // fix blocks
        repairBlock(blocks[moff[1]], mask, blocks, present);
        present[moff[1]] = true;
        repairBlock(blocks[moff[0]], -1u, blocks, present);
        present[moff[0]] = true;
        return true;
    }

    // three missing blocks
    if(missing == 3) {
        // special case if overall parity is missing
        if(moff[0] == 0) {
            auto diff = moff[1] ^ moff[2];
            unsigned mask = 0;
            for(unsigned p = 0; p < parityCount; p++) {
                if((diff >> p) & 1u) {
                    mask = 1u << p;
                    break;
                }
            }

            // flip repair order if necessary
            if((moff[2] & mask) == 0) {
                auto temp = moff[1];
                moff[1] = moff[2];
                moff[2] = temp;
            }

            repairBlock(blocks[moff[2]], mask, blocks, present);
            present[moff[2]] = true;

            mask = 0;
            for(unsigned p = 0; p < parityCount; p++) {
                if((moff[1] >> p) & 1u) {
                    mask = 1u << p;
                    break;
                }
            }

            repairBlock(blocks[moff[1]], mask, blocks, present);
            present[moff[1]] = true;

            repairBlock(blocks[moff[0]], -1u, blocks, present);
            present[moff[0]] = true;
            return true;
        }

    }

    return false;
}
