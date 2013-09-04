#ifndef _LOCK_H
#define _LOCK_H

#include <pthread.h>

#define USERNAME_LEN_MAX 32

typedef struct lock {
    char id [USERNAME_LEN_MAX];
    pthread_mutex_t mutex;
    unsigned int instances;
} lock_t;

typedef struct lock_node {
    lock_t *lock;
    struct lock_node *prev;
    struct lock_node *next;
} lock_node_t;

typedef struct lock_pool {
    lock_node_t* tail;
    pthread_mutex_t csec;
} lock_pool_t;

void init_lock_pool (void);
void destroy_lock_pool (void);
int get_lock (const char *const username);
int release_lock (const char *const username);

#undef USERNAME_LEN_MAX
#endif /* _LOCK_H */
