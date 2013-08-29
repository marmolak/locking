#include "lock.h"

int main (int argc, char **argv)
{
    lock_pool_t *pool = NULL;

    char username[] = "kraljeliman";

    init_lock_pool (&pool);

    get_lock (pool, username);
    unleash_lock (pool, username);
    
    destroy_pool (&pool);
}
