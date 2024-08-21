#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cstdint>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <chrono>
#include <ctime>
#include <unistd.h>
#include "Packet.h"
#include "BloomFilter.h"
#include "Utils.h"
#include "DuckLink.h"
//#include "MamaDuck.h"
#include "PapaDuck.h"
//#include "DetectorDuck.h"
#include "redis.h"

using namespace std;

vector<std::string> extractValues(const std::string input) {
    std::vector<std::string> values;

    
    std::stringstream ss(input);
    std::string token;

    
    std::string duid, topic, data, ducktype;
    bool inDataField = false;

    while (std::getline(ss, token, ' ')) {
        size_t colonPos = token.find(':');
        if (colonPos != std::string::npos) {
            std::string fieldName = token.substr(0, colonPos);
            std::string fieldValue = token.substr(colonPos + 1);

            if (fieldName == "DUID") {
                duid = fieldValue;
            } else if (fieldName == "TOPIC") {
                topic = fieldValue;
            } else if (fieldName == "DATA") {
                // Start collecting DATA field
                data = fieldValue;
                inDataField = true;
            } else if (fieldName == "DUCKTYPE") {
                // End of DATA field, collect DUCKTYPE field
                ducktype = fieldValue;
                inDataField = false;
            }
        } else if (inDataField) {
            // Append to DATA field if we are still in the DATA field
            data += ' ' + token;
        }
    }

    values.push_back(duid);
    values.push_back(topic);
    values.push_back(data);
    values.push_back(ducktype);

    return values;
}


string modifystring (string cdp, int position){
    if(cdp[position] > 127){//this goes into extended ascii range
        cdp[position] = cdp[position] ^ 0x80;
    }
    else if (cdp[position] < 32){//non printable characters
        cdp[position] = cdp[position] ^ 0x10;
    }
    else {
        //dont modify
    }
    
    
    return cdp;
}
string unmodifystring (string cdp, int position){
    if((cdp[position] ^ 0x80) >127){//this goes into extended ascii range
        cdp[position] = cdp[position] ^ 0x80;
    }
    else if ((cdp[position] ^ 0x10) < 32){//non printable characters
        cdp[position] = cdp[position] ^ 0x10;
    }
    else {
        //dont modify
    }
    
    
    return cdp;
}

RedisConfig initializeRedisConfig() {
    RedisConfig config;
    config.stream_name = "mystream";
    config.group_name = "TX";
    config.consumer_name = "CDP";
    config.filter_key = "WEB_CDP";
    config.key = "WEB_CDP";
    config.lora_queue = "LORA";
    config.txWebQueue = "TXWeb";
    config.txLoraQueue = "TXLora";
    config.response ;
    config.task;
    config.messageID;
    config.key_buffer;
    config.messageBuffer;
    return config;
}

RedisConfig initializeRedisRxLoraConfig() {
    RedisConfig config;
    config.stream_name = "mystream";
    config.group_name = "RX";
    config.consumer_name = "CDP";
    config.filter_key = "LORA_CDP";
    config.key = "LORA_CDP";
    config.lora_queue;
    config.txWebQueue;
    config.txLoraQueue;
    config.response ;
    config.task;
    config.messageID;
    config.key_buffer;
    config.messageBuffer;
    return config;
};

