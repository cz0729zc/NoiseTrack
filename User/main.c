#include "stm32f10x.h"
#include "Delay.h"
#include "LCD12864.h"
#include "Lcd12864Char.h"
#include "LM2904.h"
#include "Beep.h"
#include "24c02.h"
#include "iic.h"
#include "keys.h"
#include "WH_4G.h"
#include "usart1.h"
#include "usart2.h"
#include "usart3.h"
#include "timer4.h"

#define STORAGE_INTERVAL 1      // 存储间隔1秒
#define MAX_STORAGE_ADDR 256    // 24C02最大地址
#define DATA_HEADER 0xA5        // 数据头标识
#define MAX_HISTORY_ITEMS 20    // 最大显示历史记录数
#define THRESHOLD_STEP 3.0      // 阈值调整步长

// 存储数据结构
typedef struct {
    uint8_t header;         // 1字节头标识
    float noise_value;      // 4字节噪音值
    uint32_t timestamp;     // 4字节时间戳(单位秒)
} StorageData;

uint16_t storageAddr = 0;    // 当前存储地址
float noise_threshold = 80.0; // 噪音阈值
uint32_t system_time = 0;    // 系统运行时间(秒)

uint8_t view_history_mode = 0; // 0:正常模式 1:查看历史模式
uint16_t history_index = 0; // 当前查看的历史记录索引
StorageData history_data[MAX_HISTORY_ITEMS]; // 历史数据缓存

// 函数声明
void ReadAllHistoryData(void);
void DisplayHistoryData(uint16_t index);
void DisplayMainScreen(void);
void StoreCurrentData(float noise);

int main(void)
{
//    /* 模块初始化 */
//    LCD_Init();            // LCD初始化
//    IIC_Init();             // IIC初始化
//    LM2904_Init();          // 噪音传感器初始
//    Usart3_Init(9600);
//	WH_4G_Config();
//    BEEP_Init();            // 蜂鸣器初始化
//    Key_Init();             // 按键初始化
//    
//    /* 初始化TIM4定时器(1秒中断) */
//    TIM4_Init(9999, 7199); // 10kHz计数频率，1秒中断
//    
//    /* 24C02初始化检测 */
//    AT24C02_WriteOneByte(0, DATA_HEADER);
//    if(AT24C02_ReadOneByte(0) != DATA_HEADER) {
//        u3_printf("24C02 Error!\r\n");
//        while(1);
//    }
//    else{
//        u3_printf("24C02 OK!\r\n");
//    }
//    
//    /* 显示主界面 */
//    DisplayMainScreen();
//    TIM_Cmd(TIM4, ENABLE); // 启动TIM4
    
	InitLCD_12864();
	ClearDot_12864();//清除所有点
    while (1) 
    {
		WriteCommand_12864(0x34);
        DrawDot_12864(32, 6, 10); // 在(0,0)处画点
		WriteCommand_12864(0x36);
		//DrawDot_Picture();
		
//		//u3_printf("24C02 OK!\r\n");
//		Key_Value key = Key_Scan(); // 扫描按键
//        
//        if(!view_history_mode) // 正常模式
//        {
//            /* 读取并显示噪音值 */
//            float noise = ConvertToDecibel(LM2904_ReadValue());
//            //u3_printf("noise:%.2f dB\r\n", noise);
//            LCD_ShowNum(3, 0, (uint16_t)noise, 3);
//            
//            /* 显示阈值 */
//            LCD_ShowString(0, 1, "Thrsh:");
//            LCD_ShowNum(3, 1, (uint16_t)noise_threshold, 3);
//            
//            /* 噪音报警 */
//            if(noise > noise_threshold) {
//                LCD_ShowString(0, 3, "Warn!");
//				//Wire4G_yuzhiData(0x00,(uint16_t)noise);
//                BEEP_On();
//            } else {
//                LCD_ShowString(0, 3, "Safe ");
//                BEEP_Off();
//            }
//            
//            /* 定时存储数据 */
//            if(timer4_flag) {
//                timer4_flag = 0;
//                StoreCurrentData(noise);
//				Wire4G_sendData(0x00,(uint16_t)noise);
//            }
//            
//            /* 按键处理 */
//            switch(key)
//            {
//                case KEY_ADD: // 增加阈值
//                    noise_threshold += THRESHOLD_STEP;
//                    if(noise_threshold > 120.0) noise_threshold = 120.0;
//                    LCD_ShowNum(3, 1, (uint16_t)noise_threshold, 3);
//                    break;
//                    
//                case KEY_SUB: // 降低阈值
//                    noise_threshold -= THRESHOLD_STEP;
//                    if(noise_threshold < 30.0) noise_threshold = 30.0;
//                    LCD_ShowNum(3, 1, (uint16_t)noise_threshold, 3);
//                    break;
//                    
//                case KEY_VIEW: // 查看历史数据
//                    view_history_mode = 1;
//                    ReadAllHistoryData();
//                    history_index = 0;
//                    DisplayHistoryData(history_index);
//                    break;
//                    
//                default:
//                    break;
//            }
//        }
//        else // 查看历史模式
//        {
//            /* 按键处理 */
//            switch(key)
//            {
//                case KEY_ADD: // 上一条记录
//                    if(history_index > 0) history_index--;
//                    DisplayHistoryData(history_index);
//                    break;
//                    
//                case KEY_SUB: // 下一条记录
//                    if(history_index < MAX_HISTORY_ITEMS-1) history_index++;
//                    DisplayHistoryData(history_index);
//                    break;
//                    
//                case KEY_EXIT: // 退出历史模式
//                    view_history_mode = 0;
//                    DisplayMainScreen();
//                    break;
//                    
//                default:
//                    break;
//            }
//        }
//        
        delay_ms(50);  // 主循环延时
    }
}

