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

class DetectorDuck : public Packet {
    public:
    void handleReceivedPacket();
    int getType() { return DuckType::DETECTOR; }
    void sendPing(bool startReceive);

    private:
    
    //void handleReceivedPacket();

};