/**
 * @file pq.h
 *
 * An implementation of a heap-based priority queue.
 *
 * @author Ulf Söderberg
 */

#include <stdint.h>
#include <stdbool.h>

#include "debug.h"

#pragma once

#define PQ_DEFAULT_HEAPLEN  64

//#if defined(CUSTOM_PQ_HEAPLEN)
//#define PQ_HEAPLEN CUSTOM_PQ_HEAPLEN
//#else
#define PQ_HEAPLEN PQ_DEFAULT_HEAPLEN
//#endif

typedef struct pq_s pq_t;

typedef struct
{
    uint32_t prio;      // priority of this entry
    const char *name;   // name of this entry
    pq_t *q;            // ptr back to owning priority queue
} pq_entry_t;

/**
 * @brief Control struct for a priority queue.
 */
struct pq_s
{
    uint32_t len;                   // current len of heap
    pq_entry_t *heap[PQ_HEAPLEN];   // ptr to array of heap entries
    const char *name;               // name of this priority queue
};

/**
 * @brief Prints info on each entry in the priority queue
 * 
 * @param q the priority queue
 */
void pq_print(pq_t *q);

/**
 * @brief Get name of specified priority queue
 * 
 * @param q the priority queue
 * @return const char* ptr to the name of the queue
 */
static inline const char *pq_name(pq_t *q)
{
    ASSERT(q != NULL);
    ASSERT(q->name != NULL);

    return q->name;
}

/**
 * @brief Initialises the priority queue
 * 
 * @param q the priority queue
 */
void pq_init(pq_t *q, const char *name);

/**
 * @brief Initialises priority queue and insert entries from an array
 * 
 * @param q the priority queue
 * @param a the array containing pointers to the entries to be added to queue
 * @param len number of entries in array
 */
void pq_init_from_array(pq_t *q, const char *name, pq_entry_t *a[], uint32_t len);

/**
 * @brief Get current size of the priority queue
 * 
 * @param q the priority queue
 * @return uint32_t number of entries currently in the queue
 */
static inline uint32_t pq_size(pq_t *q)
{
    ASSERT(q != NULL);

    return q->len - 1;
}

/**
 * @brief Checks if priority queue is empty
 * 
 * @param q the priority queue
 * @return true if queue is empty
 * @return false if queue is NOT empty
 */
static inline bool pq_empty(pq_t *q)
{
    ASSERT(q != NULL);

    return pq_size(q) == 0;
}

/**
 * @brief Inserts an entry in the priority queue
 * 
 * @param q the priority queue
 * @param item the item to be inserted
 * @return true if successful insertion
 * @return false if the queue was full
 */
bool pq_insert(pq_t *q, pq_entry_t *item);

/**
 * @brief Removes highest priority entry
 * 
 * @param q the priority queue
 * @return pq_entry_t* pointer to removed entry
 */
pq_entry_t *pq_remove(pq_t *q);
