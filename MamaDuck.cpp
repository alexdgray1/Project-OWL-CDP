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
#include "Packet.h"
#include "MamaDuck.h"
#include "BloomFilter.h"
#include "Utils.h"

#define DUCK_ERR_NONE 0

using std::cout;
using std::string;
using std::vector;
using std::endl;

string modifystring (string cdp, int position){
    if(cdp[position] > 9){
        cdp[position] = cdp[position] + 55;
    }
    else{
        cdp[position] = cdp[position] + 48;
    }
    //cdp[position] = 
    return cdp;
}
string unmodifyString (string cdp, int position){
    /*if(cdp[position] > 54){
        cdp[position] = cdp[position] - 55;
    }
    else{
        cdp[position] = cdp[position] - 48;
    }*/
    cdp[position] = cdp[position] - 32;
    return cdp;
}
/*string unmodifyString (string cdp, int position){
    char aByte;
    if(cdp[position] > 57 && cdp[position+1] > 57 ){
        aByte = (((cdp[position] -55) << 4) &0xF0) | ((cdp[position+1]-55) & 0x0F);
        //aByte = (cdp[position] - 55)  << 4;
    }
    else if (cdp[position] > 57 && cdp[position+1] <57)
    {
        aByte = (((cdp[position] -55) << 4) &0xF0) | ((cdp[position+1]-48) & 0x0F);
    }
    else if (cdp[position] < 57 && cdp[position+1] >57)
    {
        aByte = (((cdp[position] -48) << 4) &0xF0) | ((cdp[position+1]-55) & 0x0F);
    }
    else{
        aByte = (((cdp[position]-48) << 4) & 0xF0) | ((cdp[position+1]-48) &0x0F);
        //aByte = (cdp[position] -48) << 4;
    }
    cout << aByte << endl;
    cdp[position]= aByte;
    cdp.erase(position+1, 1);
    return cdp;

}*/

void MamaDuck::run () {
  //set in continous receive mode

  //declare mamaduck and its duck ID
  MamaDuck mama;
  mama.setDuckId(duckutils::convertStringToVector("MAMA0003"));
  
  
  //while(1)
  //if received data
    //handle received packet 
    //in handling packet start tx mode
    //once sent go back to continous receive
  //else
    //do nothing
}


void MamaDuck::handleReceivedPacket(Packet& packet) {
  int err = 0;
  //vector<uint8_t> packetRecieved;
  /* ----------------receive from Lora---------------*/
/*char response[256];  // Adjust the size according to your needs
const char* mystream = "mystream";
const char* key = "CDP_LORA";
const char* message = "hello world";
const char* group_name = "LORA";
read_from_consumer_group(c, mystream, group_name, )*/

/* ----------------receive from Lora---------------*/
    
  //Packet packet = Packet(dataRx);
  //dataRx = packet.getBuffer();
  packet.decodePacket(packet.getBuffer());

  //dataRx = duckutils::convertStringToVector(dataReceived);
  bool relay = false;
  
  std::cout << "Handle Received Packet: Begin" << endl;

    /*------get received data from radio*/

  
  std::cout << "Got data from radio, prepare for relay. size: " << packet.getBuffer().size() << endl;

  relay = packet.checkRelayPacket(&filter, packet.getBuffer());
  if (relay) {
    //TODO: this callback is causing an issue, needs to be fixed for mamaduck to get packet data
    //recvDataCallback(rxPacket->getBuffer());

    cout << "handleReceivedPacket: packet RELAY START" << endl;

    // NOTE:
    // Ducks will only handle received message one at a time, so there is a chance the
    // packet being sent below will never be received, especially if the cluster is small
    // there are not many alternative paths to reach other mama ducks that could relay the packet.
    
    //Packet rxDP = Packet(packet.getBuffer());

    //Check if Duck is desitination for this packet before relaying
    if (BROADCAST_DUID == packet.dduid) {
      cout << "Packet was meant for all Papa ducks " << endl;
      switch(packet.topic) {
        case reservedTopic::ping:
          //loginfo_ln("ping received");
          cout << "Ping Received" << endl;
          err = sendPong();
          if (err != DUCK_ERR_NONE) {
            //logerr_ln("ERROR failed to send pong message. rc = %d",err);
            cout << "Error: Failed to send pong " << err << endl;
          }
          return;
        break;
        case reservedTopic::ack:{
          handleAck(packet);
          //relay batch ack 
          //err = duckRadio.relayPacket(rxPacket);
          if (err != DUCK_ERR_NONE) {
            //logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
          } else {
            //loginfo_ln("handleReceivedPacket: packet RELAY DONE");
          }
        }
        break;
        case reservedTopic::cmd:
          //loginfo_ln("Command received");
          handleCommand(packet);

          //err = duckRadio.relayPacket(rxPacket);
          if (err != DUCK_ERR_NONE) {
            //logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
          } else {
            //loginfo_ln("handleReceivedPacket: packet RELAY DONE");
          }
        break;
        default:
          //err = duckRadio.relayPacket(rxPacket);
          if (err != DUCK_ERR_NONE) {
            //logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
          } else {
            //loginfo_ln("handleReceivedPacket: packet RELAY DONE");
          }
      }
    } else if(sduid == packet.dduid) { //Target device check
        std::vector<uint8_t> dataPayload;
        uint8_t num = 1;
      cout << "Packet was meant to this duck " << endl;
      switch(packet.topic) {
        case topics::dcmd:
          cout<<"Duck command received" << endl;
          handleDuckCommand(packet);
        break;
        case reservedTopic::cmd:
          cout << "Command received" << endl;
          
          //Start send ack that command was received
          dataPayload.push_back(num);

          dataPayload.insert(dataPayload.end(), packet.sduid.begin(), packet.sduid.end());
          dataPayload.insert(dataPayload.end(), packet.muid.begin(), packet.muid.end());

          err = packet.prepareForSending(&filter, PAPADUCK_DUID, reservedTopic::ack,DuckType::MAMA, 0, dataPayload);
          if (err != DUCK_ERR_NONE) {
          cout << "ERROR handleReceivedPacket. Failed to prepare ack. Error: " << err << endl;
          }

          //err = duckRadio.sendData(txPacket->getBuffer());
          if (err == DUCK_ERR_NONE) {
            filter.bloom_add(packet.muid.data(), MUID_LENGTH);
          } else {
            cout << "ERROR handleReceivedPacket. Failed to send ack. Error: " << endl;
          }
          
          //Handle Command
          handleCommand(packet);

          break;
        case reservedTopic::ack:{
          handleAck(packet);
        }
        break;
        default:
          //err = duckRadio.relayPacket(rxPacket);
          if (err != DUCK_ERR_NONE) {
            cout << "====> ERROR handleReceivedPacket failed to relay. rc = " << endl;
          } else {
            cout << "handleReceivedPacket: packet RELAY DONE" << endl;
          }
      }

    } else {
      //err = duckRadio.relayPacket(rxPacket);
      if (err != DUCK_ERR_NONE) {
        cout << "====> ERROR handleReceivedPacket failed to relay. rc = " << endl;
      } else {
        cout << "handleReceivedPacket: packet RELAY DONE" << endl;
      }
    }

  }
}

