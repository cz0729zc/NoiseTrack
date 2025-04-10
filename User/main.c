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

#define STORAGE_INTERVAL 1      // �洢���1��
#define MAX_STORAGE_ADDR 256    // 24C02����ַ
#define DATA_HEADER 0xA5        // ����ͷ��ʶ
#define MAX_HISTORY_ITEMS 20    // �����ʾ��ʷ��¼��
#define THRESHOLD_STEP 3.0      // ��ֵ��������

// �洢���ݽṹ
typedef struct {
    uint8_t header;         // 1�ֽ�ͷ��ʶ
    float noise_value;      // 4�ֽ�����ֵ
    uint32_t timestamp;     // 4�ֽ�ʱ���(��λ��)
} StorageData;

uint16_t storageAddr = 0;    // ��ǰ�洢��ַ
float noise_threshold = 80.0; // ������ֵ
uint32_t system_time = 0;    // ϵͳ����ʱ��(��)

uint8_t view_history_mode = 0; // 0:����ģʽ 1:�鿴��ʷģʽ
uint16_t history_index = 0; // ��ǰ�鿴����ʷ��¼����
StorageData history_data[MAX_HISTORY_ITEMS]; // ��ʷ���ݻ���

// ��������
void ReadAllHistoryData(void);
void DisplayHistoryData(uint16_t index);
void DisplayMainScreen(void);
void StoreCurrentData(float noise);

int main(void)
{
//    /* ģ���ʼ�� */
//    LCD_Init();            // LCD��ʼ��
//    IIC_Init();             // IIC��ʼ��
//    LM2904_Init();          // ������������ʼ
//    Usart3_Init(9600);
//	WH_4G_Config();
//    BEEP_Init();            // ��������ʼ��
//    Key_Init();             // ������ʼ��
//    
//    /* ��ʼ��TIM4��ʱ��(1���ж�) */
//    TIM4_Init(9999, 7199); // 10kHz����Ƶ�ʣ�1���ж�
//    
//    /* 24C02��ʼ����� */
//    AT24C02_WriteOneByte(0, DATA_HEADER);
//    if(AT24C02_ReadOneByte(0) != DATA_HEADER) {
//        u3_printf("24C02 Error!\r\n");
//        while(1);
//    }
//    else{
//        u3_printf("24C02 OK!\r\n");
//    }
//    
//    /* ��ʾ������ */
//    DisplayMainScreen();
//    TIM_Cmd(TIM4, ENABLE); // ����TIM4
    
	InitLCD_12864();
	ClearDot_12864();//������е�
    while (1) 
    {
		WriteCommand_12864(0x34);
        DrawDot_12864(32, 6, 10); // ��(0,0)������
		WriteCommand_12864(0x36);
		//DrawDot_Picture();
		
//		//u3_printf("24C02 OK!\r\n");
//		Key_Value key = Key_Scan(); // ɨ�谴��
//        
//        if(!view_history_mode) // ����ģʽ
//        {
//            /* ��ȡ����ʾ����ֵ */
//            float noise = ConvertToDecibel(LM2904_ReadValue());
//            //u3_printf("noise:%.2f dB\r\n", noise);
//            LCD_ShowNum(3, 0, (uint16_t)noise, 3);
//            
//            /* ��ʾ��ֵ */
//            LCD_ShowString(0, 1, "Thrsh:");
//            LCD_ShowNum(3, 1, (uint16_t)noise_threshold, 3);
//            
//            /* �������� */
//            if(noise > noise_threshold) {
//                LCD_ShowString(0, 3, "Warn!");
//				//Wire4G_yuzhiData(0x00,(uint16_t)noise);
//                BEEP_On();
//            } else {
//                LCD_ShowString(0, 3, "Safe ");
//                BEEP_Off();
//            }
//            
//            /* ��ʱ�洢���� */
//            if(timer4_flag) {
//                timer4_flag = 0;
//                StoreCurrentData(noise);
//				Wire4G_sendData(0x00,(uint16_t)noise);
//            }
//            
//            /* �������� */
//            switch(key)
//            {
//                case KEY_ADD: // ������ֵ
//                    noise_threshold += THRESHOLD_STEP;
//                    if(noise_threshold > 120.0) noise_threshold = 120.0;
//                    LCD_ShowNum(3, 1, (uint16_t)noise_threshold, 3);
//                    break;
//                    
//                case KEY_SUB: // ������ֵ
//                    noise_threshold -= THRESHOLD_STEP;
//                    if(noise_threshold < 30.0) noise_threshold = 30.0;
//                    LCD_ShowNum(3, 1, (uint16_t)noise_threshold, 3);
//                    break;
//                    
//                case KEY_VIEW: // �鿴��ʷ����
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
//        else // �鿴��ʷģʽ
//        {
//            /* �������� */
//            switch(key)
//            {
//                case KEY_ADD: // ��һ����¼
//                    if(history_index > 0) history_index--;
//                    DisplayHistoryData(history_index);
//                    break;
//                    
//                case KEY_SUB: // ��һ����¼
//                    if(history_index < MAX_HISTORY_ITEMS-1) history_index++;
//                    DisplayHistoryData(history_index);
//                    break;
//                    
//                case KEY_EXIT: // �˳���ʷģʽ
//                    view_history_mode = 0;
//                    DisplayMainScreen();
//                    break;
//                    
//                default:
//                    break;
//            }
//        }
//        
        delay_ms(50);  // ��ѭ����ʱ
    }
}

