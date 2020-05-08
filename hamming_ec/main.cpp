#include <iostream>
#include <cmath>
#include <iomanip>
#include <fstream>
#include <cstdlib>
#include <random>
#include "HammingEC.hpp"

using namespace std;

HammingEC hec(56, 1008);

uint8_t data[56][1008];

int main(int argc, char **argv) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned> dis(0, 255);

    auto temp = data[0];
    for(size_t i = 0; i < sizeof(data); i++) {
        temp[i] = dis(gen);
    }

    void *blocks[56];
    for(int i = 0; i < 56; i++) {
        blocks[i] = data[i];
    }

    hec.parity(blocks);
    cout << "computed parity blocks" << endl;

    return 0;
}
