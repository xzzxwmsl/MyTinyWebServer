# MyTinyWebServer

## 参考
游双 Linux高性能服务器编程

qingguoyi https://github.com/qinguoyi/TinyWebServer

## 事件处理模式
### Reactor
主线程负责监听文件描述符上的事件，当有事件就绪时，通知工作线程，由工作线程完成数据的读写、接受新连接、处理事件请求等。一般由同步模式完成。

### Proactor
主线程负责监听文件描述符上的事件，并负责接受新连接、读写等任务，工作线程负责处理任务逻辑即可。通常由异步IO完成

### 同步模拟的Proactor
* 主线程向epoll内核事件表中注册socket上的读就绪事件
* 主线程调用epoll_wait等待socket上有数据可读
* 当socket上有数据可读时，epoll_wait会通知主线程，然后主线程从该socket上循环读取数据直到没有数据可读。将该数据封装成一个对象插入到请求队列上
* 睡眠在请求队列上的某个线程获得处理权，处理该请求，向epoll_wait上注册该socket写就绪事件
* 主线程调用epoll_wait等待该socket可写
* 当socket上数据可写时，epoll_wait通知主线程，主线程向该socket写工作线程处理客户请求的结果数据


## 并发编程模式：半同步半反应堆

### 半同步/半异步
* 同步线程：处理客户逻辑  
* 异步线程：监听IO事件  
* 当异步线程监听到客户请求后，将其封装成对象并插入请求队列
* 请求队列通知某个工作在**同步模式**下的工作线程来读取并处理对象

### 半同步/半反应堆
是上述模式的变体，将半异步化具体为某种事件处理模式
* 主线程充当异步线程，负责监听所有的socket上的事件
* 当有新的请求到来，主线程接受该请求并向epoll内核事件表中注册该请求的读写事件
* 如果连接的socket上有读写事件发生，主线程从socket上接收数据，并将数据封装成请求对象插入到请求队列中
* 所有的工作线程睡眠在请求队列上，通过某种竞争机制获取处理权