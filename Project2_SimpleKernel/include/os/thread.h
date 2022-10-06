#include <type.h>

//void thread_create(ptr_t funcaddr,void *arg);
void thread_create(ptr_t funcaddr,void *arg,ptr_t rc_funcaddr);
void thread_recycle();