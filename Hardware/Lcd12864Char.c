#include "stm32f10x.h"
#include "delay.h"  // 假设您已实现微秒和毫秒延时函数
#include "Lcd12864Char.h"
#include "stdio.h"

/***********************
 *  LCD底层驱动函数
 ***********************/

// GPIO初始化
void LCD_GPIO_Config(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    // 开启GPIO时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC, ENABLE);

    // 配置PA0-PA7为推挽输出（数据线）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 |
                                  GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 配置控制线PC13(RS)、PC14(RW)、PC15(EN)为推挽输出
    GPIO_InitStructure.GPIO_Pin = LCD_RS_PIN | LCD_RW_PIN | LCD_EN_PIN;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

// 写命令函数
void LCD_WriteCmd(uint8_t cmd) {
    GPIO_ResetBits(LCD_CTRL_PORT, LCD_RS_PIN); // RS=0：命令模式
    GPIO_ResetBits(LCD_CTRL_PORT, LCD_RW_PIN); // RW=0：写入

    // 数据线输出命令
    LCD_DATA_PORT->ODR = (LCD_DATA_PORT->ODR & 0xFF00) | (cmd & 0x00FF);

    // 产生EN使能脉冲（>450ns）
    GPIO_SetBits(LCD_CTRL_PORT, LCD_EN_PIN);
    delay_us(1);
    GPIO_ResetBits(LCD_CTRL_PORT, LCD_EN_PIN);
    delay_us(100); // 等待命令执行
}

// 写数据函数
void LCD_WriteData(uint8_t data) {
    GPIO_SetBits(LCD_CTRL_PORT, LCD_RS_PIN); // RS=1：数据模式
    GPIO_ResetBits(LCD_CTRL_PORT, LCD_RW_PIN); // RW=0：写入

    // 数据线输出数据
    LCD_DATA_PORT->ODR = (LCD_DATA_PORT->ODR & 0xFF00) | (data & 0x00FF);

    // 产生EN使能脉冲
    GPIO_SetBits(LCD_CTRL_PORT, LCD_EN_PIN);
    delay_us(1);
    GPIO_ResetBits(LCD_CTRL_PORT, LCD_EN_PIN);
    delay_us(100);
}

/***********************
 *  LCD高级功能函数
 ***********************/

// LCD初始化
void LCD_Init(void) {
    LCD_GPIO_Config();  // 初始化GPIO
    delay_ms(50);       // 等待LCD上电稳定

    // 初始化序列（针对ST7920控制器）
    LCD_WriteCmd(0x38); // 功能设置：8位接口，基本指令集
    delay_ms(5);
    LCD_WriteCmd(0x0C); // 显示开，关光标
    delay_ms(5);
    LCD_WriteCmd(0x01); // 清屏
    delay_ms(5);
    LCD_WriteCmd(0x06); // 输入方式：光标右移
    delay_ms(5);
}

// 设置显示位置（x: 0-15, y: 0-3）
void LCD_SetPosition(uint8_t x, uint8_t y) {
    uint8_t addr;
    switch(y) {
        case 0: addr = 0x80 + x; break; // 第一行
        case 1: addr = 0x90 + x; break; // 第二行
        case 2: addr = 0x88 + x; break; // 第三行
        case 3: addr = 0x98 + x; break; // 第四行
        default: addr = 0x80;
    }
    LCD_WriteCmd(addr);
}

// 显示字符串
void LCD_PrintString(char *str) {
    while(*str) {
        LCD_WriteData(*str++);
    }
}

/**
 * @brief 在LCD指定位置显示字符串
 * @param x 列位置 (0-15)
 * @param y 行位置 (0-3)
 * @param str 要显示的字符串
 */
void LCD_PrintStringAt(uint8_t x, uint8_t y, char *str) {
    // 设置显示位置
    uint8_t addr;
    switch(y) {
        case 0: addr = 0x80 + x; break; // 第一行
        case 1: addr = 0x90 + x; break; // 第二行
        case 2: addr = 0x88 + x; break; // 第三行
        case 3: addr = 0x98 + x; break; // 第四行
        default: addr = 0x80;    // 默认第一行
    }
    LCD_WriteCmd(addr);
    
    // 显示字符串
    while(*str) {
        LCD_WriteData(*str++);
    }
}

void LCD_WriteDataAt(uint8_t x, uint8_t y, uint8_t data) {
    // 设置显示位置
    uint8_t addr;
    switch(y) {
        case 0: addr = 0x80 + x; break; // 第一行
        case 1: addr = 0x90 + x; break; // 第二行
        case 2: addr = 0x88 + x; break; // 第三行
        case 3: addr = 0x98 + x; break; // 第四行
        default: addr = 0x80;    // 默认第一行
    }
    LCD_WriteCmd(addr);
    
    // 显示字符
    LCD_WriteData(data);
}

// 清屏函数
void LCD_Clear(void) {
    LCD_WriteCmd(0x01); // 发送清屏命令
    delay_ms(2);        // 等待清屏完成（需要较长时间）
}


// 辅助函数：在LCD上显示数字
void LCD_ShowNum(uint8_t x, uint8_t y, uint16_t num, uint8_t len) {
    char buf[10];
    sprintf(buf, "%*d", len, num);
    LCD_PrintStringAt(x, y, buf);
}

// 辅助函数：在LCD上显示字符串
void LCD_ShowString(uint8_t x, uint8_t y, const char *str) {
    LCD_PrintStringAt(x, y, (char *)str);
}

// 辅助函数：在LCD上显示字符
void LCD_ShowChar(uint8_t x, uint8_t y, char ch) {
    char buf[2] = {ch, '\0'};
    LCD_PrintStringAt(x, y, buf);
}