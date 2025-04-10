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

#include <stdlib.h> // 添加标准库头文件以使用abs()
#include <math.h>   // 如需使用数学函数

#define STORAGE_INTERVAL 1      // 存储间隔1秒
#define MAX_STORAGE_ADDR 256    // 24C02最大地址
#define DATA_HEADER 0xA5        // 数据头标识
#define MAX_HISTORY_ITEMS 20    // 最大显示历史记录数
#define THRESHOLD_STEP 3.0      // 阈值调整步长
#define WAVEFORM_POINTS 128     // 波形显示点数
#define ADC_MAX 4096            // ADC最大值

// 存储数据结构
typedef struct {
    uint8_t header;         // 1字节头标识
    uint16_t adc_value;     // 2字节ADC值
    uint32_t timestamp;     // 4字节时间戳(单位秒)
} StorageData;

uint16_t storageAddr = 0;    // 当前存储地址
float noise_threshold = 100.0; // 噪音阈值
uint32_t system_time = 0;    // 系统运行时间(秒)

uint8_t view_history_mode = 0; // 0:正常模式 1:查看历史模式
uint8_t display_mode = 0;     // 0:数值模式 1:波形模式
uint16_t history_index = 0;   // 当前查看的历史记录索引
StorageData history_data[MAX_HISTORY_ITEMS]; // 历史数据缓存
float waveform_data[WAVEFORM_POINTS];     // 波形数据缓冲区
uint8_t waveform_index = 0;   // 波形数据索引
uint8_t waveform_ready = 0;  // 新增：缓冲区就绪标志
#define SAMPLE_RATE 1000 // 假设采样率1kHz
#define MOVING_AVG_SIZE 5 // 移动平均窗口大小
#define MAX_FEATURES 64 // 最大特征点数量
#define NOISE_FLOOR 30.0f    // 最小显示噪声值
#define NOISE_CEILING 100.0f // 最大显示噪声值


