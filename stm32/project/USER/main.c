/************************************************
 神舟号 STM32F407开发板 
 安徽中科康芯科技有限公司  
 功能：多功能传感器数据采集与回传
 串口1(PA9/PA10): 调试输出(printf)
 串口2(PA2/PA3): MKS141血压/心率/血氧传感器(38400)
 串口3(PB10/PB11): 与ARM平台通信(115200)
 
 ARM指令格式:
   "mks-bp\n"    → 返回 "SBP\n"       (收缩压)
   "mks-sbp\n"   → 返回 "SBP\n"       (收缩压)
   "mks-dbp\n"   → 返回 "DBP\n"       (舒张压)
   "mks-hr\n"    → 返回 "HR\n"        (心率)
   "mks-spo2\n"  → 返回 "SPO2\n"      (血氧)
   "mks-all\n"   → 返回 "HR,SBP,DBP,SPO2\n" (全部)
   "asm-temp\n"  → 返回 "temp\n"      (体温)
************************************************/
#include "main.h"

extern u8 USART3_RX_BUF[];
extern u16 USART3_RX_STA;

void Program_Init(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	Usart1_Init(9600);           // 调试串口
	AS6221_IIC_Init();           // 温度传感器
	MKS141_Usart2_Init(38400);   // 心率血氧血压模块
	Usart3_Init(115200);         // 与ARM通信
}

int main(void)
{
	MKS141_Date mk;
	char reply_buf[32];
	char cmd_buf[80];
	int timeout;
	int got_data;

	Program_Init();
	printf("=== Sensor Hub Ready ===\n");

	while (1)
	{
		if (USART3_RX_STA)
		{
			// 先拷贝指令，再清接收缓冲区
			memset(cmd_buf, 0, sizeof(cmd_buf));
			strcpy(cmd_buf, (char *)USART3_RX_BUF);
			USART3_RX_STA = 0;
			memset(USART3_RX_BUF, 0, sizeof(USART3_RX_BUF));
			printf("ARM CMD: %s\n", cmd_buf);

			// ========== 体温 ==========
			if (contains_substring((uint8_t *)cmd_buf, "asm-temp"))
			{
				// 等待传感器接触人体（>31℃即为接触，传感器偏低3℃），最多30秒
				timeout = 30;
				got_data = 0;
				float t = 0;
				while (timeout > 0)
				{
					t = AS6221_GetTemperature();
					if (t > 31.0 && t < 45)  // 传感器偏低3℃，31℃≈真实34℃，表示已接触
					{
						got_data = 1;
						break;
					}
					delay_s(1);
					timeout--;
				}
				// 接触后等5秒让温度升到体温稳定，然后补偿+3℃发送
				if (got_data)
				{
					delay_s(5);
					t = AS6221_GetTemperature() + 3.0;  // 传感器偏差补偿
				}
				sprintf(reply_buf, "%.1f\n", got_data ? t : 0.0);
				Usart_SendString(USART3, reply_buf);
				printf("TEMP: %.1f -> ARM\n", got_data ? t : 0.0);
			}
			// ========== 收缩压 ==========
			else if (contains_substring((uint8_t *)cmd_buf, "mks-sbp") ||
					 (contains_substring((uint8_t *)cmd_buf, "mks-bp") &&
					  !contains_substring((uint8_t *)cmd_buf, "dbp") &&
					  !contains_substring((uint8_t *)cmd_buf, "hr")  &&
					  !contains_substring((uint8_t *)cmd_buf, "spo2")))
			{
				timeout = 80;
				got_data = 0;
				while (timeout > 0)
				{
					mk = Get_MKS141_Date();
					if (mk.Head == 0xFF && mk.SBP > 0)
					{
						got_data = 1;
						break;
					}
					delay_s(1);
					timeout--;
				}
				sprintf(reply_buf, "%d\n", got_data ? mk.SBP : 0);
				Usart_SendString(USART3, reply_buf);
				printf("SBP: %s", reply_buf);
			}
			// ========== 舒张压 ==========
			else if (contains_substring((uint8_t *)cmd_buf, "mks-dbp"))
			{
				timeout = 80;
				got_data = 0;
				while (timeout > 0)
				{
					mk = Get_MKS141_Date();
					if (mk.Head == 0xFF && mk.DBP > 0)
					{
						got_data = 1;
						break;
					}
					delay_s(1);
					timeout--;
				}
				sprintf(reply_buf, "%d\n", got_data ? mk.DBP : 0);
				Usart_SendString(USART3, reply_buf);
				printf("DBP: %s", reply_buf);
			}
			// ========== 血氧 ==========
			else if (contains_substring((uint8_t *)cmd_buf, "mks-spo2"))
			{
				timeout = 50;
				got_data = 0;
				while (timeout > 0)
				{
					mk = Get_MKS141_Date();
					if (mk.Head == 0xFF && mk.SPO2 > 0)
					{
						got_data = 1;
						break;
					}
					delay_s(1);
					timeout--;
				}
				sprintf(reply_buf, "%d\n", got_data ? mk.SPO2 : 0);
				Usart_SendString(USART3, reply_buf);
				printf("SPO2: %s", reply_buf);
			}
			// ========== 心率 ==========
			else if (contains_substring((uint8_t *)cmd_buf, "mks-hr"))
			{
				timeout = 50;
				got_data = 0;
				while (timeout > 0)
				{
					mk = Get_MKS141_Date();
					if (mk.Head == 0xFF && mk.HR > 0)
					{
						got_data = 1;
						break;
					}
					delay_s(1);
					timeout--;
				}
				sprintf(reply_buf, "%d\n", got_data ? mk.HR : 0);
				Usart_SendString(USART3, reply_buf);
				printf("HR: %s", reply_buf);
			}
			// ========== 综合(HR,SBP,DBP,SPO2) ==========
			else if (contains_substring((uint8_t *)cmd_buf, "mks-all"))
			{
				timeout = 80;
				got_data = 0;
				while (timeout > 0)
				{
					mk = Get_MKS141_Date();
					if (mk.Head == 0xFF && mk.HR > 0 && mk.SBP > 0)
					{
						got_data = 1;
						break;
					}
					delay_s(1);
					timeout--;
				}
				if (got_data)
					sprintf(reply_buf, "%d,%d,%d,%d\n", mk.HR, mk.SBP, mk.DBP, mk.SPO2);
				else
					sprintf(reply_buf, "0,0,0,0\n");
				Usart_SendString(USART3, reply_buf);
				printf("ALL: %s", reply_buf);
			}
			else
			{
				printf("Unknown CMD: %s\n", cmd_buf);
			}
		}

		delay_s(1);
	}
}
