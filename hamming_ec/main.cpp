#include <iostream>
#include <cmath>
#include <iomanip>
#include <fstream>
#include <cstdlib>
#include <random>
#include <cstring>
#include "HammingEC.hpp"

using namespace std;

uint8_t orig[56][1008];
uint8_t corr[56][1008];

int main(int argc, char **argv) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned> dis(0, 255);

    HammingEC_56::initLuts();

    auto temp = orig[0];
    for(size_t i = 0; i < sizeof(orig); i++) {
        temp[i] = dis(gen);
    }

    void *blocks[HammingEC_56::getBlockCount()];
    for(int i = 0; i < HammingEC_56::getBlockCount(); i++) {
        blocks[i] = orig[i];
    }

    cout << "parity bits: " << HammingEC_56::getParityCount() << endl;

    HammingEC_56::parity(sizeof(orig[0]), blocks);
    cout << "computed parity blocks" << endl;
    for(auto &b : orig) {
        cout << " - ";
        unsigned byte = b[0];
        for(unsigned i = 0; i < 8; i++) {
            if(((byte >> i) & 1u) == 0)
                cout << "0";
            else
                cout << "1";
        }
        cout << endl;
    }

    memcpy(corr, orig, sizeof(corr));
    for(int i = 0; i < HammingEC_56::getBlockCount(); i++) {
        blocks[i] = corr[i];
    }

    bool present[HammingEC_56::getBlockCount()];
    memset(present, 1, sizeof(present));

    int missing[] = { 13,14 };
    for(auto i : missing) {
        present[i] = false;
        bzero(corr[i], sizeof(corr[i]));
    }

    if(!HammingEC_56::repair(sizeof(corr[0]), blocks, present)) {
        cout << "failed to repair missing blocks" << endl;
    }

    cout << "repaired missing blocks" << endl;/*
    for(auto &b : corr) {
        cout << " - ";
        unsigned byte = b[0];
        for(unsigned i = 0; i < 8; i++) {
            if(((byte >> i) & 1u) == 0)
                cout << "0";
            else
                cout << "1";
        }
        cout << endl;
    }*/

    if(memcmp(corr, orig, sizeof(corr)) != 0) {
        cout << "error repairing data" << endl;
    }

    return 0;
}
