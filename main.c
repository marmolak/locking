#include <stdio.h>
#include <stdlib.h>

#include "lock.h"

int main (int argc, char **argv)
{
    lock_pool_t *pool = NULL;

    init_lock_pool (&pool);

    int count = 10000;

    int p = 0;
    for ( p = 0; p < count; p++ ) {
        char username[100] = { 0 };
        sprintf (username, "username%d", p);
        get_lock (pool, username);
    }

    for ( p = 0; p < count; p++ ) {
        char username[100] = { 0 };
        sprintf (username, "username%d", p);
        unleash_lock (pool, username);
    }

    for ( p = 0; p < count; p++ ) {
        char username[100] = { 0 };
        sprintf (username, "username%d", p);
        get_lock (pool, username);
    }
    
    destroy_pool (&pool);

    return EXIT_SUCCESS;
}
