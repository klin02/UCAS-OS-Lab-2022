#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define START 0x100000 

int stop=0;
void stophere();
int main(){
        //以数组形式进行遍历，一个4K页共含512项，对4K个4K页进行测试
        //两轮测试：第一轮不存在，进行分配；第二轮从swap分区换出
        //每512页打印一次测试数据
	uint64_t *array = START;
	int i, j;
	sys_move_cursor(1, 2);
	printf("Testing First Loop:\n");
	//for(i = 0; i < 4096; i++){
	for(i = 0; i < 1024; i++){	
		// if(i == 221)
		// 	stophere();
                array[i*512] = i;
		if(i % 64 == 0)
		// if(i > 220)
			printf("%04d:%04d ", i, array[i * 512]);
	}
	printf("First Loop done!\n");
	sys_clear();
	sys_move_cursor(1, 2);
	printf("Test Second Loop:\n");
	//for(i = 0; i < 4096; i++){
	for(i = 0; i < 1024; i++){
		if(i % 64 == 0)
			printf("%04d:%04d ", i, array[i * 512]);
	}
	printf("\n Test Done\n");
	return 0;
}
// void stophere(){
// 	stop = 1;
// }