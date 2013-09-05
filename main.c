#include <stdio.h>
#include <stdlib.h>

#include "lock.h"

int main (int argc, char **argv)
{
    lock_pool_t *pool = NULL;

    init_lock_pool ();

    int count = 20000;

    int p = 0;
    for ( p = 0; p < count; p++ ) {
        char username[100] = { 0 };
        sprintf (username, "username%d", p);
        get_lock (username);
    }

    for ( p = 0; p < count; p++ ) {
        char username[100] = { 0 };
        sprintf (username, "username%d", p);
        release_lock (username);
    }

    for ( p = 0; p < count; p++ ) {
        char username[100] = { 0 };
        sprintf (username, "username%d", p);
        get_lock (username);
    }
    
    destroy_lock_pool ();

    return EXIT_SUCCESS;
}