// 函数声明
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
    /* 模块初始化 */
    LCD_Init();            // LCD初始化
    IIC_Init();             // IIC初始化
    LM2904_Init();          // 噪音传感器初始
    Usart3_Init(9600);
    WH_4G_Config();
    BEEP_Init();            // 蜂鸣器初始化
    Key_Init();             // 按键初始化
    
    /* 初始化TIM4定时器(1秒中断) */
    TIM4_Init(9999, 7199); // 10kHz计数频率，1秒中断
    
    /* 24C02初始化检测 */
    AT24C02_WriteOneByte(0, DATA_HEADER);
    if(AT24C02_ReadOneByte(0) != DATA_HEADER) {
        u3_printf("24C02 Error!\r\n");
        while(1);
    }
    else{
        u3_printf("24C02 OK!\r\n");
    }

    // 初始化波形缓冲区
    for(int i=0; i<WAVEFORM_POINTS; i++) {
        waveform_data[i] = NOISE_FLOOR;
    }

    /* 显示主界面 */
    DisplayMainScreen();
    TIM_Cmd(TIM4, ENABLE); // 启动TIM4
    
    while (1) 
    {
        Key_Value key = Key_Scan(); // 扫描按键
        
        if(!view_history_mode) // 正常模式
        {
            /* 读取ADC值 */
            uint16_t adc_value = LM2904_ReadValue();
            float noise = ConvertToDecibel(adc_value);
            
            /* 更新波形数据 */
            UpdateWaveform(noise);
            
            /* 根据显示模式更新界面 */
            if(display_mode == 0) {
                /* 数值显示模式 */
                LCD_ShowString(0, 0, "Noise:");
                LCD_ShowNum(3, 0, (uint16_t)noise, 3);
                LCD_ShowString(0, 1, "Thrsh:");
                LCD_ShowNum(3, 1, (uint16_t)noise_threshold, 3);

//                for (size_t i = 110; i < 128; i++)
//                {
//                    lcd_draw_dots(i, 0, 1); // 绘制分隔线 
//                }
				//lcd_draw_dots(60,0,1);

                // lcd_draw_Vline(126, 0, 63, 1); // 绘制分隔线 
                // lcd_draw_Vline(125, 0, 63, 1); // 绘制分隔线
                // lcd_draw_Vline(124, 0, 63, 1); // 绘制分隔线
                // lcd_draw_Vline(123, 0, 63, 1); // 绘制分隔线
                // lcd_draw_Vline(122, 0, 63, 1); // 绘制分隔线
                // lcd_draw_Vline(121, 0, 63, 1); // 绘制分隔线
                // lcd_draw_Vline(120, 0, 63, 1); // 绘制分隔线
                // lcd_draw_Vline(119, 0, 63, 1); // 绘制分隔线
                // lcd_draw_Vline(118, 0, 63, 1); // 绘制分隔线
                // lcd_draw_Vline(117, 0, 63, 1); // 绘制分隔线
                // lcd_draw_Vline(116, 0, 63, 1); // 绘制分隔线

                // 显示缓冲区填充状态
                if(!IsWaveformBufferFull()) {
                    LCD_ShowString(0, 2, "     %");
                    LCD_ShowNum(0, 2, waveform_index*100/WAVEFORM_POINTS, 3);
                }

                /* 噪音报警 */
                if(noise > noise_threshold) {
                    LCD_ShowString(0, 3, "Warn!");
                    BEEP_On();
                } else {
                    LCD_ShowString(0, 3, "Safe ");
                    BEEP_Off();
                }
            } else {
                /* 波形显示模式（仅显示波形） */
                LCD_Clear();
                lcd_clear();
                //DisplayWaveform();
                //DisplaySmoothWaveform();
                DisplayFullWaveform();
            }
            
            /* 按键处理 */
            switch(key)
            {
                case KEY_ADD: // 增加阈值
                    if(display_mode == 0) { // 只在数值模式下可调阈值
                        noise_threshold += THRESHOLD_STEP;
                        if(noise_threshold > 120.0) noise_threshold = 120.0;
                        LCD_ShowNum(3, 1, (uint16_t)noise_threshold, 3);
                    }
                    break;
                    
                case KEY_SUB: // 降低阈值
                    if(display_mode == 0) { // 只在数值模式下可调阈值
                        noise_threshold -= THRESHOLD_STEP;
                        if(noise_threshold < 30.0) noise_threshold = 30.0;
                        LCD_ShowNum(3, 1, (uint16_t)noise_threshold, 3);
                    }
                    break;
                    
                case KEY_VIEW: // 查看历史数据
                    if(display_mode == 0) { // 只在数值模式下可查看历史
                        view_history_mode = 1;
                        ReadAllHistoryData();
                        history_index = 0;
                        DisplayHistoryData(history_index);
                    }
                    break;
                    
                case KEY_EXIT: // 切换显示模式
                    display_mode = !display_mode;
                    if(display_mode == 0) {
                        DisplayMainScreen(); // 返回数值显示
                    } else {
                        lcd_clear();        // 进入纯波形模式
                    }
                    break;
                    
                default:
                    break;
            }
            
            /* 定时存储数据 */
            if(timer4_flag) {
                timer4_flag = 0;
                StoreCurrentData(adc_value);
                Wire4G_sendData(0x00, (uint16_t)noise);
            }
        }
        else // 查看历史模式
        {
            /* 按键处理 */
            switch(key)
            {
                case KEY_ADD: // 上一条记录
                    if(history_index > 0) history_index--;
                    DisplayHistoryData(history_index);
                    break;
                    
                case KEY_SUB: // 下一条记录
                    if(history_index < MAX_HISTORY_ITEMS-1) history_index++;
                    DisplayHistoryData(history_index);
                    break;
                    
                case KEY_EXIT: // 退出历史模式
                    view_history_mode = 0;
                    display_mode = 0;
                    DisplayMainScreen();
                    break;
                    
                default:
                    break;
            }
        }
        
        delay_ms(50);  // 主循环延时
    }
}


// 修改UpdateWaveform函数实现数据滚动更新
// 修改后的UpdateWaveform函数
void UpdateWaveform(float noise_db) {
    // 缓冲区未满时直接添加数据
    if(waveform_index < WAVEFORM_POINTS) {
        waveform_data[waveform_index++] = noise_db;
    } 
    // 缓冲区已满时滚动更新
    else {
        // 所有数据左移一位
        for(int i = 0; i < WAVEFORM_POINTS-1; i++) {
            waveform_data[i] = waveform_data[i+1];
        }
        // 新数据放在末尾
        waveform_data[WAVEFORM_POINTS-1] = noise_db;
    }
}
// 添加缓冲区状态检查函数
uint8_t IsWaveformBufferFull(void) {
    return (waveform_index >= WAVEFORM_POINTS);
}

