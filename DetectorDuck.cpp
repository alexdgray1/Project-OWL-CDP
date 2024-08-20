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
#include "DetectorDuck.h"

#define DUCK_ERR_NONE 0

void DetectorDuck::handleReceivedPacket() {

  //loginfo("handleReceivedPacket()...");

  std::vector<uint8_t> data;
  int err;
  //int err = duckRadio.readReceivedData(&data);

  if (err != DUCK_ERR_NONE) {
    //logerr("ERROR Failed to get data from DuckRadio. rc = %s\n",err);
    return;
  }

  if (data[TOPIC_POS] == reservedTopic::pong) {
    //logdbg("run() - got ping response!");
    //rssiCb(duckRadio.getRSSI());
  }
}

void DetectorDuck::sendPing(bool startReceive) {
  int err = DUCK_ERR_NONE;
  std::vector<uint8_t> data(1, 0);
  err = txPacket->prepareForSending(&filter, BROADCAST_DUID, DuckType::DETECTOR, reservedTopic::ping, 0, data);

  if (err == DUCK_ERR_NONE) {
    //err = duckRadio.sendData(txPacket->getBuffer());
    if (startReceive) {
      //duckRadio.startReceive();
    }
    if (err != DUCK_ERR_NONE) {
      //logerr("ERROR Failed to ping, err = %s\n",err);
    }
  } else {
    //logerr("ERROR Failed to build packet, err = %s\n",err);
  }
}

void run () {
  //start in transmit mode
  //setup packet info for ping
  //send ping
  //if receive
    //start in receive mode
    //factory reset?
    //send ping
    //run handleReceivePacket
    //go back to transmit
  //else do nothing 
    
}