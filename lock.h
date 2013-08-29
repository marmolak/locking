#ifndef _LOCK_H
#define _LOCK_H

#include <pthread.h>

typedef struct lock {
    char id[32];
    pthread_mutex_t mutex;
} lock_t;

typedef struct lock_node {
    lock_t *lock;
    struct lock_node *prev;
    struct lock_node *next;
} lock_node_t;

typedef struct lock_pool {
    lock_node_t tail;
    pthread_mutex_t csec;
} lock_pool_t;

int init_lock_pool (lock_pool_t **pool_arg);
int destroy_pool (lock_pool_t **pool_arg);
int get_lock (lock_pool_t *pool, const char *const username);
int unleash_lock (lock_pool_t *pool, const char *const username);

#endif /* _LOCK_H */
