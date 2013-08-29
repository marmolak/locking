#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "lock.h"

#define LOCK_CSEC() do { pthread_mutex_lock (&pool->csec); } while (0)
#define UNLOCK_CSEC() do { pthread_mutex_unlock (&pool->csec); } while (0) 
#define RETURN_UNLOCK_CSEC(code) do { UNLOCK_CSEC(); return (code); } while (0)

int init_lock_pool (lock_pool_t **pool_arg)
{
    if ( pool_arg == NULL ) { return 0; }
    *pool_arg = malloc (sizeof (lock_pool_t));
    if ( *pool_arg == NULL ) { return 0; }

    lock_pool_t *pool = *pool_arg;

    pool->tail.lock = NULL;
    pool->tail.prev = NULL;

    pthread_mutex_init (&pool->csec, NULL);
    return 1;
}

int destroy_pool (lock_pool_t **pool_arg)
{
    if ( pool_arg == NULL ) { return 0; }
    if ( *pool_arg == NULL ) { return 0; }
    
    lock_pool_t *pool = *pool_arg;

    lock_node_t *tail = pool->tail.prev;
    while (tail != NULL) {
        lock_node_t *new_tail = tail->prev;
        free (tail->lock);
        free (tail);
        tail = new_tail;
    }

    free ((void *)pool);
    *pool_arg = NULL;

    return 1;
}

/* search for keys */
static int search_key (lock_pool_t *const pool, const char *const username, const lock_node_t **node)
{
    assert (pool != NULL);

    if ( pool->tail.prev == NULL ) { return 0; }

    const lock_node_t *tail = pool->tail.prev;
    while (tail != NULL) {
        const char *usr = tail->lock->id;

        if ( strcmp (usr, username) == 0 ) {
            *node = tail;
            return 1;
        }

        tail = tail->prev;
    }

    return 0;
}

static int add_key (lock_pool_t *const pool, const char *const username) {

    lock_node_t *const tail = &pool->tail;
    
    lock_node_t *new_node = malloc (sizeof (lock_node_t));
    if ( new_node == NULL ) { return 0; }

    new_node->lock = malloc (sizeof (lock_t));
    if ( new_node == NULL ) {
        free (new_node);
        return 0;
    }

    /* -1 for null char */
    strncpy (new_node->lock->id, username, (sizeof (tail->lock->id) - 1));
    pthread_mutex_init (&new_node->lock->mutex, NULL);
    pthread_mutex_lock (&new_node->lock->mutex);

    new_node->prev = tail->prev;
    new_node->next = tail;

    pool->tail.prev = new_node;

    return 1;
}

int get_lock (lock_pool_t *pool, const char *const username)
{
    assert (pool != NULL);
    /*
     * Global critical section
     */
    LOCK_CSEC ();

    const lock_node_t *node = NULL;
    int ret = search_key (pool, username, &node);
    if ( ret == 1 ) {
        assert (node != NULL);
        lock_t *lck = node->lock;

        assert (lck != NULL);

        pthread_mutex_t *const mutex = &lck->mutex;
        /* Unlock global critical section */
        UNLOCK_CSEC ();
        pthread_mutex_lock (mutex);
        return 1;
    }
    /* no keys found - add new key to list */

    ret = add_key (pool, username);
    RETURN_UNLOCK_CSEC (ret);
}

int unleash_lock (lock_pool_t *pool, const char *const username)
{
    assert (pool != NULL);
    /* Critical section */
    LOCK_CSEC ();

    const lock_node_t *node = NULL;
    int ret = search_key (pool, username, &node);
    /* Also exit of critical section */
    if ( ret != 1 ) { RETURN_UNLOCK_CSEC (ret); }

    assert ( node != NULL );

    lock_node_t *prev = node->prev;
    lock_node_t *next = node->next;

    if ( next != NULL ) { next->prev = prev; }
    if ( prev != NULL ) { prev->next = next; }

    pthread_mutex_unlock (&node->lock->mutex);
    pthread_mutex_destroy (&node->lock->mutex);

    free (node->lock);
    free ((void *)node);
    
    RETURN_UNLOCK_CSEC (ret);
}

#undef PREALOCATED_LOCKS_NUM
#undef LOCK_CSEC
#undef UNLOCK_CSEC
#undef RETURN_UNLOCK_CSEC
