#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define NUM_TB 3
#define BARR_KEY 58
#define BUF_LEN 20

int main(int argc, char *argv[])
{
    assert(argc >= 1);
    int print_location = (argc == 1) ? 0 : atoi(argv[1]);

    // Initialize barrier
    int handle = sys_barrier_init(BARR_KEY, NUM_TB);

    // Launch child processes
    pid_t pids[NUM_TB];
    
    // Start three test programs
    for (int i = 0; i < NUM_TB; ++i)
    {
        char buf_location[BUF_LEN];
        char buf_handle[BUF_LEN];
        assert(itoa(print_location + i, buf_location, BUF_LEN, 10) != -1);
        assert(itoa(handle, buf_handle, BUF_LEN, 10) != -1);

        char *argv[3] = {"test_barrier", buf_location, buf_handle};
        pids[i] = sys_exec(argv[0], 3, argv);

    }
    //while(1);
    // Wait child processes to exit
    //printf("                                     pidlist : %d %d %d",pids[0],pids[1],pids[2]);
    // printf("\n\n\n");
    // sys_ps();
    // int cnt=0;
    // while(1){
    //     for(int i=0;i<1000000;i++)
    //     ;
    //     // sys_yield();
    //     sys_move_cursor(0,29);
    //     printf("bar %d\n",cnt++);
    //     sys_ps();
    //     // sys_move_cursor(0,28);
    //     // printf("pid %d",2);
    //     //asm volatile("wfi");
    // }

    //while(1) ;
    for (int i = 0; i < NUM_TB; ++i)
    {
        sys_waitpid(pids[i]);
    }
    // Destroy barrier
    sys_barrier_destroy(handle);

    return 0;
}