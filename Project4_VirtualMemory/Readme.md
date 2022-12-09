# Project4_VirtaulMemory

### Completion of TASK

+ TASK 1-6 均已完成（C-Core），且通过上板测试
+ 支持进程及线程回收
+ 添加自行测试程序`swap.c`及`snapshot.c`，分别测试swap功能和快照功能
+ 添加程序`idle.c`作为空泡进程，当就绪队列为空时，将调度至该进程
+ 更改kill逻辑
  + 取消进程组操作，杀死父进程后不再自动杀死子进程
  + kill时，将同步杀死该进程的所有线程
+ swap功能测试
  + 由于qemu上限制只能读取镜像内的扇区，如需测试该功能，需要在`Makefile`中`image`部分最后一行添加如下命令
    ```makefile
    dd if=/dev/zero of=$(DIR_BUILD)/image oflag=append conv=notrunc bs=32MB count=1
    ```
  
    

### Running Tips

+ qemu上使用`make run-smp`和`make debug-smp`进行运行和调试
+ 板卡上使用`make minicom`运行
+ 单核运行使用`loadboot`，双核运行使用`loadbootm`



### More Details

+ debug过程可见[实验过程文档.md](实验过程文档.md)
+ 问题解答详情可见[design_review.md](design_review.md)
  + 页表及页表项结构，大小及分配方式等
  + 用户进程页表设置及程序载入
  + 页缺失错误处理流程
  + 替换策略及测试程序
  + 线程管理结构
  + 线程的创建与启动、结束与回收、调度
  + 注：支持通过Marp生成PPT