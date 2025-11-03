#include "stm32f10x.h"
#include "./systick/bsp_SysTick.h"
#include "./dht11/bsp_dht11.h"
#include "./usart/bsp_usart.h"
#include "./led/bsp_led.h" 
#include "./usart/bsp_TiMbase.h"

#include "stdio.h"
#include "string.h"

#define PRODUCT_ID    "h6NGXZrj48"   // 产品ID
#define DEVICE_NAME   "Test1"         // 设备名单
#define PASSWORD      "version=2018-10-31&res=products%2Fh6NGXZrj48%2Fdevices%2FTest1&et=2062546500&method=md5&sign=RJ%2Fa0j%2B8uqK3buzujxal0w%3D%3D"
//PASSWORD token
// 主题格式
#define TOPIC_FORMAT  "$sys/%s/%s/thing/property/post"	

//将传感器读取的数据转化为小数并上传到服务器
float temp;      //温度
float humi;      //湿度
uint16_t lux=0;    //光照

//已连上服务器，允许读取传感器数据标志位
unsigned int  Start_flag=0,Start_flag1=0;// 启动标志：0-等待 1-执行操作
unsigned int  Contrue_flag=0;            // 连接成功标志（控制LED开）
unsigned int  Conflas_flag=0;            // 连接失败标志（控制LED关）

	
unsigned int Humi_int=0;       // 湿度整数部分
unsigned int Humi_deci=0;      // 湿度小数部分
unsigned int Temp_int=0;
unsigned int Temp_deci=0;

//传感器读取的数据结果放大10倍	
uint16_t temperature = 0;      // 温度值（放大10倍）
uint16_t humidity = 0;
	
 char buffer[64];        // 通用数据缓冲区
char led_flag=0;         // LED状态标志

uint8_t connect_packet[128];  // 定义CONNECT报文缓冲区
uint16_t connect_len=0;       //构建connect报文

uint8_t subscribe_packet[128];//定义subscribe报文缓冲区
 uint16_t subscribe_len=0;    //构建subscribe报文 
 
 /**
  * @brief  将整型数据转换为浮点型
  * @param  value: 放大10倍的整型值
  * @retval 转换后的浮点数值
  */
//将整型数据转化为浮点型
float intToFloat(int value) 
	{
    return (float)value / 10.0f;  // 除以10恢复原始精度
}
	/**
  * @brief  MQTT协议剩余长度编码
  * @param  buf: 存储编码结果的缓冲区
  * @param  len: 需要编码的长度值
  * @retval 编码后的字节数
  */
//可变字节编码剩余长度
uint8_t encodeRemainingLength(uint8_t *buf, uint32_t len) {
    uint8_t digit, pos = 0;
    do {
        digit = len % 128;         // 取低7位
        len /= 128;                // 右移7位
        if (len > 0) digit |= 0x80;// 设置最高位表示还有后续字节
        buf[pos++] = digit;        // 存储编码字节
    } while (len > 0 && pos < 4);  // 最多4字节
    return pos;                    // 返回编码字节数
}

/**
  * @brief  构建MQTT CONNECT报文
  * @param  packet: 报文存储缓冲区
  * @param  client_id: 客户端ID
  * @param  username: 用户名（产品ID）
  * @param  password: 密码（鉴权令牌）
  * @retval 报文总长度
  */
