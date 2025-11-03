
#include "./usart/bsp_usart.h"
#include "./led/bsp_led.h" 
extern char led_flag;
extern unsigned int  Start_flag;
extern unsigned int  Contrue_flag;
extern unsigned int  Conflas_flag;
////串口接收缓存区 	
//u16 point2 = 0;
//u8 Send_slave[53]={0};//本地的气象数据
//服务器返回的数据
u16 MQTT_Connect_receiv[4]={0}; //收到的数据，Connect已连接四个字节
u16 MQTT_Set_true[4]={0}; //收到的数据，收到的控制数据为真
u16 MQTT_Set_fals[4]={0}; //收到的数据，收到的控制数据为假
u8  MQTT_Connect_cnt=0,MQTT_Set_true_cnt=0,MQTT_Set_fals_cnt=0;//数组计数

 /**
  * @brief  USART GPIO 配置,工作参数配置
  * @param  无
  * @retval 无
  */
	
	static void NVIC_Configuration(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;
  
  /* 嵌套向量中断控制器组选择 */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
		/* 配置USART为中断源 */
  NVIC_InitStructure.NVIC_IRQChannel = DEBUG_USART3_IRQ;
  /* 抢断优先级*/
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  /* 子优先级 */
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  /* 使能中断 */
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  /* 初始化配置NVIC */
  NVIC_Init(&NVIC_InitStructure);
	

}

void USART_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	// 打开串口GPIO的时钟
	DEBUG_USART_GPIO_APBxClkCmd(DEBUG_USART_GPIO_CLK, ENABLE);
	
	// 打开串口外设的时钟
	DEBUG_USART_APBxClkCmd(DEBUG_USART_CLK, ENABLE);

	// 将USART Tx的GPIO配置为推挽复用模式
	GPIO_InitStructure.GPIO_Pin = DEBUG_USART_TX_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(DEBUG_USART_TX_GPIO_PORT, &GPIO_InitStructure);

  // 将USART Rx的GPIO配置为浮空输入模式
	GPIO_InitStructure.GPIO_Pin = DEBUG_USART_RX_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(DEBUG_USART_RX_GPIO_PORT, &GPIO_InitStructure);
	
	// 配置串口的工作参数
	// 配置波特率
	USART_InitStructure.USART_BaudRate = DEBUG_USART_BAUDRATE;
	// 配置 针数据字长
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	// 配置停止位
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	// 配置校验位
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	// 配置硬件流控制
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	// 配置工作模式，收发一起
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	// 完成串口的初始化配置
	USART_Init(DEBUG_USARTx, &USART_InitStructure);

	// 使能串口
	USART_Cmd(DEBUG_USARTx, ENABLE);	    
}

void USART3_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	// 打开串口GPIO的时钟
	DEBUG_USART3_GPIO_APBxClkCmd(DEBUG_USART3_GPIO_CLK, ENABLE);
	
	// 打开串口外设的时钟
	DEBUG_USART3_APBxClkCmd(DEBUG_USART3_CLK, ENABLE);

	// 将USART Tx的GPIO配置为推挽复用模式
	GPIO_InitStructure.GPIO_Pin = DEBUG_USART3_TX_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(DEBUG_USART3_TX_GPIO_PORT, &GPIO_InitStructure);

  // 将USART Rx的GPIO配置为浮空输入模式
	GPIO_InitStructure.GPIO_Pin = DEBUG_USART3_RX_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(DEBUG_USART3_RX_GPIO_PORT, &GPIO_InitStructure);
	
	// 配置串口的工作参数
	// 配置波特率
	USART_InitStructure.USART_BaudRate = DEBUG_USART3_BAUDRATE;
	// 配置 针数据字长
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	// 配置停止位
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	// 配置校验位
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	// 配置硬件流控制
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	// 配置工作模式，收发一起
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	// 完成串口的初始化配置
	USART_Init(DEBUG_USART3x, &USART_InitStructure);

	
	// 串口中断优先级配置
	NVIC_Configuration();
	
	// 使能串口接收中断
	USART_ITConfig(DEBUG_USART3x, USART_IT_RXNE, ENABLE);	
	
	// 使能串口
	USART_Cmd(DEBUG_USART3x, ENABLE);		  
}

///重定向c库函数printf到串口，重定向后可使用printf函数
int fputc(int ch, FILE *f)
{
		/* 发送一个字节数据到串口 */
		USART_SendData(DEBUG_USARTx, (uint8_t) ch);
		
		/* 等待发送完毕 */
		while (USART_GetFlagStatus(DEBUG_USARTx, USART_FLAG_TXE) == RESET);		
	
		return (ch);
}

///重定向c库函数scanf到串口，重写向后可使用scanf、getchar等函数
int fgetc(FILE *f)
{
		/* 等待串口输入数据 */
		while (USART_GetFlagStatus(DEBUG_USARTx, USART_FLAG_RXNE) == RESET);

		return (int)USART_ReceiveData(DEBUG_USARTx);
}

/*****************  发送一个字节 **********************/
void Usart_SendByte( USART_TypeDef * pUSARTx, uint8_t ch)
{
	/* 发送一个字节数据到USART */
	USART_SendData(pUSARTx,ch);
		
	/* 等待发送数据寄存器为空 */
	while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);	
}