// 添加重置缓冲区函数
void ResetWaveformBuffer(void) {
    waveform_index = 0;
}

//显示波形
void DisplayWaveform(void)
{
    static uint8_t last_y[WAVEFORM_POINTS] = {0};
    uint8_t i, y;
    

    // 清除旧波形
    for(i = 0; i < WAVEFORM_POINTS; i++) {
        lcd_draw_dots(i, last_y[i], 0); // 清除旧点
    }
    
    // 绘制新波形
    for(i = 0; i < WAVEFORM_POINTS; i++) {
        // 计算Y坐标 (0-63范围，63表示最小值)
        y = 63 - (waveform_data[(waveform_index + i) % WAVEFORM_POINTS] * 64 / ADC_MAX);
        
        // 确保y值在有效范围内
        if(y > 63) y = 63;
        
        // 绘制新点
        lcd_draw_dots(i, y, 1);
        last_y[i] = y;
    }
}


// 移动平均滤波
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

// 提取特征点（峰值和谷值）
void extract_features(float* samples, uint16_t count, uint16_t* feature_indices, uint8_t* feature_count) {
    *feature_count = 0;
    
    for(uint16_t i = 1; i < count - 1; i++) {
        if((samples[i] > samples[i-1] && samples[i] > samples[i+1]) || // 峰值
           (samples[i] < samples[i-1] && samples[i] < samples[i+1])) {  // 谷值
            if(*feature_count < MAX_FEATURES) {
                feature_indices[(*feature_count)++] = i;
            }
        }
    }
}


