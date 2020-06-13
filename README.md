# chatBoxes
计算机网络课程作业，基于linux底层socket接口实现一个聊天软件，包括sever和client

## 运行环境

* 操作系统：linux
* 运行库：pthread ,ncurses
* 编译/调试器：gcc/gcc-c++，gdb

## 运行界面

server端：

![server interface](https://raw.githubusercontent.com/a-psycho/chatBoxes/master/images/server_run.png)

client端：

![client interface](https://raw.githubusercontent.com/a-psycho/chatBoxes/master/images/client_run.png)

## 源码说明

### src/mysocket.cpp & include/mysocket.h

用c++类将linux底层socket接口进行简单的封装，为了client/server代码编写时简化工作。

类似与其他面向对象的网络库（如python），封装好了使用流程就是，服务端：socket --> bind --> listen -> accept --> read/write --> close；客户端：socket --> bind --> connect --> read/write --> close;

### include/myprotocol.h

定义了server/client之间通信时应采用的简单协议。对通信包的头部，长度，有效位，payload进行了定义。

### src/server.cpp

server端的完整实现，只有字符界面。主线程接收到一个请求便创建一个线程为其服务。较为难处理的地方在于共享资源的加解锁操作，这部分出问题要花大量时间多线程调试；还有用户上线下线时要通知给所有用户，如果此时有人给已经下线的用户发消息，应该如何处理等等，这部分冲突问题要结合底层socket的一些细节进行处理。

### src/client.cpp

client端的完整实现，有字符图像界面（使用[ncurses](http://tldp.org/HOWTO/NCURSES-Programming-HOWTO/intro.html)库进行实现）,界面设计虽然很简单，但也花了很多时间编写和调试。共两个线程，一个用来接收服务端数据并显现在消息框，一个用来接收用户输入进行发送信息等操作，发送的消息也要在消息框显示，所以也存在共享资源的加解锁操作。









