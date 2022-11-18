#include <type.h>

typedef pid_t pthread_t;
void do_pthread_create(pthread_t *thread,
                   void (*start_routine)(void*),
                   void *arg);

int do_pthread_join(pthread_t thread);