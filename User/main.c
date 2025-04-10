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

#include <stdlib.h> // ��ӱ�׼��ͷ�ļ���ʹ��abs()
#include <math.h>   // ����ʹ����ѧ����

#define STORAGE_INTERVAL 1      // �洢���1��
#define MAX_STORAGE_ADDR 256    // 24C02����ַ
#define DATA_HEADER 0xA5        // ����ͷ��ʶ
#define MAX_HISTORY_ITEMS 20    // �����ʾ��ʷ��¼��
#define THRESHOLD_STEP 3.0      // ��ֵ��������
#define WAVEFORM_POINTS 128     // ������ʾ����
#define ADC_MAX 4096            // ADC���ֵ

// �洢���ݽṹ
typedef struct {
    uint8_t header;         // 1�ֽ�ͷ��ʶ
    uint16_t adc_value;     // 2�ֽ�ADCֵ
    uint32_t timestamp;     // 4�ֽ�ʱ���(��λ��)
} StorageData;

uint16_t storageAddr = 0;    // ��ǰ�洢��ַ
float noise_threshold = 100.0; // ������ֵ
uint32_t system_time = 0;    // ϵͳ����ʱ��(��)

uint8_t view_history_mode = 0; // 0:����ģʽ 1:�鿴��ʷģʽ
uint8_t display_mode = 0;     // 0:��ֵģʽ 1:����ģʽ
uint16_t history_index = 0;   // ��ǰ�鿴����ʷ��¼����
StorageData history_data[MAX_HISTORY_ITEMS]; // ��ʷ���ݻ���
float waveform_data[WAVEFORM_POINTS];     // �������ݻ�����
uint8_t waveform_index = 0;   // ������������
uint8_t waveform_ready = 0;  // ������������������־
#define SAMPLE_RATE 1000 // ���������1kHz
#define MOVING_AVG_SIZE 5 // �ƶ�ƽ�����ڴ�С
#define MAX_FEATURES 64 // �������������
#define NOISE_FLOOR 30.0f    // ��С��ʾ����ֵ
#define NOISE_CEILING 100.0f // �����ʾ����ֵ


// ��������
void ReadAllHistoryData(void);
void DisplayHistoryData(uint16_t index);
void DisplayMainScreen(void);
void DisplayWaveform(void);
void StoreCurrentData(uint16_t adc_value);
void UpdateWaveform(float adc_value);
void DisplaySmoothWaveform(void);
float moving_average_filter(float new_sample);
void extract_features(float* samples, uint16_t count, uint16_t* feature_indices, uint8_t* feature_count);

void DisplayFullWaveform(void);
uint8_t IsWaveformBufferFull(void);
void ResetWaveformBuffer(void);


