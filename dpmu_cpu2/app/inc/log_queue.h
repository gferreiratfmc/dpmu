/*
 * log_queue.h
 *
 *  Created on: 28 de out de 2024
 *      Author: gferreira
 */

#ifndef APP_INC_LOG_QUEUE_H_
#define APP_INC_LOG_QUEUE_H_

#include <stdbool.h>
#include "GlobalV.h"


#define QUEUE_SIZE 8
#define MESSAGE_SIZE sizeof(debug_log_t)

typedef struct {
    debug_log_t messages[QUEUE_SIZE];
    int begin;
    int end;
    int current_load;
} QUEUE;

void InitLogQueue(QUEUE *queue);
bool StartEnque(QUEUE *queue, debug_log_t *debug_log_message );
bool CopyMessageToQueDone(QUEUE *queue );
bool StartDeque(QUEUE *queue, debug_log_t *debug_log_message );
bool CopyMessageFromQueDone(QUEUE *queue);

#endif /* APP_INC_LOG_QUEUE_H_ */
