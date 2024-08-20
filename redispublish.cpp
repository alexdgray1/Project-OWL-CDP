#include <iostream>
#include <string>
#include <cstring>
#include <hiredis/hiredis.h>
#include "redis.h"

// Function prototypes
redisContext* redis_init(const char* server, int port);
void publish(redisContext *redisConnect, const char *stream_name, const char *key, const char *value, char *response);
void create_consumer_group(redisContext* c, const char* stream_name, const char* group_name);
void read_from_consumer_group(redisContext *c, const char *stream_name, const char *group_name, const char *consumer_name, const char *filter_key, char *key_buffer, char *message_buffer, char *messageID);
//std::string read_first_message_with_key(redisContext *c, const char *stream_name, const char *group_name, const char *consumer_name, const char *filter_key);
void enqueue_task(redisContext *c, const char *queue_name, const char *task);
void print_queue(redisContext *c, const char *queue_name);
void dequeue_task(redisContext *c, const char *queue_name, char * taskBuffer);
// Main function to demonstrate usage
int main() {
    // Initialize Redis connection
    redisContext* redisConnect = redis_init("localhost", 6379);
    
    // Stream and group names
    const char* stream_name = "mystream";
    const char* group_name = "TX";
    const char* group_name2 = "RX";
    const char* consumer_name = "CDP";
    const char* filter_key = "WEB_CDP";
    const char* lora_queue = "LORA";

    // Create consumer group
    create_consumer_group(redisConnect, stream_name, group_name);
    //create_consumer_group(redisConnect, stream_name, group_name2);

    // Publish messages to the stream
    char response[256];

    ///for TX
    publish(redisConnect, stream_name, "WEB_CDP", "configuration message", response);
    publish(redisConnect, stream_name, "WEB_CDP", "configuration message2", response);
    publish(redisConnect, stream_name, "CDP_LORA", "tx payload message", response);

    //For RX
    publish(redisConnect, stream_name, "CDP_WEB", "message incoming", response);
    publish(redisConnect, stream_name, "LORA_CDP", "rx payload message", response);
    

    // Read messages from the consumer group
    char key_buffer[256] = {0};
    char message_buffer[256] = {0};
    char messageID[256] = {0};
    //std::string message;
    //read_from_consumer_group(redisConnect, stream_name, group_name, consumer_name, filter_key, key_buffer, message_buffer, messageID);
    //std::string message = read_first_message_with_key(redisConnect, stream_name, group_name, consumer_name, filter_key);
    read_from_consumer_group(redisConnect, stream_name, group_name, consumer_name, filter_key, key_buffer, message_buffer, messageID);

    // Print the contents of message_buffer
    //std::cout << "message Returned: " << message << std::endl;
    printf("Message Received: %s\n", message_buffer);

    
    //enqueue_task(redisConnect, lora_queue, message.c_str());
    print_queue(redisConnect, lora_queue);
    //std::string msg = read_first_message_with_key(redisConnect, stream_name, lora_queue, consumer_name, filter_key);
    //char* mutableCharArray = new char[message.size()];
    //dequeue_task(redisConnect, lora_queue, mutableCharArray);
    //delete[] mutableCharArray; // Free memory when done
    print_queue(redisConnect, lora_queue);

    //std::string messageTwo = read_first_message_with_key(redisConnect, stream_name, group_name, consumer_name, filter_key);
    //enqueue_task(redisConnect, lora_queue, messageTwo.c_str());
    print_queue(redisConnect, lora_queue);
    
    //read_from_consumer_group(redisConnect, stream_name, group_name, consumer_name, key_buffer, message_buffer, messageID);
    //read_from_consumer_group(redisConnect, stream_name, group_name2, consumer_name, key_buffer, message_buffer, messageID);
    //read_from_consumer_group(redisConnect, stream_name, group_name2, consumer_name, key_buffer, message_buffer, messageID);


    // Clean up
    redisFree(redisConnect);
    return 0;
}