void MamaDuck::handleCommand(Packet & dp) {
  int err;
  std::vector<uint8_t> dataPayload;
  std::vector<uint8_t> alive = {'I','m',' ','a','l','i','v','e'};

  switch(dp.getBuffer().at(DATA_POS)) {
    case 0:
      //Send health quack
      //loginfo_ln("Health request received");
      cout << "Health request received" << endl;
      dataPayload.insert(dataPayload.end(), alive.begin(), alive.end());
      err = dp.prepareForSending(&filter, PAPADUCK_DUID, 
         topics::health, DuckType::MAMA, 0, dataPayload);
      if (err != DUCK_ERR_NONE) {
      cout << "ERROR handleReceivedPacket. Failed to prepare ack. Error: "<< endl;
      }

      //err = duckRadio.sendData(txPacket->getBuffer());
      if (err == DUCK_ERR_NONE) {
        //Packet healthPacket = Packet(txPacket->getBuffer());
        //Packet healthPacket;

        filter.bloom_add(dp.muid.data(), MUID_LENGTH);
      } else {
        cout << "ERROR handleReceivedPacket. Failed to send ack. Error: " << endl;
      }

    break;
    case 1:
    /*  //Change wifi status
      #ifdef CDPCFG_WIFI_NONE
        logwarn_ln("WiFi not supported");
      #else
        if((char)rxDP.data[1] == '1') {
          loginfo_ln("Command WiFi ON");
          WiFi.mode(WIFI_AP);

        } else if ((char)rxDP.data[1] == '0') {
          loginfo_ln("Command WiFi OFF");
          WiFi.mode( WIFI_MODE_NULL );
        }
      #endif*/
    break;
    default:
      cout << "Command not recognized" << endl;
  }

}

void MamaDuck::handleAck(Packet & dp) {
  
  if (lastMessageMuid.size() == MUID_LENGTH) {
    const uint8_t numPairs = dp.data[0];
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
    

    // TODO[Rory Olsen: 2021-06-23]: The application may need to know about
    //   acks. I recommend a callback specifically for acks, or
    //   similar.
  }
}

void MamaDuck::handleDuckCommand(Packet & dp) {
  //loginfo_ln("Doesn't do anything yet. But Duck Command was received.");
  cout << "Doesnt do anything yet" << endl;
}

/*string unmodifyString (string cdp, int position){
    if(cdp[position] > 54){
        cdp[position] = cdp[position] - 55;
    }
    else{
        cdp[position] = cdp[position] - 48;
    }
    
    return cdp;
}*/

int main()
{

////set device ID
    string devID = "MAMA0003";

    // Instantiate Packet Object
    Packet dp;

    MamaDuck md;

    md.setDuckId(duckutils::convertStringToVector("MAMA0003"));

    //static std::vector<uint8_t> BROADCAST_DUID = {0xFF, 0xFF, 0xFF, 0xFF,
                                              //0xFF, 0xFF, 0xFF, 0xFF};
    //string bduid = duckutils::convertVectorToString(BROADCAST_DUID);
    //cout << bduid << endl;

    string CDP = "DUCK0001��������Y3ZB$! ��V ";
                //DUCK0001MAMA0003Y3ZB08 ��V7
                //DUCK0001MAMA0003K9WQ0! ��VTest Data String
                  

    for(int i = 0; i < 7; i++)
    {
        CDP = unmodifyString(CDP, TOPIC_POS+i);
    }

  std::vector<uint8_t> dataRx(CDP.size());
  //dataRx.size() = sizeof(CDP);
    dataRx = duckutils::convertStringToVector(CDP);
    
    dp.setBuffer(dataRx);
    //packetRecieved = duckutils::convertStringToVector(CDP);
    //dp.setBuffer(packetRecieved);

    //gets payload generated
    //vector<uint8_t> payload = dp.getBuffer();

    //Packet packet = Packet(dp.getBuffer());
    //duckutils::printVector(packet.getBuffer());

    

    md.handleReceivedPacket(dp);

    //dp.reset();


    return 0;
}