int main(void)
{
    /* ģ���ʼ�� */
    LCD_Init();            // LCD��ʼ��
    IIC_Init();             // IIC��ʼ��
    LM2904_Init();          // ������������ʼ
    Usart3_Init(9600);
    WH_4G_Config();
    BEEP_Init();            // ��������ʼ��
    Key_Init();             // ������ʼ��
    
    /* ��ʼ��TIM4��ʱ��(1���ж�) */
    TIM4_Init(9999, 7199); // 10kHz����Ƶ�ʣ�1���ж�
    
    /* 24C02��ʼ����� */
    AT24C02_WriteOneByte(0, DATA_HEADER);
    if(AT24C02_ReadOneByte(0) != DATA_HEADER) {
        u3_printf("24C02 Error!\r\n");
        while(1);
    }
    else{
        u3_printf("24C02 OK!\r\n");
    }

    // ��ʼ�����λ�����
    for(int i=0; i<WAVEFORM_POINTS; i++) {
        waveform_data[i] = NOISE_FLOOR;
    }

    /* ��ʾ������ */
    DisplayMainScreen();
    TIM_Cmd(TIM4, ENABLE); // ����TIM4
    
    while (1) 
    {
        Key_Value key = Key_Scan(); // ɨ�谴��
        
        if(!view_history_mode) // ����ģʽ
        {
            /* ��ȡADCֵ */
            uint16_t adc_value = LM2904_ReadValue();
            float noise = ConvertToDecibel(adc_value);
            
            /* ���²������� */
            UpdateWaveform(noise);
            
            /* ������ʾģʽ���½��� */
            if(display_mode == 0) {
                /* ��ֵ��ʾģʽ */
                LCD_ShowString(0, 0, "Noise:");
                LCD_ShowNum(3, 0, (uint16_t)noise, 3);
                LCD_ShowString(0, 1, "Thrsh:");
                LCD_ShowNum(3, 1, (uint16_t)noise_threshold, 3);

//                for (size_t i = 110; i < 128; i++)
//                {
//                    lcd_draw_dots(i, 0, 1); // ���Ʒָ��� 
//                }
				//lcd_draw_dots(60,0,1);

                // lcd_draw_Vline(126, 0, 63, 1); // ���Ʒָ��� 
                // lcd_draw_Vline(125, 0, 63, 1); // ���Ʒָ���
                // lcd_draw_Vline(124, 0, 63, 1); // ���Ʒָ���
                // lcd_draw_Vline(123, 0, 63, 1); // ���Ʒָ���
                // lcd_draw_Vline(122, 0, 63, 1); // ���Ʒָ���
                // lcd_draw_Vline(121, 0, 63, 1); // ���Ʒָ���
                // lcd_draw_Vline(120, 0, 63, 1); // ���Ʒָ���
                // lcd_draw_Vline(119, 0, 63, 1); // ���Ʒָ���
                // lcd_draw_Vline(118, 0, 63, 1); // ���Ʒָ���
                // lcd_draw_Vline(117, 0, 63, 1); // ���Ʒָ���
                // lcd_draw_Vline(116, 0, 63, 1); // ���Ʒָ���

                // ��ʾ���������״̬
                if(!IsWaveformBufferFull()) {
                    LCD_ShowString(0, 2, "     %");
                    LCD_ShowNum(0, 2, waveform_index*100/WAVEFORM_POINTS, 3);
                }

                /* �������� */
                if(noise > noise_threshold) {
                    LCD_ShowString(0, 3, "Warn!");
                    BEEP_On();
                } else {
                    LCD_ShowString(0, 3, "Safe ");
                    BEEP_Off();
                }
            } else {
                /* ������ʾģʽ������ʾ���Σ� */
                LCD_Clear();
                lcd_clear();
                //DisplayWaveform();
                //DisplaySmoothWaveform();
                DisplayFullWaveform();
            }
            
            /* �������� */
            switch(key)
            {
                case KEY_ADD: // ������ֵ
                    if(display_mode == 0) { // ֻ����ֵģʽ�¿ɵ���ֵ
                        noise_threshold += THRESHOLD_STEP;
                        if(noise_threshold > 120.0) noise_threshold = 120.0;
                        LCD_ShowNum(3, 1, (uint16_t)noise_threshold, 3);
                    }
                    break;
                    
                case KEY_SUB: // ������ֵ
                    if(display_mode == 0) { // ֻ����ֵģʽ�¿ɵ���ֵ
                        noise_threshold -= THRESHOLD_STEP;
                        if(noise_threshold < 30.0) noise_threshold = 30.0;
                        LCD_ShowNum(3, 1, (uint16_t)noise_threshold, 3);
                    }
                    break;
                    
                case KEY_VIEW: // �鿴��ʷ����
                    if(display_mode == 0) { // ֻ����ֵģʽ�¿ɲ鿴��ʷ
                        view_history_mode = 1;
                        ReadAllHistoryData();
                        history_index = 0;
                        DisplayHistoryData(history_index);
                    }
                    break;
                    
                case KEY_EXIT: // �л���ʾģʽ
                    display_mode = !display_mode;
                    if(display_mode == 0) {
                        DisplayMainScreen(); // ������ֵ��ʾ
                    } else {
                        lcd_clear();        // ���봿����ģʽ
                    }
                    break;
                    
                default:
                    break;
            }
            
            /* ��ʱ�洢���� */
            if(timer4_flag) {
                timer4_flag = 0;
                StoreCurrentData(adc_value);
                Wire4G_sendData(0x00, (uint16_t)noise);
            }
        }
        else // �鿴��ʷģʽ
        {
            /* �������� */
            switch(key)
            {
                case KEY_ADD: // ��һ����¼
                    if(history_index > 0) history_index--;
                    DisplayHistoryData(history_index);
                    break;
                    
                case KEY_SUB: // ��һ����¼
                    if(history_index < MAX_HISTORY_ITEMS-1) history_index++;
                    DisplayHistoryData(history_index);
                    break;
                    
                case KEY_EXIT: // �˳���ʷģʽ
                    view_history_mode = 0;
                    display_mode = 0;
                    DisplayMainScreen();
                    break;
                    
                default:
                    break;
            }
        }
        
        delay_ms(50);  // ��ѭ����ʱ
    }
}


