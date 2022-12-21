# Project6_FileSystem

## Completion of Task

+ TASK 1-2 均已完成（A-Core），且通过上板测试
+ 添加程序`idle.c`作为空泡进程，当就绪队列为空时，将调度至该进程
+ 添加自行测试程序`bigfile.c`，用于测试大文件（8MB）的读写
+ 支持shell显示当前路径

## Running Tips

+ qemu上使用`make run-net`和`make debug-net`进行运行和调试
+ 板卡上使用`make minicom`运行

+ 单核运行使用`loadboot`，双核运行使用`loadbootm`

## More Details

+ debug过程可见[实验过程文档.md](实验过程文档.md)
+ 问题解答详情可见[design_review.md](design_review.md)
  + 磁盘布局
  + 文件系统元数据（superblock、inode、dentry、file descriptor）
  + 最多支持的文件和目录数
  + 文件系统初始化过程
  + 多级目录解析
  + 注：支持通过Marp生成PPT