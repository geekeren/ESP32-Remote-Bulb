# ESP32-Remote-Bulb

> 本项目是一个简单的 基于ESP32和AWS Iot Pass实现一个板子上的Button控制另一个板子的LED的IoT项目

主要思路是通过button push 消息到主题 "esp32Button/switch"，同时LED通过订阅该主题，通过GPIO对LED进行点亮和熄灭，再使用device shadow做同步。

## ESP32
