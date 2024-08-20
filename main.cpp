#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cstdint>
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
//#include "PapaDuck.h"
//#include "DetectorDuck.h"
#include "redis.h"

using namespace std;



string extractValue(const std::string& input, const std::string& key) {
    string::size_type startPos = input.find(key);
    if (startPos == std::string::npos) return"";  // Key not found
    
    startPos += key.length();  // Move past the key
    std::string::size_type endPos = input.find(' ', startPos);
    if (endPos == std::string::npos) endPos = input.length();  
    string data = input.substr(startPos, endPos - startPos);
    return data;
}


   struct StreamInfo {
    const char* mystream;
    const char* key;
    const char* message;
    const char* group_name;
};

int main()
{

    
   

//////connect redis server
redisContext* redisConnect = redis_init("localhost", 6379);
const char* mystream = "mystream";
	const char* key = "WEB_CDP";
	const char* value = "DUID:MAMA0003 TOPIC:status DATA:Test_Data_String DUCKTYPE:LINK";
    const char* value2 = "DUID:MAMA0003 TOPIC:status DATA:Test_Data_String_Again DUCKTYPE:LINK";
    const char* groupName = "CDP";
    const char * txWebQueue = "WEBTX";
    const char * txLoraQueue = "LORATX";
    char message_id[255];

    char  taskBuffer[255];

    redisCommand(redisConnect, "DEL %s", txWebQueue);
    redisCommand(redisConnect, "DEL %s", txLoraQueue);
	  // Use a single buffer to receive the response
    char response[256];  // Adjust the size according to your needs
    create_consumer_group(redisConnect, mystream, groupName);
    enqueue_task(redisConnect, txWebQueue, value);
    publish(redisConnect, mystream, key, value, response);
    enqueue_task(redisConnect, txWebQueue, value2);
    publish(redisConnect, mystream, key, value2, response);



/*---------read from webserver---------*/
    
    while (true) {
        // Read from the consumer group
        const string stream_name = "mystream";
        const string group_name = "TX";
        const char* streamName = "mystream";
        const char* groupNm = "TX";
        //const char* group_name2 = "RX";
        const string consumer_name = "CDP";
        string filter_key = "WEB_CDP";
        const string lora_queue = "LORA";
        string response;
        string key_buffer;
        string  messageBuffer;
        string messageID;
        bool messageReceived = 1;
        //string response;

        //read_from_consumer_group(redisConnect, stream_name, group_name, consumer_name, filter_key, key_buffer, messageBuffer, messageID);
        //check_pending_messages(redisConnect, streamName, groupNm);
        int queueLength = queue_len(redisConnect, txWebQueue);
        if (queueLength == 0) {
			printf("No message received from Web\n");
            messageReceived = 0;
		}
       
        if (messageReceived) {
            // Process the received message
            
            dequeue_task(redisConnect, txWebQueue,taskBuffer );
            string process = taskBuffer; // Assume message_buffer contains the raw data
            acknowledge_message(redisConnect, streamName, groupNm, message_id);

            string DUID = extractValue(process, "DUID:");
            string TOPIC = extractValue(process, "TOPIC:");
            string DATA = extractValue(process, "DATA:");
            string DUCKTYPE = extractValue(process, "DUCKTYPE:");

            cout << "DUID: " << DUID << endl;
            cout << "TOPIC: " << TOPIC << endl;
            cout << "DATA: " << DATA << endl;
            cout << "DUCKTYPE: " << DUCKTYPE << endl;

            vector<uint8_t> dduid = duckutils::convertStringToVector(DUID);
            uint8_t topic = Packet::stringToTopic(TOPIC);
            vector<uint8_t> data = duckutils::convertStringToVector(DATA);

            // Instantiate Packet Object
            Packet dp;
            DuckLink dl;
            //DetectorDuck dd;
            //MamaDuck md;
            //PapaDuck pd;

            // Create BloomFilter
            BloomFilter filter = BloomFilter(DEFAULT_NUM_SECTORS, DEFAULT_NUM_HASH_FUNCS, DEFAULT_BITS_PER_SECTOR, DEFAULT_MAX_MESSAGES);

            if (DUCKTYPE == "MAMA") {
                /*md.setDuckId(duckutils::convertStringToVector("MAMA0001"));
                md.handleReceivedPacket(dp);*/
                // Add code for socket connection to Lora for TX and RX
            }
            else if (DUCKTYPE == "PAPA") {
                /*pd.setDuckId(duckutils::convertStringToVector("PAPA0001"));
                pd.handleReceivedPacket(dp);*/
                // Add code for socket connection to Lora for RX and maybe TX for duck commands
            }
            else if (DUCKTYPE == "LINK") {
                dl.setDuckId(duckutils::convertStringToVector("DUCK0001"));
                dl.prepareForSending(&filter, dduid, topic, dl.getType(), 0x00, data);
                //dl.sendToLora(redisConnect, dl.getBuffer());
                /*---------Send to Lora---------*/
                vector<uint8_t> payload = dl.getBuffer();
                char* charPtr = new char[payload.size() + 1];
                memcpy(charPtr, payload.data(), payload.size());
                charPtr[payload.size()] = '\0';
                const char * txLoraQueue = "LORATX";
                enqueue_task(redisConnect, txLoraQueue, charPtr);
                /*---------Send to Lora---------*/
                delete[] charPtr;
                messageBuffer.clear(); // Clear the content of messageBuffer
                // Add code for socket connection to Lora for TX
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