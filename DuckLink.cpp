#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <chrono>
#include <ctime>
#include "Packet.h"
#include "BloomFilter.h"
#include "Utils.h"
#include "DuckLink.h"

void run() {

    //if incoming packet from redis socket
        //set device ID
        //set duck type
        //create packet
        //sned packet
    //else
        //do nothing

}

/*int main () {
    Packet dp;

    DuckLink dl;

    BloomFilter filter = BloomFilter(DEFAULT_NUM_SECTORS, DEFAULT_NUM_HASH_FUNCS, DEFAULT_BITS_PER_SECTOR, DEFAULT_MAX_MESSAGES);

    dp.prepareForSending(&filter, dduid, topic, dp.getType(), 0x00, data);
    return 0;
}*/