// �޸�UpdateWaveform����ʵ�����ݹ�������
// �޸ĺ��UpdateWaveform����
void UpdateWaveform(float noise_db) {
    // ������δ��ʱֱ���������
    if(waveform_index < WAVEFORM_POINTS) {
        waveform_data[waveform_index++] = noise_db;
    } 
    // ����������ʱ��������
    else {
        // ������������һλ
        for(int i = 0; i < WAVEFORM_POINTS-1; i++) {
            waveform_data[i] = waveform_data[i+1];
        }
        // �����ݷ���ĩβ
        waveform_data[WAVEFORM_POINTS-1] = noise_db;
    }
}
// ��ӻ�����״̬��麯��
uint8_t IsWaveformBufferFull(void) {
    return (waveform_index >= WAVEFORM_POINTS);
}

// ������û���������
void ResetWaveformBuffer(void) {
    waveform_index = 0;
}

//��ʾ����
void DisplayWaveform(void)
{
    static uint8_t last_y[WAVEFORM_POINTS] = {0};
    uint8_t i, y;
    

    // ����ɲ���
    for(i = 0; i < WAVEFORM_POINTS; i++) {
        lcd_draw_dots(i, last_y[i], 0); // ����ɵ�
    }
    
    // �����²���
    for(i = 0; i < WAVEFORM_POINTS; i++) {
        // ����Y���� (0-63��Χ��63��ʾ��Сֵ)
        y = 63 - (waveform_data[(waveform_index + i) % WAVEFORM_POINTS] * 64 / ADC_MAX);
        
        // ȷ��yֵ����Ч��Χ��
        if(y > 63) y = 63;
        
        // �����µ�
        lcd_draw_dots(i, y, 1);
        last_y[i] = y;
    }
}


// �ƶ�ƽ���˲�
float moving_average_filter(float new_sample) {
    static float samples[MOVING_AVG_SIZE] = {0};
    static uint8_t index = 0;
    static float sum = 0;
    
    sum -= samples[index];
    samples[index] = new_sample;
    sum += new_sample;
    index = (index + 1) % MOVING_AVG_SIZE;
    
    return sum / MOVING_AVG_SIZE;
}

// ��ȡ�����㣨��ֵ�͹�ֵ��
void extract_features(float* samples, uint16_t count, uint16_t* feature_indices, uint8_t* feature_count) {
    *feature_count = 0;
    
    for(uint16_t i = 1; i < count - 1; i++) {
        if((samples[i] > samples[i-1] && samples[i] > samples[i+1]) || // ��ֵ
           (samples[i] < samples[i-1] && samples[i] < samples[i+1])) {  // ��ֵ
            if(*feature_count < MAX_FEATURES) {
                feature_indices[(*feature_count)++] = i;
            }
        }
    }
}


