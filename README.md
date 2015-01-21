[![Build Status](https://travis-ci.org/lwch/qtun.svg?branch=master)](https://travis-ci.org/lwch/qtun)

# 网络模型与协议

1. [OSI模型](http://blog.q-devel.com/osi%e6%a8%a1%e5%9e%8b/)
2. [IPV4数据包结构](http://blog.q-devel.com/ipv4%e6%95%b0%e6%8d%ae%e5%8c%85%e7%bb%93%e6%9e%84/)
3. [TCP数据包结构](http://blog.q-devel.com/tcp%e6%95%b0%e6%8d%ae%e5%8c%85%e7%bb%93%e6%9e%84/)
4. [UDP数据包结构](http://blog.q-devel.com/udp%e6%95%b0%e6%8d%ae%e5%8c%85%e7%bb%93%e6%9e%84/)
5. [ICMP数据包结构](http://blog.q-devel.com/icmp%e6%95%b0%e6%8d%ae%e5%8c%85%e7%bb%93%e6%9e%84/)

# Jump The Great Firewall系列

1. [Jump The Great Firewall【step1 创建虚拟网卡】](http://blog.q-devel.com/jump-the-great-firewall-step1/)
2. [Jump The Great Firewall【step2 回应收到的ping包】](http://blog.q-devel.com/jump-the-great-firewall-step2/)
3. [Jump The Great Firewall【step3 建立TCP链接】](http://blog.q-devel.com/jump-the-great-firewall-step3/)
4. [Jump The Great Firewall【step4 包装read/write函数】](http://blog.q-devel.com/jump-the-great-firewall-step4/)
5. [Jump The Great Firewall【step5 简陋的翻墙】](http://blog.q-devel.com/jump-the-great-firewall-step5/)
6. [Jump The Great Firewall【step6 支持多客户端】](http://blog.q-devel.com/jump-the-great-firewall-step6/)
7. [Jump The Great Firewall【step7 支持加密压缩】](http://blog.q-devel.com/jump-the-great-firewall-step7/)
8. [Jump The Great Firewall【step8 系统消息与活跃表】](http://blog.q-devel.com/jump-the-great-firewall-step8/)
9. [Jump The Great Firewall【step9 内存池】](http://blog.q-devel.com/jump-the-great-firewall-step9/)
10. [Jump The Great Firewall【step10 syslog】](http://blog.q-devel.com/jump-the-great-firewall-step10/)
11. [Jump The Great Firewall【step11 数据分片】](http://blog.q-devel.com/jump-the-great-firewall-step11/)

# 部署

本软件只支持`unix/linux`系的操作系统

## server端

1. 下载代码
2. 安装cmake和gcc
3. cd src && mkdir build && cd build
4. cmake -DCMAKE\_BUILD\_TYPE=RELEASE ..
5. make
6. 使用命令`sudo nohup ./step11 -l10.0.1.1 -g 2>&1 1>/dev/null &`运行服务器端程序并绑定到默认端口6687，这里的step11为当前最新版本，这里可以使用-p参数绑定到其他端口
7. 修改/etc/sysctl.conf，增加或修改net.ipv4.ip\_forward = 1，使用命令sysctl -p使其生效
8. 检查iptables中的nat表是否支持tun0网卡转发，若不支持则使用命令sudo iptables -t nat -A POSTROUTING -o tun0使其支持

## client端

1. 下载代码
2. 安装cmake和gcc
3. cd src && mkdir build && cd build
4. cmake -DCMAKE\_BUILD\_TYPE=RELEASE ..
5. 使用命令`sudo nohup ./step11 -l10.0.1.2 -s服务器IP -g 2>&1 1>/dev/null &`连接服务器，这里的step11为当前最新版本
6. 使用命令`sudo route add 服务器IP gw 默认网关`添加到服务器的路由
7. 使用命令`sudo route del -net 0.0.0.0/0 dev eth0`删除默认路由
8. 使用命令`sudo route add -net 0.0.0.0/0 dev tun0`将默认路由指向tun0网卡
9. 使用诸如chnroutes工具创建国内路由到原默认网关

## 可用参数

短命令 | 长命令         | 后接参数     | 默认值               | 说明
------ | -------------- | ------------ | -------------------- | ----------------------------------------------
-a     | --aes          | 密钥文件路径 | 无                   | AES加密，若指定该参数则使用该密钥进行加密通信
-d     | --des          | 密钥文件路径 | 无                   | DES加密，若指定该参数则使用该密钥进行加密通信
-g     | --gzip         | 无           | 无                   | 若指定该参数则使用gzip进行压缩
-m     | --mask         | 掩码长度     | 24                   | 该参数指定了服务端所划分的子网网段
-l     | --localip      | 本地IP地址   | 无                   | 该IP地址被绑定到虚拟网卡tun`n`
-s     | --server       | 服务器IP地址 | 无                   | 该参数指定了服务器的IP地址，若没有指定该参数则当前运行的是服务端
-p     | --port         | 端口号       | 6687                 | 若当前运行的是服务端则为其绑定的端口号，否则为连接到服务器的端口号
-v     | --log-level    | log等级      | LOG\_WARNING对应的值 | 详见syslog.h中定义的值
-t     | --internal-mtu | mtu值        | 1500                 | 内部使用的mtu值，过大的数据包将被分片

# 关于

1. blog：http://blog.q-devel.com/
2. QQ：510134884
3. QQ群：374553828
4. email：lwch748@gmail.com
