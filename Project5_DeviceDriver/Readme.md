# Project5_DeviceDriver

## Completion of Task

+ TASK 1-5 均已完成（C-Core），且通过上板测试
+ 添加程序`idle.c`作为空泡进程，当就绪队列为空时，将调度至该进程
+ 添加自行测试程序`echo.c`，测试收包、修改、并再次发包的过程

## Running Tips

+ qemu上使用`make run-net`和`make debug-net`进行运行和调试
+ 板卡上使用`make minicom`运行
+ 单核运行使用`loadboot`，双核运行使用`loadbootm`


## More Details

+ debug过程可见[实验过程文档.md](实验过程文档.md)
+ 问题解答详情可见[design_review.md](design_review.md)
  + 发送/接收中断描述符内容（代码及相关设置）
  + 网卡中断相关流程
    + 初始化
    + 中断开关设置
    + 处理过程
  + 收包检查
    + 相关结构
    + 使用场景
    + 检查方法
    + 具体实现
  + 注：支持通过Marp生成PPT