//// �洢��ǰ����
//void StoreCurrentData(float noise)
//{
//    // ׼���洢����
//    StorageData data;
//    data.header = DATA_HEADER;
//    data.noise_value = noise;
//    data.timestamp = system_time;
//    
//    // ת��Ϊ�ֽ�����
//    uint8_t *dataBytes = (uint8_t*)&data;
//    
//    // д��24C02
//    for(int i=0; i<sizeof(StorageData); i++)
//    {
//        AT24C02_WriteOneByte(storageAddr, dataBytes[i]);
//        storageAddr++;
//        
//        // ��ַ���ƴ���
//        if(storageAddr >= MAX_STORAGE_ADDR - sizeof(StorageData))
//            storageAddr = 0;
//    }
//}

//// ��ȡ������ʷ���ݵ�����
//void ReadAllHistoryData(void)
//{
//    uint16_t addr = 0;
//    uint8_t valid_count = 0;
//    
//    // ��ջ���
//    for(int i=0; i<MAX_HISTORY_ITEMS; i++)
//    {
//        history_data[i].header = 0;
//    }
//    
//    // ɨ������24C02������Ч����
//    while(addr < MAX_STORAGE_ADDR - sizeof(StorageData) && valid_count < MAX_HISTORY_ITEMS)
//    {
//        StorageData data;
//        uint8_t *dataBytes = (uint8_t*)&data;
//        
//        // ��ȡһ����¼
//        for(int i=0; i<sizeof(StorageData); i++)
//        {
//            dataBytes[i] = AT24C02_ReadOneByte(addr + i);
//        }
//        
//        // ���������Ч��
//        if(data.header == DATA_HEADER)
//        {
//            history_data[valid_count] = data;
//            valid_count++;
//        }
//        
//        addr += sizeof(StorageData);
//    }
//}

//// ��ʾָ����������ʷ����
//void DisplayHistoryData(uint16_t index)
//{
//    LCD_Clear();
//    LCD_ShowString(0, 0, "History Data:");
//    
//    if(history_data[index].header == DATA_HEADER)
//    {       
//        // ��ʾ����ֵ
//        LCD_ShowString(0, 1, "Noise:");
//        LCD_ShowNum(3, 1, (uint16_t)history_data[index].noise_value, 3);
//        
//        // ��ʾ����ָʾ��
//        LCD_ShowString(0, 3, "Page:");
//        LCD_ShowNum(3, 3, index+1, 2);
//    }
//    else
//    {
//        LCD_ShowString(0, 1, "No Data!");
//    }
//}

//// ��ʾ������
//void DisplayMainScreen(void)
//{
//    LCD_Clear();
//    LCD_ShowString(0, 0, "Noise:XXXdB");
//    LCD_ShowString(0, 1, "Thrsh:XXXdB");
//}