// ��ʾƽ�����Σ�ʹ�÷ֱ�ֵ��
void DisplaySmoothWaveform(void) {
    static float filtered_samples[WAVEFORM_POINTS];
    static uint16_t feature_indices[MAX_FEATURES];
    static uint8_t feature_count = 0;
    
    // 1. �˲�����
    for(int i = 0; i < WAVEFORM_POINTS; i++) {
        filtered_samples[i] = moving_average_filter(waveform_data[i]);
    }
    
    // 2. ������ȡ
    extract_features(filtered_samples, WAVEFORM_POINTS, feature_indices, &feature_count);
    
    // 3. ����
    lcd_clear();
    
    // 4. ������ʾ��Χ��30dB-120dBӳ�䵽0-63��
    float min_db = 10.0f;
    float max_db = 100.0f;
    float db_range = max_db - min_db;
    
    // 5. ����������
    lcd_draw_Vline(0, 0, 63, 1);   // Y��
    lcd_draw_Hline(0, 63, 127, 1); // X�ᣨ�ײ���
    
    // 6. ����ƽ������
    uint16_t last_x = 0;
    uint8_t last_y = 63 - (uint8_t)((filtered_samples[0] - min_db) * 63 / db_range);
    
    for(int i = 1; i < WAVEFORM_POINTS; i++) {
        uint16_t x = i;
        float amplitude_factor = 1.5f; // ����ϵ��������1���ӷ���
        uint8_t y = 63 - (uint8_t)((filtered_samples[i] - min_db) * 63 / db_range * amplitude_factor);
        
        
        // ȷ��yֵ����Ч��Χ��
        if(y > 63) y = 63;
        if(y < 0) y = 0;
        
        // ʹ��Bresenham�㷨�����߶�
        int dx = abs((int)(x - last_x));
        int dy = abs((int)(y - last_y));
        int sx = (last_x < x) ? 1 : -1;
        int sy = (last_y < y) ? 1 : -1;
        int err = dx - dy;
        
        while(1) {
            lcd_draw_dots(last_x, last_y, 1);
            
            if(last_x == x && last_y == y) break;
            
            int e2 = 2 * err;
            if(e2 > -dy) {
                err -= dy;
                last_x += sx;
            }
            if(e2 < dx) {
                err += dx;
                last_y += sy;
            }
        }
    }
    
    // 7. ���������
    for(int j = 0; j < feature_count; j++) {
        uint16_t idx = feature_indices[j];
        if(idx < WAVEFORM_POINTS) {
            uint8_t y = 63 - (uint8_t)((filtered_samples[idx] - min_db) * 63 / db_range);
            // ��ʮ�ֱ��������
            lcd_draw_dots(idx, y, 1);
            lcd_draw_dots(idx+1, y, 1);
            lcd_draw_dots(idx-1, y, 1);
            lcd_draw_dots(idx, y+1, 1);
            lcd_draw_dots(idx, y-1, 1);
        }
    }
    
    // // 8. ��ʾ�̶ȱ�ǣ���ѡ��
    // LCD_ShowString(120, 0, "120");
    // LCD_ShowString(120, 30, "75");
    // LCD_ShowString(120, 60, "30");
}

