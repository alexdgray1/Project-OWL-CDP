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
#include "PapaDuck.h"
#include "BloomFilter.h"
#include "Utils.h"
#include "redis.h"

#define DUCK_ERR_NONE 0

using std::cout;
using std::endl;

redisContext* redisConnect = redis_init("localhost", 6379);
string response;

string modifystring (string cdp, int position){
    if(cdp[position] > 127){//this goes into extended ascii range
        cdp[position] = cdp[position] ^ 0x80;
    }
    else if (cdp[position] < 32){//non printable characters
        cdp[position] = cdp[position] ^ 0x20;
    }
    else {
        //dont modify
    }
    
    
    return cdp;
}
//||
string unmodifystring (string cdp, int position){
    
    

    if ((cdp[position] ^ 0x20) < 32  ){//non printable characters
        cdp[position] = cdp[position] ^ 0x20;
    }
    else if((cdp[position] ^ 0x80) >127){//this goes into extended ascii range
        cdp[position] = cdp[position] ^ 0x80;
    }
    else {
        //dont modify
    }
    
    
    return cdp;
}



void PapaDuck::handleReceivedPacket(Packet &dp) {


  int err= 0;
  cout << "handleReceivedPacket() START" << endl;
  //declare string to receive packet

  

  
  dp.decodePacket(dp.getBuffer());
  //duckutils::printVector(dataRX);
  


  if (err != DUCK_ERR_NONE) {
    cout << "ERROR handleReceivedPacket. Failed to get data. rc = " << err << endl;
    return;
  }
  // ignore pings
  if (dp.getBuffer().at(TOPIC_POS) == reservedTopic::ping) {
    dp.reset();
    return;
  }
  


  // build our RX DuckPacket which holds the updated path in case the packet is relayed
  bool relay = dp.checkRelayPacket(&filter, dp.getBuffer());
  if (relay) {
    cout << "relaying: " <<  duckutils::convertToHex(dp.getBuffer().data(), dp.getBuffer().size()).c_str() << endl;
    
    
    
    enableAcks(1);
    if (acksEnabled) {
      
      if (needsAck(dp)) {
        handleAck(dp);
      }
    
  }

    

    cout << "handleReceivedPacket() DONE" << endl;
  }
}

void PapaDuck::handleAck(Packet& dp) {
  
  cout << "Starting new ack broadcast" << endl;
  storeForAck(dp);

  if (ackBufferIsFull()) {
    cout << "Ack buffer is full. Sending broadcast ack immediately." << endl;
    broadcastAck();
  }
}

void PapaDuck::enableAcks(bool enable) {
  acksEnabled = enable;
}

bool PapaDuck::ackHandler(PapaDuck * duck)
{
  duck->broadcastAck();
  return false;
}

void PapaDuck::storeForAck(Packet & packet) {
  ackStore.push_back(std::pair<Duid, Muid>(packet.sduid, packet.muid));
}

bool PapaDuck::ackBufferIsFull() {
  return (ackStore.size() >= MAX_MUID_PER_ACK);
}

bool PapaDuck::needsAck(Packet & dp) {
  if (dp.getBuffer().at(TOPIC_POS) == reservedTopic::ack) {
    return false;
  } else {
    return true;
  }
}

void PapaDuck::broadcastAck() {
  assert(ackStore.size() <= MAX_MUID_PER_ACK);

  const uint8_t num = static_cast<uint8_t>(ackStore.size());

  std::vector<uint8_t> dataPayload;
  dataPayload.push_back(num);
  for (int i = 0; i < num; i++) {
    Duid duid = ackStore[i].first;
    Muid muid = ackStore[i].second;
    cout << "Storing ack for duid: " << duckutils::convertVectorToString(duid) <<  "muid: " << duckutils::convertVectorToString(muid) << endl;
      
      
    dataPayload.insert(dataPayload.end(), duid.begin(), duid.end());
    dataPayload.insert(dataPayload.end(), muid.begin(), muid.end());
  }
  Packet txAck;
  int err = txAck.prepareForSending(&filter, BROADCAST_DUID, DuckType::PAPA,
    reservedTopic::ack, 0, dataPayload);
  
  /*---send to lora-----*/
  string ackPayload = duckutils::convertVectorToString(txAck.getBuffer());
  for (int i =0; i < 7; i++) {
    ackPayload = modifystring(ackPayload, TOPIC_POS+ i);
  }
  publish(redisConnect, "mystream", "CDP_LORA",ackPayload , response);
  /*---send to lora-----*/

  if (err == DUCK_ERR_NONE) {
    
    filter.bloom_add(txAck.muid.data(), MUID_LENGTH);
  } else {
    cout << "ERROR handleReceivedPacket. Failed to send ack. Error: " << err << endl;
  }
  
  ackStore.clear();
}

void PapaDuck::sendCommand(uint8_t cmd, std::vector<uint8_t> value) {
  cout << "Initiate sending command" << endl;
  std::vector<uint8_t> dataPayload;
  dataPayload.push_back(cmd);
  dataPayload.insert(dataPayload.end(), value.begin(), value.end());

  Packet txCommand;
  int err = txCommand.prepareForSending(&filter, BROADCAST_DUID, DuckType::PAPA,
    reservedTopic::cmd, 0, dataPayload);
  if (err != DUCK_ERR_NONE) {
    
    cout << "ERROR handleReceivedPacket. Failed to prepare ack. Error: " << err << endl;
  }

  /*---send to lora-----*/
  string commandPayload = duckutils::convertVectorToString(txCommand.getBuffer());
  for (int i =0; i < 7; i++) {
    commandPayload = modifystring(commandPayload, TOPIC_POS+i);
  }
  publish(redisConnect, "mystream", "CDP_LORA",commandPayload , response);
  /*---send to lora-----*/

  if (err == DUCK_ERR_NONE) {

    filter.bloom_add(txCommand.muid.data(), MUID_LENGTH);
  } else {
    cout << "ERROR handleReceivedPacket. Failed to send ack. Error: " << err << endl;
  }
  
}

void PapaDuck::sendCommand(uint8_t cmd, std::vector<uint8_t> value, std::vector<uint8_t> dduid) {
  //loginfo_ln("Initiate sending command");
  std::vector<uint8_t> dataPayload;
  dataPayload.push_back(cmd);
  dataPayload.insert(dataPayload.end(), value.begin(), value.end());

  Packet txCommand;
  int err = txCommand.prepareForSending(&filter, dduid, DuckType::PAPA,
    reservedTopic::cmd, 0, dataPayload);
  if (err != DUCK_ERR_NONE) {
    cout << "ERROR handleReceivedPacket. Failed to prepare cmd. Error: " << endl;
  }

  /*---send to lora-----*/
  string commandPayload = duckutils::convertVectorToString(txCommand.getBuffer());
  for (int i =0; i < 7; i++) {
    commandPayload = modifystring(commandPayload, TOPIC_POS+i);
  }
  publish(redisConnect, "mystream", "CDP_LORA",commandPayload , response);
  /*---send to lora-----*/

  if (err == DUCK_ERR_NONE) {
    //Packet packet = Packet(txPacket->getBuffer());
    
    filter.bloom_add(txCommand.muid.data(), MUID_LENGTH);
  } else {
    cout << "ERROR handleReceivedPacket. Failed to send cmd. Error: " << err << endl;
  }
}