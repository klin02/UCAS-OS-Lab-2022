---
marp: true
theme: gaia 
paginate: true

header: Prj5_DeviceDriver &emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&ensp; 游昆霖 2020K8009926006
---
<!--_class: lead-->
# Project_5 &ensp;DeviceDriver
## —— Design Review
### 游昆霖 2020K8009926006

---
<!--_class: lead-->
# Question 1: 
### Please give an example about the 
### contents of filled Tx/Rx descriptors?

---
### Tx descriptors
```c
//e1000_configure_tx
    for(int i=0;i<TXDESCS;i++){
        tx_desc_array[i].addr = (uint64_t)(kva2pa((uintptr_t)tx_pkt_buffer[i]));
        tx_desc_array[i].length = 0;
        tx_desc_array[i].cso = 0;
        tx_desc_array[i].cmd = (uint8_t)(~E1000_TXD_CMD_DEXT & 
                                (E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP));
        tx_desc_array[i].status = 0; //将存储发送状态
        tx_desc_array[i].css = 0;
        tx_desc_array[i].special = 0;
    }
//e1000_transmit
    memcpy((uint8_t *)tx_pkt_buffer[tail],(uint8_t *)txpacket,length);
    tx_desc_array[tail].length = (uint16_t)length;
    tx_desc_array[tail].status = 0;
```

---
### Tx descriptors
+ 初始化
  + addr字段设置为相应缓冲区地址
  + cmd字段
    + DEXT位设置为0，表示按照Legacy布局解析描述符内容
    + RS位设置为1，令DMA控制器记录传输状态于STA字段
    + EOP位设置为1，表示每个描述符对应数据帧均为最后一帧
  + 其余字段均置零
+ 发送
  + 长度设置为待发送数据帧长度
  + status置0

---
### Rx descriptors
```c
//e1000_configure_rx
    for(int i=0;i<RXDESCS;i++){
        rx_desc_array[i].addr = (uint64_t)(kva2pa((uintptr_t)rx_pkt_buffer[i]));
        rx_desc_array[i].length = 0;
        rx_desc_array[i].csum = 0;
        rx_desc_array[i].status = 0;
        rx_desc_array[i].errors = 0;
        rx_desc_array[i].special = 0;
    }
//e1000_poll
    memcpy((uint8_t *)rxbuffer,(uint8_t *)rx_pkt_buffer[new_tail],
                                rx_desc_array[new_tail].length;);
    rx_desc_array[new_tail].length = 0;
    rx_desc_array[new_tail].csum = 0;
    rx_desc_array[new_tail].status = 0;
    rx_desc_array[new_tail].errors = 0;
    rx_desc_array[new_tail].special = 0;
```

---
### Rx descriptors
+ 初始化
  + addr字段设置为相应缓冲区地址
  + 其余字段置0
+ 接收
  + 除addr字段全部清空
  
---
<!--_class: lead-->
# Question 2:
### What is the workflow for handling MAC interrupt?
### Please describe the steps for 
### enabling and handling MAC interrupts,
### as well as the registers you will use 

---
### 网卡中断相关流程
+ 初始化
  + e1000_init
    + 通过置位IMC置零IMS寄存器，读取ICR寄存器将其清零
    + 设置TDH、TDT均为0，RDH、RDT分别为0和RXDESCS-1
    + 置位TCTL和RCTL寄存器允许DMA工作
  + plic_init
    + 关闭其余中断源使能位，使能网卡中断并设置优先级、门限
  + enable_ext_interrupt / enable_preempt
    + 置位SIE寄存器的SEIE位，允许外设中断


---
### 网卡中断相关流程
+ 中断开关设置
  + 禁用 —— 通过置位IMC从而将IMS寄存器置零，屏蔽网卡中断
    + 初始化时禁用，以避免因为TDH=TDT在内核态时进入中断
    + 中断处理时禁用，减少无效中断
      + 例如只进行收包时，因为TDH=TDT反复触发TXQE终端
  + 使能 —— 通过置位IMS寄存器允许相应中断触发
    + 阻塞前使能，以便通过中断唤醒阻塞进程

---
### 网卡中断相关流程
+ 处理过程
  + 触发中断后，`interrupt helper`根据scause判断为外设中断，进入`handle_irq_ext`
  + 调用`plic_claim`获取中断源信息，若为网卡中断源，则进入`net_handle_irq`，否则报错。处理结束后进行`plic_complete`
  + 读取ICR寄存器，获取网卡中断类型并处理
    + 若为TXQE中断，禁用该中断，释放发送阻塞队列
    + 若为RDXCMT0中断，禁用该中断，释放接收阻塞队列
    + 由于IMS寄存器置位只是起到屏蔽某中断的作用，在ICR寄存器中该位可能仍为高，可能需要处理两个中断

---
<!--_class: lead-->
# Question 3:
### How do you check if a packet is received in Task 3?

---
+ 相关结构
  + 一个包被收到，可以表现为DMA将当前持有的描述符status位DD位置高，并将HEAD指针向后移动。
+ 使用场景
  + 对于接收进程/线程而言，将由于未收到包或未收到指定数量的包而阻塞（即tail下一格为head，软件不可继续处理）。则在时间中断/相应网卡中断处理函数中，需要检查是否新收到了包，如是，则可释放接受阻塞队列。
+ 检查方法
  + 根据E1000手册，需要通过查看描述符的STA.DD位判断是否为1，判断是否接受成功。在具体场景中，即tail下一格的STA.DD位是否为1。

---
+ 具体实现
```c
void check_net_recv(){
    local_flush_dcache();
    uint32_t tail = e1000_read_reg(e1000, E1000_RDT);
    uint32_t new_tail = (tail + 1)%RXDESCS;
    if( rx_desc_array[new_tail].status != 0)
    {
        while(recv_block_queue.prev != NULL){
            pcb_t * tmp = list_entry(recv_block_queue.prev,pcb_t,list);
            dequeue(&recv_block_queue);
            do_unblock(&(tmp->list)); //改变状态及入队列
        }
    }
    else
        return ;
}
```