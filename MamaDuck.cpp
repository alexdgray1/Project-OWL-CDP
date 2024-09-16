#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <chrono>
#include <ctime>
#include <string>
#include "redis.h"
#include "Packet.h"
#include "MamaDuck.h"
#include "BloomFilter.h"
#include "Utils.h"

#define DUCK_ERR_NONE 0

using std::cout;
using std::string;
using std::vector;
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


void MamaDuck::handleReceivedPacket(Packet& dp) {
  int err = 0;
  
  bool relay = false;

  Packet txAck;
  string ackPayload;
  string cmdPayload;
  string payload;


  relay = dp.checkRelayPacket(&filter, dp.getBuffer());
  if (relay) {
    
    cout << "handleReceivedPacket: packet RELAY START" << endl;

    // NOTE:
    // Ducks will only handle received message one at a time, so there is a chance the
    // packet being sent below will never be received, especially if the cluster is small
    // there are not many alternative paths to reach other mama ducks that could relay the packet.
    
    

    //Check if Duck is desitination for this packet before relaying
    if (BROADCAST_DUID == dp.dduid) {
      cout << "Packet was meant for all Papa ducks " << endl;
      switch(dp.topic) {
        case reservedTopic::ping:
          
          cout << "Ping Received" << endl;
          err = sendPong();
          if (err != DUCK_ERR_NONE) {
            
            cout << "Error: Failed to send pong " << err << endl;
          }
          return;
        break;
        case reservedTopic::ack:{

          handleAck(dp);
          //relay batch ack 

          /*---send to lora-----*/
          ackPayload = duckutils::convertVectorToString(dp.getBuffer());
          for (int i =0; i < 7; i++) {
            ackPayload = modifystring(ackPayload, i);
          }
          publish(redisConnect, "mystream", "CDP_LORA",ackPayload , response);
          

          if (err != DUCK_ERR_NONE) {
            cout << "====> ERROR handleReceivedPacket failed to relay. rc = " << endl;
          } else {
            cout << "handleReceivedPacket: packet RELAY DONE" << endl;
          }
        }
        break;
        case reservedTopic::cmd:
          //loginfo_ln("Command received");
          handleCommand(dp);

          /*---send to lora-----*/
          cmdPayload = duckutils::convertVectorToString(dp.getBuffer());
          for (int i =0; i < 7; i++) {
            cmdPayload = modifystring(cmdPayload, TOPIC_POS+ i);
          }
          publish(redisConnect, "mystream", "CDP_LORA",cmdPayload , response);
          /*---send to lora done-----*/

          if (err != DUCK_ERR_NONE) {
            cout << "====> ERROR handleReceivedPacket failed to relay. rc = " << endl;
          } else {
            cout << "handleReceivedPacket: packet RELAY DONE" << endl;
          }
        break;
        default:

          

          if (err != DUCK_ERR_NONE) {
            cout << "====> ERROR handleReceivedPacket failed to relay. rc = " << endl;
          } else {
            cout << "handleReceivedPacket: packet RELAY DONE" << endl;
          }
          payload = duckutils::convertVectorToString(dp.getBuffer());
                for (int i =0; i < 7; i++) {
                    payload = modifystring(payload, TOPIC_POS+i);
                }
                publish(redisConnect, "mystream", "CDP_LORA",payload , response);
      }
    } else if(sduid == dp.dduid) { //Target device check
        std::vector<uint8_t> dataPayload;
        uint8_t num = 1;
        //string ackPayload;
        

      cout << "Packet was meant to this duck " << endl;
      switch(dp.topic) {
        case topics::dcmd:
          cout<<"Duck command received" << endl;
          handleDuckCommand(dp);
        break;
        case reservedTopic::cmd:
          cout << "Command received" << endl;
          
          //Start send ack that command was received

          
          dataPayload.push_back(num);

          dataPayload.insert(dataPayload.end(), dp.sduid.begin(), dp.sduid.end());
          dataPayload.insert(dataPayload.end(), dp.muid.begin(), dp.muid.end());

          err = txAck.prepareForSending(&filter, PAPADUCK_DUID, reservedTopic::ack,DuckType::MAMA, 0, dataPayload);
          if (err != DUCK_ERR_NONE) {
          cout << "ERROR handleReceivedPacket. Failed to prepare ack. Error: " << err << endl;
          }

          /*---send to lora-----*/
          ackPayload = duckutils::convertVectorToString(txAck.getBuffer());
          for (int i =0; i < 7; i++) {
            ackPayload = modifystring(ackPayload, TOPIC_POS+i);
          }
          publish(redisConnect, "mystream", "CDP_LORA",ackPayload , response);
          /*---send to lora-----*/
          if (err == DUCK_ERR_NONE) {
            filter.bloom_add(txAck.muid.data(), MUID_LENGTH);
          } else {
            cout << "ERROR handleReceivedPacket. Failed to send ack. Error: " << endl;
          }
          
          //Handle Command
          handleCommand(dp);

          break;
        case reservedTopic::ack:{
          handleAck(dp);
        }
        break;
        default:
          
          if (err != DUCK_ERR_NONE) {
            cout << "====> ERROR handleReceivedPacket failed to relay. rc = " << endl;
          } else {
            cout << "handleReceivedPacket: packet RELAY DONE" << endl;
          }
          
          payload = duckutils::convertVectorToString(dp.getBuffer());
                for (int i =0; i < 7; i++) {
                    payload = modifystring(payload, TOPIC_POS+i);
                }
                publish(redisConnect, "mystream", "CDP_LORA",payload , response);
      }

    } else {
      
      if (err != DUCK_ERR_NONE) {
        cout << "====> ERROR handleReceivedPacket failed to relay. rc = " << endl;
      } else {
        cout << "handleReceivedPacket: packet RELAY DONE" << endl;
      }
      payload = duckutils::convertVectorToString(dp.getBuffer());
                for (int i =0; i < 7; i++) {
                    payload = modifystring(payload, TOPIC_POS+i);
                }
                publish(redisConnect, "mystream", "CDP_LORA",payload , response);
    }

  }
}

