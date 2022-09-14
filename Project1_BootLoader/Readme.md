# Project1_Bootloader
## Completion of Task
+ Task 1-5 均已完成（C-Core），且通过上板测试。
+ 支持为运行的任务分配空闲用户栈帧，实现见`main.c`及`crt0.S`
+ 支持多种模式调用用户程序，包含id/name/bat1(id-order)/bat2(name-order)
+ 支持输入非法调用模式，或用户交互时输入非法id/name时重新输入
+ 支持新增用户程序，只需在`test`文件夹中新增对应文件

## Running Tips
+ 在运行`loadboot`指令后选择调用模式
+ 可修改`./build`文件夹中的`bat1.txt`以通过id批处理调用用户程序
+ 可修改`./build`文件夹中的`bat2.txt`以通过name批处理调用用户程序


## More detail
+ debug过程可见[实验过程文档.md](实验过程文档.md)
+ 问题解答详情可见[design_review.md](design_review.md)
  + 代码架构
  + 项目运行流程
  + 内核装载运行方式
  + 用户程序信息结构体及初始化
  + 扇区结构
  + 其他问题及思考
  + 注：支持使用Marp生成PPT
