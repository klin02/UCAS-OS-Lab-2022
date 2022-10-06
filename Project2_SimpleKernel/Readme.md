# Project2_SimpleKernel

## Part1

### Completion of Task

+ Task 1-2 均已完成，且通过上板测试。
+ 实现进程切换调度和互斥锁的阻塞与释放
+ 使用跳转表形式实现用户程序对内核程序的调用
+ 实现多锁机制，支持自行设置映射机制，使得用户程序能申请多把锁
+ PCB增加`lock_time`结构，锁释放时新增ROB（Re-order Buffer）逻辑，支持锁的公平分配，防止进程饥饿现象


## Running Tips

+ 输入`loadboot`即可自动运行


## More detail

+ debug过程可见[part1_实验过程文档.md](part1_实验过程文档.md)
+ 问题解答详情可见[part1_design_review.md](part1_design_review.md)
  + switch_to上下文切换方法
  + PCB及内核栈初始化
  + switchto_context_t结构体详情
  + 用户程序信息结构体及初始化
  + 调度程序（do_scheduler）执行流程
  + 阻塞任务的存放位置（单锁/多锁）
  + 单锁和多锁机制实现步骤
  + 多锁模式下进程饥饿现象及解决方法
  + 注：支持使用Marp生成PPT