void MamaDuck::handleCommand(Packet & dp) {
  int err;
  std::vector<uint8_t> dataPayload;
  std::vector<uint8_t> alive = {'I','m',' ','a','l','i','v','e'};
  Packet healthPacket;
  string healthPayload;

  switch(dp.getBuffer().at(DATA_POS)) {
    case 0:
      //Send health quack

      cout << "Health request received" << endl;
      
      dataPayload.insert(dataPayload.end(), alive.begin(), alive.end());
      err = healthPacket.prepareForSending(&filter, PAPADUCK_DUID, 
         topics::health, DuckType::MAMA, 0, dataPayload);
      if (err != DUCK_ERR_NONE) {
      cout << "ERROR handleReceivedPacket. Failed to prepare ack. Error: "<< endl;
      }

      /*---send to lora-----*/
      healthPayload = duckutils::convertVectorToString(healthPacket.getBuffer());
          for (int i =0; i < 7; i++) {
            healthPayload = modifystring(healthPayload, TOPIC_POS+i);
          }
      publish(redisConnect, "mystream", "CDP_LORA",healthPayload , response);
      /*---send to lora-----*/
      if (err == DUCK_ERR_NONE) {
        
        filter.bloom_add(healthPacket.muid.data(), MUID_LENGTH);
      } else {
        cout << "ERROR handleReceivedPacket. Failed to send ack. Error: " << endl;
      }

    break;
    case 1:
    /*  //Change wifi status
        if((char)rxDP.data[1] == '1') {
          cout << "Command WiFi ON" << endl;
          

        } else if ((char)rxDP.data[1] == '0') {
          cout << "Command WiFi OFF" << endl;
          
        }
    */
    break;
    default:
      cout << "Command not recognized" << endl;
  }

}

void MamaDuck::handleAck(Packet & dp) {
  
  if (lastMessageMuid.size() == MUID_LENGTH) {
    const uint8_t numPairs = dp.data.at(0);
    static const int NUM_PAIRS_LENGTH = 1;
    static const int PAIR_LENGTH = DUID_LENGTH + MUID_LENGTH;
    for (int i = 0; i < numPairs; i++) {
      int pairOffset = NUM_PAIRS_LENGTH + i*PAIR_LENGTH;
      std::vector<uint8_t>::const_iterator duidOffset = dp.data.begin() + pairOffset;
      std::vector<uint8_t>::const_iterator muidOffset = dp.data.begin() + pairOffset + DUID_LENGTH;
      if (std::equal(dduid.begin(), dduid.end(), duidOffset)
        && std::equal(lastMessageMuid.begin(), lastMessageMuid.end(), muidOffset)
      ) {
        cout << "handleReceivedPacket: matched ack-MUID "<< duckutils::convertVectorToString(lastMessageMuid).c_str();
        lastMessageAck = true;
        break;
      }
    }
    
  }
}

void MamaDuck::handleDuckCommand(Packet & dp) {
  //loginfo_ln("Doesn't do anything yet. But Duck Command was received.");
  cout << "Doesnt do anything yet" << endl;
}

