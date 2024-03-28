/**
 * @file pq.c
 *
 * An implementation of a heap-based priority queue.
 *
 * @author Ulf Söderberg
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "pq.h"

static pq_entry_t pq_root = {.prio = 0, .name = "PQ Root"};

static inline uint32_t pq_prio(pq_t *q, uint32_t i)
{
    return q->heap[i]->prio;
}

static inline const char *pqe_name(pq_t *q, uint32_t i)
{
    return q->heap[i]->name;
}

static void pq_swap(pq_t *q, uint32_t i, uint32_t j)
{
    pq_entry_t *di = q->heap[i];
    q->heap[i] = q->heap[j];
    q->heap[j] = di;
}

static void pq_upheap(pq_t *q, uint32_t i)
{
    uint32_t parind = i / 2;

    while (parind > 0 && pq_prio(q, i) < pq_prio(q, parind)) {
        pq_swap(q, i, parind);
        i = i / 2;
        parind = i / 2;
    }
}

static inline uint32_t qp_min_child(pq_t *q, uint32_t i)
{
    uint32_t lci = i * 2;
    uint32_t rci = lci + 1;

    if (rci > pq_size(q)) {
        return lci;
    } else if (pq_prio(q, lci) < pq_prio(q, rci)) {
        return lci;
    } else {
        return rci;
    }
}

static void pq_downheap(pq_t *q, uint32_t i)
{
    uint32_t lci = i * 2;

    while (lci <= pq_size(q)) {
        uint32_t child = qp_min_child(q, i);
        if (pq_prio(q, i) > pq_prio(q, child)) {
            pq_swap(q, i, child);
        }
        i = child;
        lci = i * 2;
    }
}

static inline pq_entry_t *pq_pop(pq_t *q)
{
    return q->heap[--q->len];
}

void pq_print(pq_t *q)
{
    ASSERT(q != NULL);
    ASSERT(q->name != NULL);

    printf("Current state of '%s':\n", q->name);
    printf("%8s    %s\n", "Prio", "Name");
    for (int i = 1; i < q->len; ++i) {
        printf("%8lu -- %s\n", pq_prio(q, i), pqe_name(q, i));
    }
}

void pq_init(pq_t *q, const char *name)
{
    static const char *no_name = "no name";

    ASSERT(q != NULL);

    q->len = 0;
    q->heap[q->len++] = &pq_root;
    q->name = name != NULL ? name : no_name;
}

void pq_init_from_array(pq_t *q, const char *name, pq_entry_t *a[], uint32_t len)
{
    ASSERT(a != NULL);
    ASSERT(len < PQ_HEAPLEN);

    pq_init(q, name);
    for (uint32_t i = 0; i < len; ++i) {
        q->heap[q->len++] = a[i];
    }

    uint32_t start_index = pq_size(q) / 2;

    for (uint32_t i = start_index; i > 0; --i) {
        pq_downheap(q, i);
    }
}

bool pq_insert(pq_t *q, pq_entry_t *item)
{
    ASSERT(q != NULL);
    ASSERT(item != NULL);

    item->q = q;
    q->heap[q->len++] = item;
    pq_upheap(q, q->len - 1);

    return true;
}

pq_entry_t *pq_remove(pq_t *q)
{
    ASSERT(q != NULL);

    pq_entry_t *retval = q->heap[1];
    pq_entry_t *repl = pq_pop(q);

    retval->q = NULL;

    if (pq_size(q) > 0) {
        q->heap[1] = repl;
        pq_downheap(q, 1);
    }

    return retval;
}
