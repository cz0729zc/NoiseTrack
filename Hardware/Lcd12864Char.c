#include "stm32f10x.h"
#include "delay.h"  // ��������ʵ��΢��ͺ�����ʱ����
#include "Lcd12864Char.h"
#include "stdio.h"

/***********************
 *  LCD�ײ���������
 ***********************/

// GPIO��ʼ��
void LCD_GPIO_Config(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    // ����GPIOʱ��
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC, ENABLE);

    // ����PA0-PA7Ϊ��������������ߣ�
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 |
                                  GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // ���ÿ�����PC13(RS)��PC14(RW)��PC15(EN)Ϊ�������
    GPIO_InitStructure.GPIO_Pin = LCD_RS_PIN | LCD_RW_PIN | LCD_EN_PIN;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

// д�����
void LCD_WriteCmd(uint8_t cmd) {
    GPIO_ResetBits(LCD_CTRL_PORT, LCD_RS_PIN); // RS=0������ģʽ
    GPIO_ResetBits(LCD_CTRL_PORT, LCD_RW_PIN); // RW=0��д��

    // �������������
    LCD_DATA_PORT->ODR = (LCD_DATA_PORT->ODR & 0xFF00) | (cmd & 0x00FF);

    // ����ENʹ�����壨>450ns��
    GPIO_SetBits(LCD_CTRL_PORT, LCD_EN_PIN);
    delay_us(1);
    GPIO_ResetBits(LCD_CTRL_PORT, LCD_EN_PIN);
    delay_us(100); // �ȴ�����ִ��
}

// д���ݺ���
void LCD_WriteData(uint8_t data) {
    GPIO_SetBits(LCD_CTRL_PORT, LCD_RS_PIN); // RS=1������ģʽ
    GPIO_ResetBits(LCD_CTRL_PORT, LCD_RW_PIN); // RW=0��д��

    // �������������
    LCD_DATA_PORT->ODR = (LCD_DATA_PORT->ODR & 0xFF00) | (data & 0x00FF);

    // ����ENʹ������
    GPIO_SetBits(LCD_CTRL_PORT, LCD_EN_PIN);
    delay_us(1);
    GPIO_ResetBits(LCD_CTRL_PORT, LCD_EN_PIN);
    delay_us(100);
}

/***********************
 *  LCD�߼����ܺ���
 ***********************/

// LCD��ʼ��
void LCD_Init(void) {
    LCD_GPIO_Config();  // ��ʼ��GPIO
    delay_ms(50);       // �ȴ�LCD�ϵ��ȶ�

    // ��ʼ�����У����ST7920��������
    LCD_WriteCmd(0x38); // �������ã�8λ�ӿڣ�����ָ�
    delay_ms(5);
    LCD_WriteCmd(0x0C); // ��ʾ�����ع��
    delay_ms(5);
    LCD_WriteCmd(0x01); // ����
    delay_ms(5);
    LCD_WriteCmd(0x06); // ���뷽ʽ���������
    delay_ms(5);
}

// ������ʾλ�ã�x: 0-15, y: 0-3��
void LCD_SetPosition(uint8_t x, uint8_t y) {
    uint8_t addr;
    switch(y) {
        case 0: addr = 0x80 + x; break; // ��һ��
        case 1: addr = 0x90 + x; break; // �ڶ���
        case 2: addr = 0x88 + x; break; // ������
        case 3: addr = 0x98 + x; break; // ������
        default: addr = 0x80;
    }
    LCD_WriteCmd(addr);
}

// ��ʾ�ַ���
void LCD_PrintString(char *str) {
    while(*str) {
        LCD_WriteData(*str++);
    }
}

/**
 * @brief ��LCDָ��λ����ʾ�ַ���
 * @param x ��λ�� (0-15)
 * @param y ��λ�� (0-3)
 * @param str Ҫ��ʾ���ַ���
 */
void LCD_PrintStringAt(uint8_t x, uint8_t y, char *str) {
    // ������ʾλ��
    uint8_t addr;
    switch(y) {
        case 0: addr = 0x80 + x; break; // ��һ��
        case 1: addr = 0x90 + x; break; // �ڶ���
        case 2: addr = 0x88 + x; break; // ������
        case 3: addr = 0x98 + x; break; // ������
        default: addr = 0x80;    // Ĭ�ϵ�һ��
    }
    LCD_WriteCmd(addr);
    
    // ��ʾ�ַ���
    while(*str) {
        LCD_WriteData(*str++);
    }
}

void LCD_WriteDataAt(uint8_t x, uint8_t y, uint8_t data) {
    // ������ʾλ��
    uint8_t addr;
    switch(y) {
        case 0: addr = 0x80 + x; break; // ��һ��
        case 1: addr = 0x90 + x; break; // �ڶ���
        case 2: addr = 0x88 + x; break; // ������
        case 3: addr = 0x98 + x; break; // ������
        default: addr = 0x80;    // Ĭ�ϵ�һ��
    }
    LCD_WriteCmd(addr);
    
    // ��ʾ�ַ�
    LCD_WriteData(data);
}

// ��������
void LCD_Clear(void) {
    LCD_WriteCmd(0x01); // ������������
    delay_ms(2);        // �ȴ�������ɣ���Ҫ�ϳ�ʱ�䣩
}


// ������������LCD����ʾ����
void LCD_ShowNum(uint8_t x, uint8_t y, uint16_t num, uint8_t len) {
    char buf[10];
    sprintf(buf, "%*d", len, num);
    LCD_PrintStringAt(x, y, buf);
}

// ������������LCD����ʾ�ַ���
void LCD_ShowString(uint8_t x, uint8_t y, const char *str) {
    LCD_PrintStringAt(x, y, (char *)str);
}

// ������������LCD����ʾ�ַ�
void LCD_ShowChar(uint8_t x, uint8_t y, char ch) {
    char buf[2] = {ch, '\0'};
    LCD_PrintStringAt(x, y, buf);
}