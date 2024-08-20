#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis/hiredis.h>

void handleMessage(char *message);

int main(void) {
    // Connect to Redis server
    redisContext *c = redisConnect("127.0.0.1", 6379);
    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
    }

    // Publish a message to the channel
    const char *tx_channel = "txchannel";
    const char *message = "Hello world, how are you";
    redisReply *reply = redisCommand(c, "PUBLISH %s %s", tx_channel, message);

    printf("Message published, subscribers received: %lld\n", reply->integer);

    // Free the reply object
    freeReplyObject(reply);

    // Now subscribe to a different channel to receive messages
    const char *rx_channel = "rxchannel";
    reply = redisCommand(c, "SUBSCRIBE %s", rx_channel);
    freeReplyObject(reply);

    // Listen for incoming messages
    while (redisGetReply(c, (void**)&reply) == REDIS_OK) {
        if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 3) {
            if (strcmp(reply->element[0]->str, "message") == 0) {
                printf("Received message: %s\n", reply->element[2]->str);

                // Handle the received message
                handleMessage(reply->element[2]->str);
            }
        }
        freeReplyObject(reply);
    }

    // Disconnect and free the context
    redisFree(c);
    
    return 0;
}

// Function to handle the received message
void handleMessage(char *message) {
    printf("Handling received message: %s\n", message);
}