int main()
{

    
   

//////connect redis server
redisContext* redisConnect = redis_init("localhost", 6379);

RedisConfig redisConfig = initializeRedisConfig();
redisReply* reply = (redisReply*)redisCommand(redisConnect, "DEL %s", redisConfig.txWebQueue.c_str());
//redisReply* reply = (redisReply*)redisCommand(redisConnect, "DEL %s", redisConfig.txLoraQueue.c_str());
//redisReply* reply = (redisReply*)delete_stream(redisConnect, redisConfig.stream_name);

string value = "DUID:MAMA0001 TOPIC:status DATA:Test Data String DUCKTYPE:LINK ";
string value2 = "DUID:MAMA0001 TOPIC:status DATA:Test Data String Again DUCKTYPE:LINK ";

//string msg;
    // Use a single buffer to receive the response
//char response[256];  // Adjust the size according to your needs
//string response;
create_consumer_group(redisConnect, redisConfig.stream_name, redisConfig.group_name);
//enqueue_task(redisConnect, redisConfig.txWebQueue, value);
publish(redisConnect, redisConfig.stream_name, redisConfig.key, value, redisConfig.response);
//enqueue_task(redisConnect, redisConfig.txWebQueue, value2);
publish(redisConnect, redisConfig.stream_name, redisConfig.key, value2, redisConfig.response);

/*---------conditional checks for while loops*/
    int messageReceived = 1;
    int messageReceivedLora = 0;

    /*------Duck objects----------*/
    DuckLink dl;
    //DetectorDuck dd;
    //MamaDuck md;
    PapaDuck pd;

    /*---------for WEB_CDP--------------*/
    std::vector<std::string> extractedValues;
    vector<uint8_t> dduid;
    uint8_t topic;
    vector<uint8_t> data;
    string DUCKTYPE;

    /*---------for CDP_WEB-------------*/
    vector<uint8_t> receivedData;
    vector<uint8_t> sduid;
    vector<uint8_t> receivedMsg;
    string receivedSduid;
    string receivedTopic;
    string receivedMessage;
    string messageForWeb;

    /*----------for CDP_Lora-----------*/
    vector<uint8_t> payload;
    string cdppayload;


    while (true) {
        
        print_queue(redisConnect, redisConfig.txWebQueue);
        read_from_consumer_group(redisConnect, redisConfig.stream_name, redisConfig.group_name, redisConfig.consumer_name, redisConfig.filter_key, redisConfig.key_buffer, redisConfig.messageBuffer, redisConfig.messageID, redisConfig.txWebQueue, redisConfig.task);
        messageReceived = acknowledge_message(redisConnect, redisConfig.stream_name, redisConfig.group_name, redisConfig.messageID);
        sleep(1);

        if (messageReceived) {

            messageReceived = 0;
            dequeue_task(redisConnect, redisConfig.txWebQueue, redisConfig.task);
            extractedValues = extractValues(redisConfig.task);
            redisConfig.task.clear();
            
            cout << "DUID: " <<extractedValues[0] << endl;
            cout << "TOPIC: " << extractedValues[1] << endl;
            cout << "DATA: " << extractedValues[2] << endl;
            cout << "DUCKTYPE: " << extractedValues[3] << endl;

            dduid = duckutils::convertStringToVector(extractedValues[0]);
            topic = Packet::stringToTopic(extractedValues[1]);
            data = duckutils::convertStringToVector(extractedValues[2]);
            DUCKTYPE = extractedValues[3];

            // Instantiate Packet Object
            Packet dp;
        
            

            if (DUCKTYPE == "MAMA") {
                /*md.setDuckId(duckutils::convertStringToVector("MAMA0001"));
                md.handleReceivedPacket(dp);*/
                
            }
            else if (DUCKTYPE == "PAPA") {
                pd.setDuckId(duckutils::convertStringToVector("PAPA0001"));
                RedisConfig redisConfigRxLora = initializeRedisRxLoraConfig();
                /*-------Read from lora begin--------------*/

                while(!messageReceivedLora){
                    sleep(5);
                    read_from_consumer_group(redisConnect, redisConfigRxLora.stream_name, redisConfigRxLora.group_name, redisConfigRxLora.consumer_name, redisConfigRxLora.filter_key, redisConfigRxLora.key_buffer,redisConfigRxLora.messageBuffer, redisConfigRxLora.messageID, redisConfigRxLora.txLoraQueue, redisConfigRxLora.task);
                    messageReceivedLora = acknowledge_message(redisConnect, redisConfigRxLora.stream_name, redisConfigRxLora.group_name, redisConfigRxLora.messageID);
                    
                }
                dequeue_task(redisConnect, redisConfigRxLora.txWebQueue, redisConfigRxLora.task);
                dp.setBuffer(duckutils::convertStringToVector(redisConfigRxLora.task));
                messageReceivedLora = 1;
                
                /*-------Read from lora end--------------*/


                pd.handleReceivedPacket(dp);
                

                /*-------Send to web server begin-----------*/
                
                receivedData = dp.getBuffer();
                receivedMsg.assign(receivedData.begin()+DATA_POS, receivedData.end());
                sduid.assign(receivedData.begin(), receivedData.begin() + 8);
                receivedSduid = duckutils::convertVectorToString(sduid);
                receivedTopic = Packet::topicToString(receivedData.at(TOPIC_POS));
                receivedMessage = duckutils::convertVectorToString(receivedMsg);
                messageForWeb = "SDUID:" + receivedSduid + " TOPIC:"  + receivedTopic + " DATA:" + receivedMessage +" ";
                publish(redisConnect, redisConfig.stream_name, "CDP_WEB", messageForWeb, redisConfig.response);

                redisConfigRxLora.messageBuffer.clear();
                /*-------Send to web server end-----------*/

                
                
            }
            else if (DUCKTYPE == "LINK") {
                dl.setDuckId(duckutils::convertStringToVector("DUCK0001"));
                BloomFilter filter = BloomFilter(DEFAULT_NUM_SECTORS, DEFAULT_NUM_HASH_FUNCS, DEFAULT_BITS_PER_SECTOR, DEFAULT_MAX_MESSAGES);
                dl.prepareForSending(&filter, dduid, topic, dl.getType(), 0x00, data);
                
                /*---------Send to Lora---------*/
                payload = dl.getBuffer();
                cdppayload = duckutils::convertVectorToString(payload);
                publish(redisConnect, redisConfig.stream_name, "CDP_LORA", cdppayload, redisConfig.response);
                /*---------Send to Lora---------*/
                
                
            }
            else if (DUCKTYPE == "DETECTOR") {
                /*dd.setDuckId(duckutils::convertStringToVector("DETECTOR"));
                dd.sendPing(1);
                dd.handleReceivedPacket();*/
                // Add code for setup to send and receive pings
            }
            else {
                cout << "Error setting duck type. Recheck spelling and ensure all letters are capitalized." << endl;
                
            }
        }
        else {
            cout << "No message received. Waiting for next poll..." << endl;
        }

        // Sleep for a while before checking again
        sleep(1); // Sleep for 1 second (or adjust as needed)
    }
    redisFree(redisConnect);

    return 0;
}