//构建CONNECT报文
uint16_t buildMqttConnectPacket(uint8_t *packet, const char *client_id, const char *username, const char *password)
{
    uint16_t pos = 0;
	  uint32_t remaining_len=0;
	  uint8_t len_buf[4];
	  uint8_t len_bytes=0;
	
    packet[pos++] = 0x10; // CONNECT类型
    //计算剩余长度
    remaining_len = 2 + 4 + 1 + 1 + 2 + 2 + strlen(client_id) + 2 + strlen(username) + 2 + strlen(password);
	
    //编码剩余长度
    len_bytes = encodeRemainingLength(len_buf, remaining_len);
    memcpy(&packet[pos], len_buf, len_bytes);
    pos += len_bytes;

    //协议名"MQTT"
    packet[pos++] = 0x00; 
	  packet[pos++] = 0x04;
    memcpy(&packet[pos], "MQTT", 4); 
	  pos += 4;

    //协议级别、连接标志
    packet[pos++] = 0x04; // MQTT 3.1.1
    packet[pos++] = 0xC2; // Clean Session
    packet[pos++] = 0x00; 
		packet[pos++] = 0x64; // 100秒

    //客户端ID
    packet[pos++] = (strlen(client_id) >> 8) & 0xFF;
    packet[pos++] = strlen(client_id) & 0xFF;
    memcpy(&packet[pos], client_id, strlen(client_id)); pos += strlen(client_id);

    //用户名和密码
    packet[pos++] = (strlen(username) >> 8) & 0xFF;
    packet[pos++] = strlen(username) & 0xFF;
    memcpy(&packet[pos], username, strlen(username)); pos += strlen(username);

    packet[pos++] = (strlen(password) >> 8) & 0xFF;
    packet[pos++] = strlen(password) & 0xFF;
    memcpy(&packet[pos], password, strlen(password)); pos += strlen(password);

    return pos;
}
/**
  * @brief  构建MQTT PUBLISH报文
  * @param  packet: 报文存储缓冲区
  * @param  topic: 主题名称
  * @param  payload: 消息内容(JSON格式)
  * @retval 报文总长度
  */
//构建PUBLISH报文
uint16_t buildMqttPublishPacket(uint8_t *packet, const char *topic, const char *payload) 
{
    uint16_t pos = 0;
	  uint32_t remaining_len=0;
	  uint8_t len_bytes=0;
	  uint8_t len_buf[4];
	
    packet[pos++] = 0x30; // PUBLISH QoS=0

    //计算剩余长度
    remaining_len = 2 + strlen(topic) + strlen(payload);
    
    //编码剩余长度    
    len_bytes = encodeRemainingLength(len_buf, remaining_len);
    memcpy(&packet[pos], len_buf, len_bytes);
    pos += len_bytes;

    //主题
    packet[pos++] = (strlen(topic) >> 8) & 0xFF;
    packet[pos++] = strlen(topic) & 0xFF;
    memcpy(&packet[pos], topic, strlen(topic)); pos += strlen(topic);

    //有效载荷
    memcpy(&packet[pos], payload, strlen(payload)); pos += strlen(payload);

    return pos;
}
/**
  * @brief  上报传感器数据到OneNET平台
  */
void reportSensorData() 
	{
    char topic[128];
		char json[256];
		uint8_t publish_packet[512];
		uint16_t publish_len=0;
    sprintf(topic, TOPIC_FORMAT, PRODUCT_ID, DEVICE_NAME); // 生成完2整主题

    sprintf(json, 
		      "{\"id\":\"123\",\"params\":{"
		      "\"humi\":{\"value\":%.1f},"
		      "\"temp\":{\"value\":%.1f}"
		      "}}", humi, temp);

    publish_len = buildMqttPublishPacket(publish_packet, topic, json);
    USART3_SendData(publish_packet, publish_len);
}

/**
  * @brief  构建MQTT SUBSCRIBE报文
  * @param  packet: 报文存储缓冲区
  * @retval 报文总长度
  */
//构建SUBSCRIBE报文(订阅属性设置主题)
uint16_t buildSubscribePacket(uint8_t *packet) 
{
    uint16_t pos = 0;
	  uint32_t remaining_len=0;
	  const char *topic = "$sys/"PRODUCT_ID"/"DEVICE_NAME"/thing/property/set";
    packet[pos++] = 0x82; // SUBSCRIBE类型(QoS=0)

    // 剩余长度（后续字节数）
    remaining_len = 2 + 2 + strlen("$sys/"PRODUCT_ID"/"DEVICE_NAME"/thing/property/set") + 1;
    pos += encodeRemainingLength(&packet[pos], remaining_len);

    // Packet Identifier(随机值)
    packet[pos++] = 0x00; packet[pos++] = 0x01;

    //主题名称    
    packet[pos++] = (strlen(topic) >> 8) & 0xFF;
    packet[pos++] = strlen(topic) & 0xFF;
    memcpy(&packet[pos], topic, strlen(topic)); pos += strlen(topic);

    // QoS级别(1)
    packet[pos++] = 0x00;

    return pos;
}



