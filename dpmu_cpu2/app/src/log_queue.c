/*
 * log_queue.c
 *
 *  Created on: 28 de out de 2024
 *      Author: gferreira
 */
#include "cli_cpu2.h"
#include "log_queue.h"

uint16_t byteCounterMessageToQueue = 0;
uint16_t byteCounterMessageFromQueue = 0;
char *messageQueueSpot;
char *newMessageToEnqueue;
char *dequeueingMessage;
char *outputMessage;

void InitLogQueue(QUEUE *queue) {
    queue->begin = 0;
    queue->end = 0;
    queue->current_load = 0;
    //memset(&queue->messages[0], 0, ( QUEUE_SIZE * MESSAGE_SIZE ) );
    byteCounterMessageToQueue = 0;
    byteCounterMessageFromQueue = 0;
}

bool StartEnque(QUEUE *queue, debug_log_t *debug_log_message ) {
    if (queue->current_load < QUEUE_SIZE) {
        if (queue->end == QUEUE_SIZE) {
            queue->end = 0;
        }
        byteCounterMessageToQueue = 0;
        messageQueueSpot = (char *)&queue->messages[queue->end];
        newMessageToEnqueue = (char *)debug_log_message;
        return true;
    } else {
        return false;
    }
}

bool CopyMessageToQueDone(QUEUE *queue ) {
    bool retVal = false;

    messageQueueSpot[byteCounterMessageToQueue]=newMessageToEnqueue[byteCounterMessageToQueue];
    byteCounterMessageToQueue++;
    if(byteCounterMessageToQueue==sizeof(debug_log_t) ) {
        retVal = true;
        queue->end++;
        queue->current_load++;
    }
    return retVal;
}

bool StartDeque(QUEUE *queue, debug_log_t *debug_log_message ) {
    if (queue->current_load > 0) {
        byteCounterMessageFromQueue = 0;
        dequeueingMessage = (char *)&queue->messages[queue->end];
        outputMessage = (char *)debug_log_message;
        return true;
    } else {
        return false;
    }
}

bool CopyMessageFromQueDone(QUEUE *queue) {
    bool retVal = false;
    outputMessage[byteCounterMessageFromQueue]=dequeueingMessage[byteCounterMessageFromQueue];
    byteCounterMessageFromQueue++;
    if( byteCounterMessageFromQueue == sizeof(debug_log_t) ) {
        retVal = true;
        queue->begin = (queue->begin + 1) % QUEUE_SIZE;
        queue->current_load--;
    }
    return retVal;
}
