#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <error.h>
#include <limits.h>
#include <glib.h>

#include "lock.h"

#define LOCK_CSEC() do { pthread_mutex_lock (&pool->csec); } while (0)
#define UNLOCK_CSEC() do { pthread_mutex_unlock (&pool->csec); } while (0)
#define RETURN_UNLOCK_CSEC(code) do { UNLOCK_CSEC(); return (code); } while (0)

static lock_pool_t *pool = NULL;
static GHashTable *hash_table = NULL;

static pthread_once_t pool_is_initialized = PTHREAD_ONCE_INIT;
static unsigned int ref_count = 0;

/* Callbacks */
static void new_pool (void);
static void free_lock (gpointer lock);
/**/

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

    hash_table = g_hash_table_new_full (&g_str_hash, &g_direct_equal, NULL, &free_lock);
    if ( hash_table == NULL ) {
        // fail
        return;
    }

    pthread_mutex_init (&pool->csec, NULL);
}

static void free_lock (gpointer lock)
{
    lock_t *const lck = (lock_t *) lock;

    pthread_mutex_unlock (&lck->mutex);
    pthread_mutex_destroy (&lck->mutex);

    free (lock);
}

void destroy_lock_pool (void)
{
    assert (pool != NULL);
    LOCK_CSEC ();

    --ref_count;

    if ( ref_count > 0 ) { UNLOCK_CSEC(); return; }

    assert (hash_table != NULL);
    /* Initialize hash table. */
    /* Remove all pairs. Memory is deallocated by free_lock callback. */
    g_hash_table_destroy (hash_table);
    hash_table = NULL;

    UNLOCK_CSEC ();

    pthread_mutex_destroy (&pool->csec);
    free ((void *)pool);

    /* Initializae lock pool. */
    pool = NULL;
    pool_is_initialized = PTHREAD_ONCE_INIT;
}

/* search for keys */
static int search_key (const char *const username, lock_t **lck)
{
    assert (pool != NULL);

    *lck = (lock_t *) g_hash_table_lookup (hash_table, (gpointer) username);

    if ( *lck != NULL ) {
        return 1;
    }

    return 0;
}

static int add_key (const char *const username)
{
    assert (pool != NULL);

    lock_t *const new_lock = calloc (1, sizeof (lock_t));
    if ( new_lock == NULL ) {
        // report to cim
        return 0;
    }

    /* -1 for null char */
    strncpy (new_lock->id, username, (sizeof (new_lock->id) - 1));
    pthread_mutex_init (&new_lock->mutex, NULL);
    pthread_mutex_lock (&new_lock->mutex);
    new_lock->instances = 1;

    g_hash_table_insert (hash_table, (gpointer) username, new_lock);
    return 1;
}

int get_lock (const char *const username)
{
    assert (pool != NULL);
    LOCK_CSEC ();
    /*
     * Global critical section
     */

    lock_t *lck = NULL;
    int ret = search_key (username, &lck);
    if ( ret == 1 ) {
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

    lock_t *lck = NULL;
    int ret = search_key (username, &lck);

    /* Also exit of critical section */
    if ( ret == 0 ) { RETURN_UNLOCK_CSEC (ret); }

    assert (lck != NULL);

    --lck->instances;
    if ( lck->instances > 0 ) {
        pthread_mutex_unlock (&lck->mutex);
        RETURN_UNLOCK_CSEC (ret);
    }

    g_hash_table_remove (hash_table, (gpointer) username);
    
    RETURN_UNLOCK_CSEC (ret);
}

#undef PREALOCATED_LOCKS_NUM
#undef LOCK_CSEC
#undef UNLOCK_CSEC
#undef RETURN_UNLOCK_CSEC
