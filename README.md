# Temperature-and-humidity-monitoring-system
1、系统采用DHT11数字温湿度传感器采集环境数据，通过MQTT协议将数据传输至中国移动OneNET物联网云平台。 2、软件设计实现传感器驱动、ESP01s（ESP8266） AT指令控制、MQTT协议栈集成和数据加密传输。 3、系统能够在5秒内完成数据采集与上传，温度测量误差±1℃，湿度误差±5%RH，网络断线后可自动重连。
The system uses DHT11 to collect environmental data, is controlled by ESP01sAT commands, and transmits the data to the OneNET Internet of Things cloud platform via the MQTT protocol.
