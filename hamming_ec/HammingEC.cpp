//
// Created by robert on 5/8/20.
//

#include <cstdint>
#include <cstdlib>
#include "HammingEC.hpp"

typedef uint64_t slice_t;

HammingEC::HammingEC(unsigned _blockCount, unsigned _blockSize) noexcept :
    blockCount(_blockCount), blockSize(_blockSize / sizeof(slice_t))
{
    if(_blockSize % sizeof(slice_t) != 0) abort();

}

void HammingEC::parity(void **blocks) const {
    unsigned mask = 1;
    while(mask <= blockCount) {
        auto pblock = (slice_t*) blocks[mask - 1];
        for(unsigned i = 0; i < blockSize; i++) {
            pblock[i] = 0;
        }
        for (unsigned b = mask + 1; b <= blockCount; b++) {
            if((b & mask) == 0u) {
                auto block = (slice_t*) blocks[b - 1];
                for(unsigned i = 0; i < blockSize; i++) {
                    pblock[i] ^= block[i];
                }
            }
        }
        mask <<= 1u;
    }
}
