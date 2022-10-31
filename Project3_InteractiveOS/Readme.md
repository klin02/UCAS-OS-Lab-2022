# Project3_InteractiveOS

### Completion of Task

+ TASK 1-4 均已完成（C-Core），且通过上板测试。
+ 支持滚屏操作
+ 支持进程组操作（新增ppid，杀死父进程时会自动杀死子进程）
+ 支持杀死进程时同步信号的释放
+ 使用大内核锁实现互斥访问，`multicore`加速比为1.8~2

### Running Tips

+ qemu上分别使用`make run-smp`和`make debug-smp`进行运行和调试
+ 板卡上仍使用`make minicom`进行运行
+ 使用`loadbootd`可运行单核，`loadbootm`可运行多核

### More Details

+ debug过程可见[实验过程文档.md](实验过程文档.md)
+ 问题解答详情可见[design_review.md](design_review.md)
  + spawn/exec，kill，wait，exit代码及实现要点
    + 进程回收实现要点
  + 杀死持有锁进程的处理流程
  + 屏障和条件变量的处理流程
    + 时间中断下的原子性保证
  + 信箱结构及处理流程
    + 并发访问保护
  + 多核的相关处理
    + 启动、内核锁的申请与释放、current_running的指定、调度
  + 讨论——同步信号回收
  + 讨论——进程回收
  + 注：支持使用Marp生成PPT

