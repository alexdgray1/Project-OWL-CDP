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

class MamaDuck : public Packet {
  public:
    ~MamaDuck() {}
    //rxDoneCallback = void (*)(std::vector<byte> data );
  /**
   * @brief Register callback for handling data received from duck devices
   * 
   * The callback will be invoked if the packet needs to be relayed (i.e not seen before)
   * @param cb a callback to handle data received by the papa duck
   */
  //void onReceiveDuckData(rxDoneCallback cb) { this->recvDataCallback = cb; }

    int getType() {return DuckType::MAMA;}
    void handleReceivedPacket();
    void handleReceivedPacket(Packet &packet);

  private :
  //rxDoneCallback recvDataCallback;
    
    void run();
    void handleCommand(Packet & dp);
    void handleDuckCommand(Packet & dp);

  /**
   * @brief Handles if there were any acks addressed to this duck.
   *
   * @param packet The a broadcast ack, which has topic type reservedTopic::ack
   */
    void handleAck(Packet & dp);

  protected: 
    bool lastMessageAck = true;
    BloomFilter filter;
};

