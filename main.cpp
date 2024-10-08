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
#include "MamaDuck.h"
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

    while (std::getline(ss, token, '_')) {
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

            } /*else if (fieldName == "DUCKTYPE") {
                ducktype = fieldValue;
            }*/
            
            else if (fieldName == "DUCKTYPE") {
                // End of DATA field, collect DUCKTYPE field
                ducktype = fieldValue;
                inDataField = false;
            }
        } else if (inDataField) {
            // Append to DATAcout << cdppayload << endl; field if we are still in the DATA field
            data += '_' + token;
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

/*--------------Encodes CDP as string-----------*/
/*
vector<uint8_t> checkDDuid.assign(dp.getBuffer().begin()+8, dp.getBuffer().begin()+16);
string cdppayload;
if (checkDDuid == BROADCAST_DDUID) {
    for(int i = 0; i < 7; i++) {
        cdppayload = modifystring(cdppayload, DUID_POS);
    }

}
*/

/*--------decodes CDP as a string--------------*/
/*
for(int i = 0; i < 7; i++) {
        cdppayload = unmodifystring(cdppayload, DUID_POS);
    }
for(int i = 0; i < 7; i++) {
        cdppayload = unmodifystring(cdppayload, TOPIC_POS);
    }
*/

string encodeCDP(string cdppayload) {
    for (int i = 0; i < cdppayload.size(); i++) {//modifies all unreadable characters within the string to make sure they are readable
        cdppayload = modifystring(cdppayload, SDUID_POS + i);
    }
    return cdppayload;
}

string decodeCDP(string cdppayload) {
    for (int i = 0; i < cdppayload.size(); i++) {//undoes modifying of string to get actual data initially put in
        cdppayload = unmodifystring(cdppayload, SDUID_POS + i);
    }
    return cdppayload;
}

string sendToWeb (vector<uint8_t> receivedData) {

    //vector<uint8_t> receivedData;
    vector<uint8_t> sduid;
    vector<uint8_t> receivedMsg;
    string receivedSduid;
    string receivedTopic;
    string receivedMessage;
    string messageForWeb;

    receivedMsg.assign(receivedData.begin()+DATA_POS, receivedData.end());
    sduid.assign(receivedData.begin(), receivedData.begin() + 8);
    receivedSduid = duckutils::convertVectorToString(sduid);
    receivedTopic = Packet::topicToString(receivedData.at(TOPIC_POS));
    receivedMessage = duckutils::convertVectorToString(receivedMsg);
    messageForWeb = "SDUID:" + receivedSduid + "_TOPIC:"  + receivedTopic + "_DATA:" + receivedMessage +"_";

    return messageForWeb;

}

RedisConfig initializeRedisWebConfig() {
    RedisConfig config;
    config.stream_name = "mystream";
    config.group_name = "TX";
    config.consumer_name = "CDP";
    config.txKey = "WEB_CDP";
    config.rxKey = "CDP_WEB";
    config.txWebQueue = "TXWeb";
    config.rxWebQueue = "RXWeb";
    config.txLoraQueue;
    config.response ;
    config.task;
    config.messageID;
    config.key_buffer;
    config.messageBuffer;
    return config;
}

RedisConfig initializeRedisLoraConfig() {
    RedisConfig config;
    config.stream_name = "mystream";
    config.group_name = "RX";
    config.consumer_name = "CDP";
    config.txKey = "CDP_LORA";
    config.rxKey = "LORA_CDP";
    config.rxLoraQueue = "RXLora";
    //config.txWebQueue;
    config.txLoraQueue = "TXLora";
    config.response ;
    config.task;
    config.messageID;
    config.key_buffer;
    config.messageBuffer;
    return config;
};

int main()
{

    
   
BloomFilter filter = BloomFilter(DEFAULT_NUM_SECTORS, DEFAULT_NUM_HASH_FUNCS, DEFAULT_BITS_PER_SECTOR, DEFAULT_MAX_MESSAGES);
//////connect redis server
redisContext* redisConnect = redis_init("localhost", 6379);

RedisConfig redisConfigWeb = initializeRedisWebConfig();
RedisConfig redisConfigLora = initializeRedisLoraConfig();
redisReply* reply = (redisReply*)redisCommand(redisConnect, "DEL %s", redisConfigWeb.txWebQueue.c_str());
//redisReply* reply = (redisReply*)redisCommand(redisConnect, "DEL %s", redisConfig.txLoraQueue.c_str());
//redisReply* reply = (redisReply*)delete_stream(redisConnect, redisConfig.stream_name);

string value = "DUID:MAMA0001_TOPIC:status_DATA:HELLO_WORLD_DUCKTYPE:LINK";
string value2 = "DUID:MAMA0001_TOPIC:status_DATA:Test Data String Again_DUCKTYPE:LINK ";


create_consumer_group(redisConnect, redisConfigWeb.stream_name, redisConfigWeb.group_name);
//publish(redisConnect, redisConfigWeb.stream_name, redisConfigWeb.txKey, value, redisConfigWeb.response);
//publish(redisConnect, redisConfigWeb.stream_name, redisConfigWeb.txKey, value2, redisConfigWeb.response);

/*---------conditional checks for while loops*/
    int messageReceived = 1;
    int messageReceivedLora = 0;

    /*------Duck objects----------*/
    DuckLink dl;
    //DetectorDuck dd;
    MamaDuck md;
    PapaDuck pd;

    /*---------for WEB_CDP--------------*/
    std::vector<std::string> extractedValues;
    vector<uint8_t> dduid;
    uint8_t topic;
    vector<uint8_t> data;
    string DUCKTYPE;

    /*---------for CDP_WEB-------------*/
    vector<uint8_t> receivedData;
    /*vector<uint8_t> sduid;
    vector<uint8_t> receivedMsg;
    string receivedSduid;
    string receivedTopic;
    string receivedMessage;
    string messageForWeb;*/

    /*----------for CDP_Lora-----------*/
    vector<uint8_t> payload;
    string cdppayload;


    while (true) {
        
        print_queue(redisConnect,redisConfigWeb.txWebQueue);
        read_from_consumer_group(redisConnect, redisConfigWeb.stream_name, redisConfigWeb.group_name, redisConfigWeb.consumer_name, redisConfigWeb.txKey, redisConfigWeb.key_buffer, redisConfigWeb.messageBuffer, redisConfigWeb.messageID, redisConfigWeb.txWebQueue, redisConfigWeb.task);
        messageReceived = acknowledge_message(redisConnect, redisConfigWeb.stream_name, redisConfigWeb.group_name, redisConfigWeb.messageID);
        sleep(1);

        if (messageReceived) {

            messageReceived = 0;
            dequeue_task(redisConnect, redisConfigWeb.txWebQueue, redisConfigWeb.task);
            extractedValues = extractValues(redisConfigWeb.task);
            redisConfigWeb.task.clear();
            
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
                md.setDuckId(duckutils::convertStringToVector("MAMA0001"));

                while(!messageReceivedLora){
                    sleep(5);
                    read_from_consumer_group(redisConnect, redisConfigLora.stream_name, redisConfigLora.group_name, redisConfigLora.consumer_name, redisConfigLora.rxKey, redisConfigLora.key_buffer,redisConfigLora.messageBuffer, redisConfigLora.messageID, redisConfigLora.txLoraQueue, redisConfigLora.task);
                    messageReceivedLora = acknowledge_message(redisConnect, redisConfigLora.stream_name, redisConfigLora.group_name, redisConfigLora.messageID);
                    
                }
                dequeue_task(redisConnect, redisConfigLora.rxLoraQueue, redisConfigLora.task);

                cdppayload = decodeCDP(redisConfigLora.task);

                dp.setBuffer(duckutils::convertStringToVector(cdppayload));
                
                //sends the redis messages in function below
                md.handleReceivedPacket(dp);

            
                        
            }
            else if (DUCKTYPE == "PAPA") {

                pd.setDuckId(duckutils::convertStringToVector("PAPA0001"));

                /*-------Read from lora begin--------------*/
                while(!messageReceivedLora){
                    sleep(5);
                    read_from_consumer_group(redisConnect, redisConfigLora.stream_name, redisConfigLora.group_name, redisConfigLora.consumer_name, redisConfigLora.rxKey, redisConfigLora.key_buffer,redisConfigLora.messageBuffer, redisConfigLora.messageID, redisConfigLora.txLoraQueue, redisConfigLora.task);
                    messageReceivedLora = acknowledge_message(redisConnect, redisConfigLora.stream_name, redisConfigLora.group_name, redisConfigLora.messageID);
                    
                }
                dequeue_task(redisConnect, redisConfigLora.rxLoraQueue, redisConfigLora.task);

                cdppayload = decodeCDP(redisConfigLora.task);


                dp.setBuffer(duckutils::convertStringToVector(cdppayload));
                /*-------Read from lora end--------------*/


                pd.handleReceivedPacket(dp);
                

                /*-------Send to web server begin-----------*/
                receivedData = dp.getBuffer();

                string messageForWeb = sendToWeb(receivedData);

                publish(redisConnect, redisConfigWeb.stream_name, "CDP_WEB", messageForWeb, redisConfigWeb.response);

                redisConfigLora.messageBuffer.clear();

                /*-------Send to web server end-----------*/

                
                
            }
            else if (DUCKTYPE == "LINK") {

                /*-----setup duck link begin -------*/
                dl.setDuckId(duckutils::convertStringToVector("DUCK0001"));
                
                /*-----setup duck link end -------*/


                dl.prepareForSending(&filter, dduid, topic, dl.getType(), 0x00, data);
                

                /*---------Send to Lora begin---------*/
                payload = dl.getBuffer();
                cdppayload = duckutils::convertVectorToString(payload);

                cdppayload = encodeCDP(cdppayload);
                
                cout << cdppayload << endl;

                cdppayload = decodeCDP(cdppayload);
                cout << cdppayload;
                 cdppayload = encodeCDP(cdppayload);
                 cout << cdppayload << endl;
                //cdppayload = modifystring(cdppayload, TOPIC_POS)

                publish(redisConnect, redisConfigLora.stream_name, "CDP_LORA", cdppayload, redisConfigLora.response);
                /*---------Send to Lora end---------*/
                
                
            }
            else if (DUCKTYPE == "DETECTOR") {
                /*dd.setDuckId(duckutils::convertStringToVector("DETECTOR"));

                //make sure ping publishes to CDP_LORA in code
                dd.sendPing(1);
                dd.handleReceivedPacket();*/
                
            }
            else {
                cout << "Error setting duck type. Recheck spelling and ensure all letters are capitalized." << endl;
                
            }
        }
        else {
            cout << "No message received. Waiting for next poll..." << endl;
            sleep(5);
        }

        // Sleep for a while before checking again
        
        //sleep(1); // Sleep for 1 second (or adjust as needed)
    }
    redisFree(redisConnect);

    return 0;
}