//// 存储当前数据
//void StoreCurrentData(float noise)
//{
//    // 准备存储数据
//    StorageData data;
//    data.header = DATA_HEADER;
//    data.noise_value = noise;
//    data.timestamp = system_time;
//    
//    // 转换为字节数组
//    uint8_t *dataBytes = (uint8_t*)&data;
//    
//    // 写入24C02
//    for(int i=0; i<sizeof(StorageData); i++)
//    {
//        AT24C02_WriteOneByte(storageAddr, dataBytes[i]);
//        storageAddr++;
//        
//        // 地址回绕处理
//        if(storageAddr >= MAX_STORAGE_ADDR - sizeof(StorageData))
//            storageAddr = 0;
//    }
//}

//// 读取所有历史数据到缓存
//void ReadAllHistoryData(void)
//{
//    uint16_t addr = 0;
//    uint8_t valid_count = 0;
//    
//    // 清空缓存
//    for(int i=0; i<MAX_HISTORY_ITEMS; i++)
//    {
//        history_data[i].header = 0;
//    }
//    
//    // 扫描整个24C02查找有效数据
//    while(addr < MAX_STORAGE_ADDR - sizeof(StorageData) && valid_count < MAX_HISTORY_ITEMS)
//    {
//        StorageData data;
//        uint8_t *dataBytes = (uint8_t*)&data;
//        
//        // 读取一条记录
//        for(int i=0; i<sizeof(StorageData); i++)
//        {
//            dataBytes[i] = AT24C02_ReadOneByte(addr + i);
//        }
//        
//        // 检查数据有效性
//        if(data.header == DATA_HEADER)
//        {
//            history_data[valid_count] = data;
//            valid_count++;
//        }
//        
//        addr += sizeof(StorageData);
//    }
//}

//// 显示指定索引的历史数据
//void DisplayHistoryData(uint16_t index)
//{
//    LCD_Clear();
//    LCD_ShowString(0, 0, "History Data:");
//    
//    if(history_data[index].header == DATA_HEADER)
//    {       
//        // 显示噪音值
//        LCD_ShowString(0, 1, "Noise:");
//        LCD_ShowNum(3, 1, (uint16_t)history_data[index].noise_value, 3);
//        
//        // 显示索引指示器
//        LCD_ShowString(0, 3, "Page:");
//        LCD_ShowNum(3, 3, index+1, 2);
//    }
//    else
//    {
//        LCD_ShowString(0, 1, "No Data!");
//    }
//}

//// 显示主界面
//void DisplayMainScreen(void)
//{
//    LCD_Clear();
//    LCD_ShowString(0, 0, "Noise:XXXdB");
//    LCD_ShowString(0, 1, "Thrsh:XXXdB");
//}