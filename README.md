# ESP32-Remote-Bulb

> 本项目是一个简单的 基于ESP32和AWS Iot Pass实现一个板子上的Button控制另一个板子的LED的IoT项目

主要思路是通过button push 消息到主题 "esp32Button/switch"，同时LED通过订阅该主题，通过GPIO对LED进行点亮和熄灭，再使用device shadow做同步。

## 项目结构
本Project使用ESP-IDF开发，由**led**、**button**两部分组成，每部分的*certs*文件夹下面放置AWS IoT的私钥、根证书、证书。

## Set Up

1. 下载ESP-IDF
2. 下载esp-iot-solution
3. 配置环境变量:

`
 export IOT_SOLUTION_PATH=$HOME/Workspace/IoT/esp-iot-solution
 export IOT_CLIENT_DIR=$HOME/Workspace/IoT/AWS-IoT-C-SDK
`

4.编译命令：`make`; 烧录命令: `make flash`
5. USB串口驱动

 - 安装CP210* USB to UART Bridge Controller驱动：https://www.silabs.com/documents/public/software/Mac_OSX_VCP_Driver.zip

 - Make flash 烧录；make monitor查看串口输出,/dev/tty.SLAB_USBtoUART

