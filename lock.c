#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <error.h>
#include <limits.h>

#include "lock.h"

#define LOCK_CSEC() do { pthread_mutex_lock (&pool->csec); } while (0)
#define UNLOCK_CSEC() do { pthread_mutex_unlock (&pool->csec); } while (0)
#define RETURN_UNLOCK_CSEC(code) do { UNLOCK_CSEC(); return (code); } while (0)

static lock_pool_t *pool = NULL;
static pthread_once_t pool_is_initialized = PTHREAD_ONCE_INIT;
static unsigned int ref_count = 0;

static void new_pool (void);

void init_lock_pool (void)
{
    pthread_once (&pool_is_initialized, new_pool);

    LOCK_CSEC ();
    if (ref_count >= UINT_MAX) {
        // fail
    }
    ++ref_count;
    UNLOCK_CSEC ();
}

static void new_pool (void)
{
    pool = calloc (1, sizeof (lock_pool_t));
    if ( pool == NULL ) {
        //error("Memory allocation failed");
        return;
    }

    pthread_mutex_init (&pool->csec, NULL);
}

void destroy_lock_pool (void)
{
    assert (pool != NULL);
    LOCK_CSEC ();

    --ref_count;

    if ( ref_count > 0 ) { UNLOCK_CSEC(); return; }

    lock_node_t *tail = pool->tail;
    while (tail != NULL) {
        lock_node_t *new_tail = tail->prev;
        pthread_mutex_unlock (&tail->lock->mutex);
        pthread_mutex_destroy (&tail->lock->mutex);
        free (tail->lock);
        free (tail);
        tail = new_tail;
    }
    UNLOCK_CSEC ();

    pthread_mutex_destroy (&pool->csec);
    free ((void *)pool);

    pool = NULL;
    pool_is_initialized = PTHREAD_ONCE_INIT;
}

/* search for keys */
static int search_key (const char *const username, const lock_node_t **node)
{
    assert (pool != NULL);

    if ( pool->tail == NULL ) { return 0; }

    const lock_node_t *tail = pool->tail;
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

static int add_key (const char *const username)
{
    assert (pool != NULL);

    lock_node_t *const new_node = calloc (1, sizeof (lock_node_t));
    if ( new_node == NULL ) { return 0; }

    new_node->lock = calloc (1, sizeof (lock_t));
    if ( new_node->lock == NULL ) {
        free (new_node);
        // fail with something
        return 0;
    }

    /* -1 for null char */
    strncpy (new_node->lock->id, username, (sizeof (new_node->lock->id) - 1));
    pthread_mutex_init (&new_node->lock->mutex, NULL);
    pthread_mutex_lock (&new_node->lock->mutex);
    new_node->lock->instances = 1;

    /* Ok. We have lock allocated and zeroed by calloc. */
    if ( pool->tail != NULL ) {
        pool->tail->next = new_node;
    }
    new_node->prev = pool->tail;
    pool->tail = new_node; 

    return 1;
}

int get_lock (const char *const username)
{
    assert (pool != NULL);
    LOCK_CSEC ();
    /*
     * Global critical section
     */

    const lock_node_t *node = NULL;
    int ret = search_key (username, &node);
    if ( ret == 1 ) {
        assert (node != NULL);
        lock_t *const lck = node->lock;
        assert (lck != NULL);

        pthread_mutex_t *const mutex = &lck->mutex;
        if ( lck->instances >= UINT_MAX ) {
            // fail - how?
        }
        ++lck->instances;

        /* Unlock global critical section */
        UNLOCK_CSEC ();

        /* lock */
        pthread_mutex_lock (mutex);
        return 1;
    }
    /* no keys found - add new key to list */
    ret = add_key (username);
    RETURN_UNLOCK_CSEC (ret);
}

int release_lock (const char *const username)
{
    /* Critical section */
    assert (pool != NULL);
    LOCK_CSEC ();

    const lock_node_t *node = NULL;
    int ret = search_key (username, &node);

    /* Also exit of critical section */
    if ( ret == 0 ) { RETURN_UNLOCK_CSEC (ret); }

    assert (node != NULL);

    --node->lock->instances;
    if ( node->lock->instances > 0 ) {
        pthread_mutex_unlock (&node->lock->mutex);
        RETURN_UNLOCK_CSEC (ret);
    }

    lock_node_t *prev = node->prev;
    lock_node_t *next = node->next;

    if ( next != NULL ) {
        next->prev = node->next;
    } else {
        /* we are last element */
        pool->tail = NULL;
    }
    if ( prev != NULL ) { prev->next = node->prev; }

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
