#ifndef _LOCK_H
#define _LOCK_H

#include <pthread.h>
#include <glib.h>

#define USERNAME_LEN_MAX 32

typedef struct lock {
    char id [USERNAME_LEN_MAX];
    pthread_mutex_t mutex;
    unsigned int instances;
} lock_t;

typedef struct lock_pool {
    GHashTable *hash_table;
    pthread_mutex_t csec;
} lock_pool_t;

int init_lock_pool (void) __attribute__((warn_unused_result));

void destroy_lock_pool (void);
int get_lock (const char *const username) __attribute__((nonnull));
int release_lock (const char *const username) __attribute__((nonnull));

#undef USERNAME_LEN_MAX
#endif /* _LOCK_H */