/*****************  发送字符串 **********************/
void Usart_SendString( USART_TypeDef * pUSARTx, char *str)
{
	unsigned int k=0;
  do 
  {
      Usart_SendByte( pUSARTx, *(str + k) );
      k++;
  } while(*(str + k)!='\0');
  
  /* 等待发送完成 */
  while(USART_GetFlagStatus(pUSARTx,USART_FLAG_TC)==RESET)
  {}
}

/*这个代码是把组成的数据发送到服务器中*/
void USART3_SendData(uint8_t *data, uint16_t len) 
{
	unsigned int i=0;
  for (i = 0; i < len; i++) 
		{ // 遍历数据缓冲区
    USART_SendData(USART3, data[i]);  // 发送一个字节数据
    while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);  // 等待发送完成
    }
}


//void DEBUG_USART3_IRQHandler(void) 
//{
//  uint8_t ucTemp;
//	if(USART_GetITStatus(DEBUG_USART3x,USART_IT_RXNE)!=RESET)
//	{		
//		ucTemp = USART_ReceiveData(DEBUG_USART3x);
//    USART_SendData(DEBUG_USARTx,ucTemp); 
//    if(led_flag==1&&ucTemp==0x01)	
//		{
//		GPIO_ResetBits(LED1_GPIO_PORT, LED1_GPIO_PIN);
//		GPIO_SetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);	 
//		GPIO_SetBits(LED3_GPIO_PORT, LED3_GPIO_PIN);
//		}	
//    if(led_flag==1&&ucTemp==0x02)	
//		{
//		GPIO_SetBits(LED1_GPIO_PORT, LED1_GPIO_PIN);
//		GPIO_ResetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);	 
//		GPIO_SetBits(LED3_GPIO_PORT, LED3_GPIO_PIN);
//		}	
//    if(led_flag==1&&ucTemp==0x03)	
//		{
//		GPIO_SetBits(LED1_GPIO_PORT, LED1_GPIO_PIN);
//		GPIO_SetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);	 
//		GPIO_ResetBits(LED3_GPIO_PORT, LED3_GPIO_PIN);
//		}		
//    if(led_flag==1&&ucTemp==0x04)	
//		{
//		GPIO_SetBits(LED1_GPIO_PORT, LED1_GPIO_PIN);
//		GPIO_SetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);	 
//		GPIO_SetBits(LED3_GPIO_PORT, LED3_GPIO_PIN);
//		}		
//	}	 
//}


// 串口中断服务函数 CAT1-4G
void DEBUG_USART3_IRQHandler(void) //返回的是字节数
{
  uint8_t ucTemp;
	if(USART_GetITStatus(DEBUG_USART3x,USART_IT_RXNE)!=RESET)
	{	
		ucTemp = USART_ReceiveData(DEBUG_USART3x);
    USART_SendData(DEBUG_USARTx,ucTemp);
    MQTT_Connect_receiv[MQTT_Connect_cnt] = USART_ReceiveData(DEBUG_USART3x);
		MQTT_Set_true[MQTT_Set_true_cnt] = USART_ReceiveData(DEBUG_USART3x);
		MQTT_Set_fals[MQTT_Set_fals_cnt] = USART_ReceiveData(DEBUG_USART3x);
		/*判断服务器返回的字节，是否已连接，如果已连接才允许数据上传onenet*/
		if(MQTT_Connect_cnt==0&&MQTT_Connect_receiv[0]==0x20)       
		{  
			MQTT_Connect_cnt++; 
		}
        else if(MQTT_Connect_cnt==1&&MQTT_Connect_receiv[1]==0x02)       
		 {
		   MQTT_Connect_cnt++;	 		 
		 }
		else if(MQTT_Connect_cnt==2&&MQTT_Connect_receiv[2]==0x00) 
		{  
		   MQTT_Connect_cnt=0; 
		   Start_flag=1;
        }
		else 
		{  
		}
		/*判断服务器返回的字节，是否为控制数据，开启true*/
		if(MQTT_Set_true_cnt==0&&MQTT_Set_true[0]==0x74)       
		{  
			MQTT_Set_true_cnt++; 
		}
        else if(MQTT_Set_true_cnt==1&&MQTT_Set_true[1]==0x72)       
		 {
		   MQTT_Set_true_cnt++;	 		 
		 }
		else if(MQTT_Set_true_cnt==2&&MQTT_Set_true[2]==0x75) 
		{  
		   MQTT_Set_true_cnt=0; 
		   Contrue_flag=1;
        }
		else 
		{  
		}
		/*判断服务器返回的字节，是否为控制数据，开启false*/
		if(MQTT_Set_fals_cnt==0&&MQTT_Set_fals[0]==0x66)       
		{  
			MQTT_Set_fals_cnt++; 
		}
        else if(MQTT_Set_fals_cnt==1&&MQTT_Set_fals[1]==0x61)       
		 {
		   MQTT_Set_fals_cnt++;	 		 
		 }
		else if(MQTT_Set_fals_cnt==2&&MQTT_Set_fals[2]==0x6C) 
		{  
		   MQTT_Set_fals_cnt=0; 
		   Conflas_flag=1;
        }
		else 
		{  
		}
		
	}	 
}

