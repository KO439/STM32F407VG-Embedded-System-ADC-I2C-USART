#include "stm32f4xx.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* ============================================================
   GLOBAL VARIABLES
============================================================ */
volatile int temp = 0;
volatile uint16_t adconv[3];
volatile int j = 0;
volatile int led_state = 0;

char buffer[10];
char res_buff[100];
int i = 0;

/* ============================================================
   SIMPLE DELAY
============================================================ */
void delay_ms(int ms)
{
    for(int i = 0; i < ms * 4000; i++);
}

/* ============================================================
   I2C1 CONFIGURATION (PB6=SCL, PB7=SDA)
============================================================ */
void I2C_simple_init(void)
{
    RCC->AHB1ENR |= (1<<1);      // GPIOB clock
    RCC->APB1ENR |= (1<<21);     // I2C1 clock

    GPIOB->MODER &= ~((3<<(6*2)) | (3<<(7*2)));
    GPIOB->MODER |=  (2<<(6*2)) | (2<<(7*2));
    GPIOB->OTYPER |= (1<<6) | (1<<7);
    GPIOB->OSPEEDR |= (3<<(6*2)) | (3<<(7*2));
    GPIOB->PUPDR |= (1<<(6*2)) | (1<<(7*2));
    GPIOB->AFR[0] |= (4<<(6*4)) | (4<<(7*4));

    I2C1->CR1 = 0;
    I2C1->CR2 = 16;      // 16 MHz
    I2C1->CCR = 80;      // 100 kHz
    I2C1->TRISE = 17;
    I2C1->CR1 |= (1<<0); // Enable I2C
}

/* ============================================================
   GPIO CONFIG (Button + LED)
============================================================ */
void config_GPIO(void)
{
    RCC->AHB1ENR |= (1<<0) | (1<<3);

    GPIOA->MODER &= ~(3<<(0*2));       // PA0 input
    GPIOD->MODER &= ~(3<<(15*2));
    GPIOD->MODER |=  (1<<(15*2));      // PD15 output
}

/* ============================================================
   USART3 CONFIG (Bluetooth HC-05)
============================================================ */
void config_GPIOB_USART(void)
{
    RCC->AHB1ENR |= (1<<1);

    GPIOB->MODER &= ~(3<<(10*2)) & ~(3<<(11*2));
    GPIOB->MODER |=  (2<<(10*2)) | (2<<(11*2));
    GPIOB->AFR[1] |= (7<<((10-8)*4)) | (7<<((11-8)*4));
}

void config_usart3(void)
{
    RCC->APB1ENR |= (1<<18);

    USART3->BRR = 0x068B;   // 9600 @16MHz
    USART3->CR1 |= (1<<2) | (1<<3) | (1<<5) | (1<<13);

    NVIC_EnableIRQ(USART3_IRQn);
}

void usart3_SendChar(char c)
{
    while(!(USART3->SR & (1<<7)));
    USART3->DR = c;
}

void usart3_SendString(char *s)
{
    while(*s) usart3_SendChar(*s++);
}

/* ============================================================
   ADC1 CONFIG (PA1, PA2, PA3)
============================================================ */
void config_ADC1(void)
{
    RCC->APB2ENR |= (1<<8);
    GPIOA->MODER |= (3<<(1*2)) | (3<<(2*2)) | (3<<(3*2));

    ADC1->CR1 = (1<<8) | (1<<5);     // Scan + EOC interrupt
    ADC1->CR2 = (1<<10) | (6<<24) | (1<<28) | (1<<0);

    ADC1->SQR1 = (2<<20);
    ADC1->SQR3 = (1<<0) | (2<<5) | (3<<10);
    ADC1->SMPR2 = (5<<3) | (5<<6) | (5<<9);

    NVIC_EnableIRQ(ADC_IRQn);
}

/* ============================================================
   TIM2 CONFIG (ADC Trigger)
============================================================ */
void config_TIM2(void)
{
    RCC->APB1ENR |= (1<<0);

    TIM2->PSC = 15999;   // 1kHz
    TIM2->ARR = 499;     // 2Hz
    TIM2->CR2 |= (2<<4); // TRGO on update
}

/* ============================================================
   MAIN
============================================================ */
int main(void)
{
    config_GPIO();
    config_GPIOB_USART();
    config_usart3();
    config_TIM2();
    config_ADC1();
    I2C_simple_init();

    while(1)
    {
        // System runs via interrupts
    }
}

/* ============================================================
   INTERRUPTS
============================================================ */
void ADC_IRQHandler(void)
{
    if (ADC1->SR & (1<<1))
    {
        adconv[j++] = ADC1->DR;

        if(j == 3)
        {
            j = 0;

            sprintf(res_buff,
                    "P1:%d P2:%d P3:%d Temp:%d\r\n",
                    adconv[0], adconv[1], adconv[2], temp);

            usart3_SendString(res_buff);
        }
    }
}

void USART3_IRQHandler(void)
{
    if (USART3->SR & (1<<5))
    {
        char rx = USART3->DR;

        if (i < 9)
            buffer[i++] = rx;

        if (i == 2)
        {
            buffer[i] = '\0';
            i = 0;
        }
    }
}