// 显示平滑波形（使用分贝值）
void DisplaySmoothWaveform(void) {
    static float filtered_samples[WAVEFORM_POINTS];
    static uint16_t feature_indices[MAX_FEATURES];
    static uint8_t feature_count = 0;
    
    // 1. 滤波处理
    for(int i = 0; i < WAVEFORM_POINTS; i++) {
        filtered_samples[i] = moving_average_filter(waveform_data[i]);
    }
    
    // 2. 特征提取
    extract_features(filtered_samples, WAVEFORM_POINTS, feature_indices, &feature_count);
    
    // 3. 清屏
    lcd_clear();
    
    // 4. 计算显示范围（30dB-120dB映射到0-63）
    float min_db = 10.0f;
    float max_db = 100.0f;
    float db_range = max_db - min_db;
    
    // 5. 绘制坐标轴
    lcd_draw_Vline(0, 0, 63, 1);   // Y轴
    lcd_draw_Hline(0, 63, 127, 1); // X轴（底部）
    
    // 6. 绘制平滑波形
    uint16_t last_x = 0;
    uint8_t last_y = 63 - (uint8_t)((filtered_samples[0] - min_db) * 63 / db_range);
    
    for(int i = 1; i < WAVEFORM_POINTS; i++) {
        uint16_t x = i;
        float amplitude_factor = 1.5f; // 幅度系数，大于1增加幅度
        uint8_t y = 63 - (uint8_t)((filtered_samples[i] - min_db) * 63 / db_range * amplitude_factor);
        
        
        // 确保y值在有效范围内
        if(y > 63) y = 63;
        if(y < 0) y = 0;
        
        // 使用Bresenham算法绘制线段
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
    
    // 7. 标记特征点
    for(int j = 0; j < feature_count; j++) {
        uint16_t idx = feature_indices[j];
        if(idx < WAVEFORM_POINTS) {
            uint8_t y = 63 - (uint8_t)((filtered_samples[idx] - min_db) * 63 / db_range);
            // 用十字标记特征点
            lcd_draw_dots(idx, y, 1);
            lcd_draw_dots(idx+1, y, 1);
            lcd_draw_dots(idx-1, y, 1);
            lcd_draw_dots(idx, y+1, 1);
            lcd_draw_dots(idx, y-1, 1);
        }
    }
    
    // // 8. 显示刻度标记（可选）
    // LCD_ShowString(120, 0, "120");
    // LCD_ShowString(120, 30, "75");
    // LCD_ShowString(120, 60, "30");
}

void DisplayFullWaveform(void) {
    // 1. 计算显示范围（固定范围）
    float min_db = NOISE_FLOOR;    // 使用预定义的最小值
    float max_db = NOISE_CEILING;  // 使用预定义的最大值
    float db_range = max_db - min_db;
    
    // 2. 清屏
    lcd_clear();
    
    // 3. 绘制坐标轴
    lcd_draw_Vline(0, 0, 63, 1);   // Y轴
    lcd_draw_Hline(0, 63, 127, 1); // X轴
    
    // 4. 绘制完整波形（使用所有128点）
    for(int i = 0; i < WAVEFORM_POINTS; i++) {
        // 计算Y坐标（0-63范围）
        uint8_t y = 63 - (uint8_t)((waveform_data[i] - min_db) * 63 / db_range);
        
        // 确保y值在有效范围内
        if(y > 63) y = 63;
        if(y < 0) y = 0;
        
        // 绘制当前点
        lcd_draw_dots(i, y, 1);
        
        // 将该点下方的所有点置1（填充波形下方区域）
        for(uint8_t fill_y = y + 1; fill_y <= 63; fill_y++) {
            lcd_draw_dots(i, fill_y, 1);
        }
    }
    
    // 5. 显示当前范围
   // LCD_ShowString(110, 0, "dB");
    //LCD_ShowNum(110, 0, (uint16_t)max_db, 3);
    //LCD_ShowNum(110, 60, (uint16_t)min_db, 3);
    
    // 6. 延迟1秒
    delay_ms(1000);
    
    // 7. 清空缓冲区
    waveform_index = 0;
    
    // 8. 重新填充缓冲区初始值
    for(int i = 0; i < WAVEFORM_POINTS; i++) {
        waveform_data[i] = NOISE_FLOOR;
    }

    display_mode = !display_mode;
    lcd_clear();
}


// 存储当前数据
void StoreCurrentData(uint16_t adc_value)
{
    // 准备存储数据
    StorageData data;
    data.header = DATA_HEADER;
    data.adc_value = adc_value;
    data.timestamp = system_time;
    
    // 转换为字节数组
    uint8_t *dataBytes = (uint8_t*)&data;
    
    // 写入24C02
    for(int i=0; i<sizeof(StorageData); i++)
    {
        AT24C02_WriteOneByte(storageAddr, dataBytes[i]);
        storageAddr++;
        
        // 地址回绕处理
        if(storageAddr >= MAX_STORAGE_ADDR - sizeof(StorageData))
            storageAddr = 0;
    }
}

// 读取所有历史数据到缓存
void ReadAllHistoryData(void)
{
    uint16_t addr = 0;
    uint8_t valid_count = 0;
    
    // 清空缓存
    for(int i=0; i<MAX_HISTORY_ITEMS; i++)
    {
        history_data[i].header = 0;
    }
    
    // 扫描整个24C02查找有效数据
    while(addr < MAX_STORAGE_ADDR - sizeof(StorageData) && valid_count < MAX_HISTORY_ITEMS)
    {
        StorageData data;
        uint8_t *dataBytes = (uint8_t*)&data;
        
        // 读取一条记录
        for(int i=0; i<sizeof(StorageData); i++)
        {
            dataBytes[i] = AT24C02_ReadOneByte(addr + i);
        }
        
        // 检查数据有效性
        if(data.header == DATA_HEADER)
        {
            history_data[valid_count] = data;
            valid_count++;
        }
        
        addr += sizeof(StorageData);
    }
}

// 显示指定索引的历史数据
void DisplayHistoryData(uint16_t index)
{
    LCD_Clear();
    LCD_ShowString(0, 0, "History Data:");
    
    if(history_data[index].header == DATA_HEADER)
    {       
        // 显示噪音值
        float noise = ConvertToDecibel(history_data[index].adc_value);
        LCD_ShowString(0, 1, "Noise:");
        LCD_ShowNum(3, 1, (uint16_t)noise, 3);
        
        // 显示索引指示器
        LCD_ShowString(0, 3, "Page:");
        LCD_ShowNum(3, 3, index+1, 2);
    }
    else
    {
        LCD_ShowString(0, 1, "No Data!");
    }
}

// 显示主界面
void DisplayMainScreen(void)
{
    LCD_Clear();
    LCD_ShowString(0, 0, "Noise:XXXdB");
    LCD_ShowString(0, 1, "Thrsh:XXXdB");
}