/**
  * @brief  向ESP8266发送AT指令
  * @param  cmd: 要发送的AT指令
  */
void ESP8266_SendCmd(char *cmd) 
	{
    Usart_SendString(USART3,cmd);
  }
/**
  * @brief  ESP8266初始化配置
  */
void ESP8266_Init(void) 
	{
		Delay_ms(5000);
    // 1. 测试AT命令
    ESP8266_SendCmd("AT\r\n");
		Delay_ms(3000);
    // 2. 设置WIFI模式(Station)
    ESP8266_SendCmd("AT+CWMODE=1\r\n");
		Delay_ms(3000);
    // 3. 连接WiFi
    //ESP8266_SendCmd("AT+CWJAP=\"huanghuai\",\"12345678\"\r\n");
		ESP8266_SendCmd("AT+CWJAP=\"iQOO10\",\"16627891336.\"\r\n");
		Delay_ms(5000);
		Delay_ms(5000);
    // 4. 连接TCP服务器
    ESP8266_SendCmd("AT+CIPSTART=\"TCP\",\"mqtts.heclouds.com\",1883\r\n");
		Delay_ms(5000);
		Delay_ms(5000);
    // 5. 启用透传模式
		ESP8266_SendCmd("AT+CIPMODE=1\r\n");
		Delay_ms(5000);
		// 6. 进入透传发送模式
    ESP8266_SendCmd("AT+CIPSEND\r\n");
		Delay_ms(5000);

}
/**
  * @brief  主函数
  * @param  无  
  * @retval 无
  */
int main(void)
{
	DHT11_Data_TypeDef DHT11_Data;
	
	/* 配置SysTick 为1us中断一次 */
	SysTick_Init();
	USART_Config();//初始化串口1
	USART3_Config();
	/*初始化DTT11的引脚*/
	BASIC_TIM_Init(); // 初始化基础定时器
	ESP8266_Init();
	DHT11_Init ();// 初始化DHT11传感器
	LED_GPIO_Config(); // 初始化LED控制引脚
//连接MQTT服务器
  connect_len = buildMqttConnectPacket(connect_packet, DEVICE_NAME, PRODUCT_ID, PASSWORD);
  USART3_SendData(connect_packet, connect_len);
	led_flag=1;
	while(1)
	{	
			if(Start_flag==1)
		{
		//订阅属性报文
		subscribe_len = buildSubscribePacket(subscribe_packet);
		USART3_SendData(subscribe_packet, subscribe_len);
		Usart_SendString(USART1,"The Drive Connet Onenet.\r\n");	
		Delay_ms(2000);
		Start_flag=0;	
		}
	
			/*调用DHT11_Read_TempAndHumidity读取温湿度，若成功则输出该信息*/
			if( DHT11_Read_TempAndHumidity ( & DHT11_Data ) == SUCCESS&&Start_flag1==1)//if( DHT11_Data.humi_deci)
			{
					sprintf(buffer, "%d.%dRH%d.%dC\r\n", 
					DHT11_Data.humi_int, DHT11_Data.humi_deci, 
					DHT11_Data.temp_int, DHT11_Data.temp_deci);
		      /*湿度数据*/		
				  Humi_deci=DHT11_Data.humi_deci;
					humidity=DHT11_Data.humi_int*10+Humi_deci;
				  /*温度数据*/
					Temp_deci=DHT11_Data.temp_deci;
					temperature=DHT11_Data.temp_int*10+Temp_deci;

						//数据上传到onenet中，动态数据上报函数
						temp=intToFloat(temperature);
						humi=intToFloat(humidity);
						reportSensorData();	
            Usart_SendString(USART1,"The Drive Send data to Onenet.\r\n");	
			    /*读取湿度并转换*/	
	        Start_flag1=0;
			}		
        /*LED2_OFF，PC0输出高电平*/
        if(Contrue_flag==1)
				{
        LED2_ON;									
				Contrue_flag=0;
				Usart_SendString(USART1,"Open Drive!\r\n");
				}	
				/*LED2_OFF，PC0输出低电平*/
        if(Conflas_flag==1)
				{		
				LED2_OFF;			
				Conflas_flag=0;
				Usart_SendString(USART1,"Close Drive!\r\n");
				}					
	}

}
/*********************************************END OF FILE**********************/
