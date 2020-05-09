//
// Created by robert on 5/8/20.
//

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "HammingEC.hpp"

#define BLOCK_COUNT (56)
#define PARITY_BITS (6)
#define MAX_MISSING (2)

unsigned lutSingleMask[BLOCK_COUNT];
unsigned lutDoubleMask[BLOCK_COUNT - 1][BLOCK_COUNT][4];

namespace HammingEC_56 {
    typedef uint64_t slice_t;

    static unsigned getMask(unsigned bits);
    static void clearBlock(unsigned blockLen, slice_t *block);
    static void xorBlock(unsigned blockLen, slice_t *a, const slice_t *b);
    static void repairBlock(unsigned blockLen, unsigned blockId, unsigned mask, slice_t **blocks, bool *present);

}

unsigned HammingEC_56::getBlockCount() { return BLOCK_COUNT; }
unsigned HammingEC_56::getParityCount() { return PARITY_BITS; }
unsigned HammingEC_56::getDataCount() { return BLOCK_COUNT - PARITY_BITS; }


void HammingEC_56::initLuts() {
    // generate correction mask for single missing blocks
    for(unsigned p = 0; p < PARITY_BITS; p++) {
        auto mask = 1u << p;
        for(auto b = mask; b < (mask << 1u) && b <= BLOCK_COUNT; b++) {
            lutSingleMask[b - 1] = mask;
        }
    }

    // generate correction order and masks for double missing blocks
    for(unsigned i = 1; i < BLOCK_COUNT; i++) {
        for(unsigned j = 0; j < i; j++) {
            auto &cell = lutDoubleMask[i-1][j];

            auto a = i + 1;
            auto b = j + 1;
            auto diff = a ^ b;
            auto da = a & diff;
            auto db = b & diff;

            if(da != 0) {
                cell[0] = a;
                cell[1] = getMask(da);
                cell[2] = b;
                cell[3] = lutSingleMask[b - 1];
            } else {
                cell[0] = b;
                cell[1] = getMask(db);
                cell[2] = a;
                cell[3] = lutSingleMask[a - 1];
            }
        }
    }
}

unsigned int HammingEC_56::getMask(unsigned int bits) {
    for(unsigned p = 0; p < PARITY_BITS; p++) {
        auto mask = 1u << p;
        if(bits & mask)
            return mask;
    }
    return 0;
}

void HammingEC_56::clearBlock(unsigned blockLen, slice_t *block) {
    for(unsigned i = 0; i < blockLen; i++) {
        block[i] = 0;
    }
}

void HammingEC_56::xorBlock(unsigned blockLen, slice_t *a, const slice_t *b) {
    for(unsigned i = 0; i < blockLen; i++) {
        a[i] ^= b[i];
    }
}

void HammingEC_56::repairBlock(unsigned blockLen, const unsigned blockId, unsigned int mask, slice_t **blocks, bool *present) {
    auto block = blocks[blockId - 1];
    clearBlock(blockLen, block);

    // reconstruct block using parity mask
    for (unsigned b = mask; b <= BLOCK_COUNT; b++) {
        if (present[b - 1] && ((b & mask) != 0u)) {
            xorBlock(blockLen, block, (slice_t *) blocks[b - 1]);
        }
    }

    // mark block as repaired
    present[blockId - 1] = true;
}

void HammingEC_56::parity(const unsigned blockSize, void **blocks) {
    if((blockSize % sizeof(slice_t)) != 0) abort();
    const auto blockLen = blockSize / sizeof(slice_t);

    bool present[BLOCK_COUNT];
    memset(present, 1, sizeof(present));

    for(unsigned p = 0; p < PARITY_BITS; p++) {
        const auto mask = 1u << p;
        present[mask - 1] = false;
        repairBlock(blockLen, mask, mask, (slice_t**)blocks, present);
    }
}

bool HammingEC_56::repair(const unsigned blockSize, void **blocks, bool *present) {
    if((blockSize % sizeof(slice_t)) != 0) abort();
    const auto blockLen = blockSize / sizeof(slice_t);

    unsigned moff[MAX_MISSING];

    unsigned missing = 0;
    for(unsigned i = 0; i < BLOCK_COUNT; i++) {
        if(!present[i]) {
            // if too many blocks are missing, do not attempt repair
            if(missing >= MAX_MISSING)
                return false;

            // record position of missing block
            moff[missing++] = i;
        }
    }

    // no-missing blocks
    if(missing == 0)
        return true;

    // one missing block
    if(missing == 1) {
        auto mask = lutSingleMask[moff[0]];
        repairBlock(blockLen, moff[0] + 1, mask, (slice_t**) blocks, present);
        return true;
    }

    // two missing blocks
    if(missing == 2) {
        auto &cell = lutDoubleMask[moff[1]-1][moff[0]];
        repairBlock(blockLen, cell[0], cell[1], (slice_t**) blocks, present);
        repairBlock(blockLen, cell[2], cell[3], (slice_t**) blocks, present);
        return true;
    }

    return false;
}
