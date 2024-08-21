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

//string read_first_message_with_key(redisContext *c, const std::string& stream_name, const std::string& group_name, const std::string& consumer_name, const std::string& filter_key);
//int acknowledge_message(redisContext *c, const std::string& stream_name, const std::string& group_name, const std::string& message_id);
//void read_from_consumer_group(redisContext *c, const std::string& stream_name, const std::string& group_name, const std::string& consumer_name, const std::string& filter_key, std::string& key_buffer, std::string& message_buffer, std::string& message_id);
vector<std::string> extractValues(const std::string input) {
    std::vector<std::string> values;

    // Use a stringstream to process the input string
    std::stringstream ss(input);
    std::string token;

    // Temporary variables to store each field
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

    // Add values to vector in the required order
    values.push_back(duid);
    values.push_back(topic);
    values.push_back(data);
    values.push_back(ducktype);

    return values;
}



// Function to initialize RedisConfig
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

string modifystring (string cdp, int position){
    if(cdp[position] > 127){//this goes beyoned extended ascii range
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
    if((cdp[position] ^ 0x80) >127){//this goes beyoned extended ascii range
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



int main()
{

    
   

//////connect redis server
redisContext* redisConnect = redis_init("localhost", 6379);

RedisConfig redisConfig = initializeRedisConfig();
    redisCommand(redisConnect, "DEL %s", redisConfig.txWebQueue);
    redisCommand(redisConnect, "DEL %s", redisConfig.txLoraQueue);
    delete_stream(redisConnect, redisConfig.stream_name);

    string value = "DUID:MAMA0001 TOPIC:status DATA:Test Data String DUCKTYPE:LINK ";
    string value2 = "DUID:MAMA0001 TOPIC:status DATA:Test Data String Again DUCKTYPE:LINK ";

    string msg;
	  // Use a single buffer to receive the response
    //char response[256];  // Adjust the size according to your needs
    //string response;
    create_consumer_group(redisConnect, redisConfig.stream_name, redisConfig.group_name);
    enqueue_task(redisConnect, redisConfig.txWebQueue, value);
    publish(redisConnect, redisConfig.stream_name, redisConfig.key, value, redisConfig.response);
    enqueue_task(redisConnect, redisConfig.txWebQueue, value2);
    publish(redisConnect, redisConfig.stream_name, redisConfig.key, value2, redisConfig.response);



/*---------read from webserver---------*/
    
    while (true) {
        // Read from the consumer group
        
        int messageReceived = 1;
        int messageReceivedLora = 0;
        //string response;

        read_from_consumer_group(redisConnect, redisConfig.stream_name, redisConfig.group_name, redisConfig.consumer_name, redisConfig.filter_key, redisConfig.key_buffer, redisConfig.messageBuffer, redisConfig.messageID);

        //msg = read_first_message_with_key(redisConnect, redisConfig.stream_name, redisConfig.group_name, redisConfig.consumer_name,redisConfig.filter_key, redisConfig.messageID);

        //check_pending_messages(redisConnect, streamName, groupNm);
        /*int queueLength = queue_len(redisConnect, txWebQueue);
        if (queueLength == 0) {
			printf("No message received from Web\n");
            messageReceived = 0;
		}*/
        messageReceived = acknowledge_message(redisConnect, redisConfig.stream_name, redisConfig.group_name, redisConfig.messageID);
        
        if (messageReceived) {
            // Process the received message
            
            dequeue_task(redisConnect, redisConfig.txWebQueue,redisConfig.task );
            //string process = redisConfig.task; // Assume message_buffer contains the raw data
            //acknowledge_message(redisConnect, streamName, groupNm, message_id);

            
            std::vector<std::string> extractedValues = extractValues(redisConfig.task);
            msg.clear();
            redisConfig.key_buffer.clear();
            redisConfig.messageBuffer.clear();
            redisConfig.messageID.clear();
            //task.clear();
            //taskBuffer[0] = '\0';
            cout << "DUID: " <<extractedValues[0] << endl;
            cout << "TOPIC: " << extractedValues[1] << endl;
            cout << "DATA: " << extractedValues[2] << endl;
            cout << "DUCKTYPE: " << extractedValues[3] << endl;

            vector<uint8_t> dduid = duckutils::convertStringToVector(extractedValues[0]);
            uint8_t topic = Packet::stringToTopic(extractedValues[1]);
            vector<uint8_t> data = duckutils::convertStringToVector(extractedValues[2]);
            string DUCKTYPE = extractedValues[3];

            // Instantiate Packet Object
            Packet dp;
            DuckLink dl;
            //DetectorDuck dd;
            //MamaDuck md;
            PapaDuck pd;

            // Create BloomFilter
            BloomFilter filter = BloomFilter(DEFAULT_NUM_SECTORS, DEFAULT_NUM_HASH_FUNCS, DEFAULT_BITS_PER_SECTOR, DEFAULT_MAX_MESSAGES);

            if (DUCKTYPE == "MAMA") {
                /*md.setDuckId(duckutils::convertStringToVector("MAMA0001"));
                md.handleReceivedPacket(dp);*/
                // Add code for socket connection to Lora for TX and RX
            }
            else if (DUCKTYPE == "PAPA") {
                pd.setDuckId(duckutils::convertStringToVector("PAPA0001"));
                RedisConfig redisConfigRxLora = initializeRedisRxLoraConfig();
                /*-------Read from lora--------------*/
                while(!messageReceivedLora){
                    read_from_consumer_group(redisConnect, redisConfigRxLora.stream_name, redisConfigRxLora.group_name, redisConfigRxLora.consumer_name, redisConfigRxLora.filter_key, redisConfigRxLora.key_buffer,redisConfigRxLora.messageBuffer, redisConfigRxLora.messageID);
                    messageReceivedLora = acknowledge_message(redisConnect, redisConfigRxLora.stream_name, redisConfigRxLora.group_name, redisConfigRxLora.messageID);
                }
                
                
                /*-------Read from lora--------------*/
                pd.handleReceivedPacket(dp);
                

                /*-------Send to web server-----------*/
                
                vector<uint8_t> receivedData = dp.getBuffer();

                vector<uint8_t> duid;
                vector<uint8_t> receivedMsg;
                receivedMsg.assign(receivedData.begin()+DATA_POS, receivedData.end());
                duid.assign(receivedData.begin(), receivedData.begin() + 8);
                string receivedSduid = duckutils::convertVectorToString(duid);
                string receivedTopic = Packet::topicToString(receivedData.at(TOPIC_POS));
                string receivedMessage = duckutils::convertVectorToString(receivedMsg);
                string messageForWeb = "SDUID:" + receivedSduid + " TOPIC:"  + receivedTopic + " DATA:" + receivedMessage +" ";
                publish(redisConnect, redisConfig.stream_name, "CDP_WEB", messageForWeb, redisConfig.response);
                /*-------Send to web server-----------*/
                
            }
            else if (DUCKTYPE == "LINK") {
                dl.setDuckId(duckutils::convertStringToVector("DUCK0001"));
                dl.prepareForSending(&filter, dduid, topic, dl.getType(), 0x00, data);
                //dl.sendToLora(redisConnect, dl.getBuffer());
                /*---------Send to Lora---------*/
                vector<uint8_t> payload = dl.getBuffer();
                string cdppayload = duckutils::convertVectorToString(payload);
                
                publish(redisConnect, redisConfig.stream_name, "CDP_LORA", cdppayload, redisConfig.response);
                /*---------Send to Lora---------*/
                
                redisConfig.messageBuffer.clear(); // Clear the content of messageBuffer
               
            }
            else if (DUCKTYPE == "DETECTOR") {
                /*dd.setDuckId(duckutils::convertStringToVector("DETECTOR"));
                dd.sendPing(1);
                dd.handleReceivedPacket();*/
                // Add code for setup to send and receive pings
            }
            else {
                cout << "Error setting duck type. Recheck spelling and ensure all letters are capitalized." << endl;
                // Handle error
            }
        }
        else {
            cout << "No message received. Waiting for next poll..." << endl;
        }

        // Sleep for a while before checking again
        sleep(5); // Sleep for 1 second (or adjust as needed)
    }

    return 0;
}