void DisplayFullWaveform(void) {
    // 1. ������ʾ��Χ���̶���Χ��
    float min_db = NOISE_FLOOR;    // ʹ��Ԥ�������Сֵ
    float max_db = NOISE_CEILING;  // ʹ��Ԥ��������ֵ
    float db_range = max_db - min_db;
    
    // 2. ����
    lcd_clear();
    
    // 3. ����������
    lcd_draw_Vline(0, 0, 63, 1);   // Y��
    lcd_draw_Hline(0, 63, 127, 1); // X��
    
    // 4. �����������Σ�ʹ������128�㣩
    for(int i = 0; i < WAVEFORM_POINTS; i++) {
        // ����Y���꣨0-63��Χ��
        uint8_t y = 63 - (uint8_t)((waveform_data[i] - min_db) * 63 / db_range);
        
        // ȷ��yֵ����Ч��Χ��
        if(y > 63) y = 63;
        if(y < 0) y = 0;
        
        // ���Ƶ�ǰ��
        lcd_draw_dots(i, y, 1);
        
        // ���õ��·������е���1����䲨���·�����
        for(uint8_t fill_y = y + 1; fill_y <= 63; fill_y++) {
            lcd_draw_dots(i, fill_y, 1);
        }
    }
    
    // 5. ��ʾ��ǰ��Χ
   // LCD_ShowString(110, 0, "dB");
    //LCD_ShowNum(110, 0, (uint16_t)max_db, 3);
    //LCD_ShowNum(110, 60, (uint16_t)min_db, 3);
    
    // 6. �ӳ�1��
    delay_ms(1000);
    
    // 7. ��ջ�����
    waveform_index = 0;
    
    // 8. ������仺������ʼֵ
    for(int i = 0; i < WAVEFORM_POINTS; i++) {
        waveform_data[i] = NOISE_FLOOR;
    }

    display_mode = !display_mode;
    lcd_clear();
}


// �洢��ǰ����
void StoreCurrentData(uint16_t adc_value)
{
    // ׼���洢����
    StorageData data;
    data.header = DATA_HEADER;
    data.adc_value = adc_value;
    data.timestamp = system_time;
    
    // ת��Ϊ�ֽ�����
    uint8_t *dataBytes = (uint8_t*)&data;
    
    // д��24C02
    for(int i=0; i<sizeof(StorageData); i++)
    {
        AT24C02_WriteOneByte(storageAddr, dataBytes[i]);
        storageAddr++;
        
        // ��ַ���ƴ���
        if(storageAddr >= MAX_STORAGE_ADDR - sizeof(StorageData))
            storageAddr = 0;
    }
}

// ��ȡ������ʷ���ݵ�����
void ReadAllHistoryData(void)
{
    uint16_t addr = 0;
    uint8_t valid_count = 0;
    
    // ��ջ���
    for(int i=0; i<MAX_HISTORY_ITEMS; i++)
    {
        history_data[i].header = 0;
    }
    
    // ɨ������24C02������Ч����
    while(addr < MAX_STORAGE_ADDR - sizeof(StorageData) && valid_count < MAX_HISTORY_ITEMS)
    {
        StorageData data;
        uint8_t *dataBytes = (uint8_t*)&data;
        
        // ��ȡһ����¼
        for(int i=0; i<sizeof(StorageData); i++)
        {
            dataBytes[i] = AT24C02_ReadOneByte(addr + i);
        }
        
        // ���������Ч��
        if(data.header == DATA_HEADER)
        {
            history_data[valid_count] = data;
            valid_count++;
        }
        
        addr += sizeof(StorageData);
    }
}

// ��ʾָ����������ʷ����
void DisplayHistoryData(uint16_t index)
{
    LCD_Clear();
    LCD_ShowString(0, 0, "History Data:");
    
    if(history_data[index].header == DATA_HEADER)
    {       
        // ��ʾ����ֵ
        float noise = ConvertToDecibel(history_data[index].adc_value);
        LCD_ShowString(0, 1, "Noise:");
        LCD_ShowNum(3, 1, (uint16_t)noise, 3);
        
        // ��ʾ����ָʾ��
        LCD_ShowString(0, 3, "Page:");
        LCD_ShowNum(3, 3, index+1, 2);
    }
    else
    {
        LCD_ShowString(0, 1, "No Data!");
    }
}

// ��ʾ������
void DisplayMainScreen(void)
{
    LCD_Clear();
    LCD_ShowString(0, 0, "Noise:XXXdB");
    LCD_ShowString(0, 1, "Thrsh:XXXdB");
}

