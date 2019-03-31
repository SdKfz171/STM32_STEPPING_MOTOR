#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_tim.h>
#include <stm32f10x_rcc.h>
#include <string.h>

#define HEX_CHARS      "0123456789ABCDEF"

#define DELAY_TIM_FREQUENCY_US 1000000		/* = 1MHZ -> timer runs in microseconds */
#define DELAY_TIM_FREQUENCY_MS 1000			/* = 1kHZ -> timer runs in milliseconds */

// Init timer for Microseconds delays
void _init_us() {
	// Enable clock for TIM2
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);

	// Time base configuration
	TIM_TimeBaseInitTypeDef TIM;
	TIM_TimeBaseStructInit(&TIM);
	TIM.TIM_Prescaler = (SystemCoreClock/DELAY_TIM_FREQUENCY_US)-1;
	TIM.TIM_Period = UINT16_MAX;
	TIM.TIM_ClockDivision = 0;
	TIM.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2,&TIM);

	// Enable counter for TIM2
	TIM_Cmd(TIM2,ENABLE);
}

// Init and start timer for Milliseconds delays
void _init_ms() {
	// Enable clock for TIM2
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);

	// Time base configuration
	TIM_TimeBaseInitTypeDef TIM;
	TIM_TimeBaseStructInit(&TIM);
	TIM.TIM_Prescaler = (SystemCoreClock/DELAY_TIM_FREQUENCY_MS)-1;
	TIM.TIM_Period = UINT16_MAX;
	TIM.TIM_ClockDivision = 0;
	TIM.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2,&TIM);

	// Enable counter for TIM2
	TIM_Cmd(TIM2,ENABLE);
}

// Stop timer
void _stop_timer() {
	TIM_Cmd(TIM2,DISABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,DISABLE); // Powersavings?
}

// Do delay for nTime milliseconds
void Delay_ms(uint32_t mSecs) {
	// Init and start timer
	_init_ms();

	// Dummy loop with 16 bit count wrap around
	volatile uint32_t start = TIM2->CNT;
	while((TIM2->CNT-start) <= mSecs);

	// Stop timer
	_stop_timer();
}

// Do delay for nTime microseconds
void Delay_us(uint32_t uSecs) {
	// Init and start timer
	_init_us();

	// Dummy loop with 16 bit count wrap around
	volatile uint32_t start = TIM2->CNT;
	while((TIM2->CNT-start) <= uSecs);

	// Stop timer
	_stop_timer();
}



void USART_SendChar(char ch) {
	while(!USART_GetFlagStatus(USART1, USART_FLAG_TC));    // wait for "Transmission Complete" flag cleared
	USART_SendData(USART1, ch);
	GPIO_SetBits(GPIOD,GPIO_Pin_2);
}

void USART_SendInt(int32_t num) {
	char str[10]; // 10 chars max for INT32_MAX
	int i = 0;
	if (num < 0) {
		USART_SendChar('-');
		num *= -1;
	}
	do str[i++] = num % 10 + '0'; while ((num /= 10) > 0);
	for (i--; i >= 0; i--) USART_SendChar(str[i]);
}

void USART_SendInt0(int32_t num) {
	char str[10]; // 10 chars max for INT32_MAX
	int i = 0;
	if (num < 0) {
		USART_SendChar('-');
		num *= -1;
	}
	if ((num < 10) && (num >= 0)) USART_SendChar('0');
	do str[i++] = num % 10 + '0'; while ((num /= 10) > 0);
	for (i--; i >= 0; i--) USART_SendChar(str[i]);
}

void USART_SendHex8(uint16_t num) {
	USART_SendChar(HEX_CHARS[(num >> 4)   % 0x10]);
	USART_SendChar(HEX_CHARS[(num & 0x0f) % 0x10]);
}

void USART_SendHex16(uint16_t num) {
	uint8_t i;
	for (i = 12; i > 0; i -= 4) USART_SendChar(HEX_CHARS[(num >> i) % 0x10]);
	USART_SendChar(HEX_CHARS[(num & 0x0f) % 0x10]);
}

void USART_SendHex32(uint32_t num) {
	uint8_t i;
	for (i = 28; i > 0; i -= 4)	USART_SendChar(HEX_CHARS[(num >> i) % 0x10]);
	USART_SendChar(HEX_CHARS[(num & 0x0f) % 0x10]);
}

void USART_SendStr(char *str) {
	while (*str) USART_SendChar(*str++);
}

void USART_SendBuf(char *buf, uint16_t bufsize) {
	uint16_t i;
	for (i = 0; i < bufsize; i++) USART_SendChar(*buf++);
}

void USART_SendBufPrintable(char *buf, uint16_t bufsize, char subst) {
	uint16_t i;
	char ch;
	for (i = 0; i < bufsize; i++) {
		ch = *buf++;
		USART_SendChar(ch > 32 ? ch : subst);
	}
}

void USART_SendBufHex(char *buf, uint16_t bufsize) {
	uint16_t i;
	char ch;
	for (i = 0; i < bufsize; i++) {
		ch = *buf++;
		USART_SendChar((char)ch);
		//USART_SendChar((char)HEX_CHARS[(ch >> 4)   % 0x10]);
		//USART_SendChar((char)HEX_CHARS[(ch & 0x0f) % 0x10]);
	}
}

void USART_SendBufHexFancy(char *buf, uint16_t bufsize, uint8_t column_width, char subst) {
	uint16_t i = 0,len,pos;
	char buffer[column_width];

	while (i < bufsize) {
		// Line number
		USART_SendHex16(i);
		USART_SendChar(':'); USART_SendChar(' '); // Faster and less code than USART_SendStr(": ");

		// Copy one line
		if (i+column_width >= bufsize) len = bufsize - i; else len = column_width;
		memcpy(buffer,&buf[i],len);

		// Hex data
		pos = 0;
		while (pos < len) USART_SendHex8(buffer[pos++]);
		USART_SendChar(' ');

		// Raw data
		pos = 0;
		do USART_SendChar(buffer[pos] > 32 ? buffer[pos] : subst); while (++pos < len);
		USART_SendChar('\n');

		i += len;
	}
}

static void GPIO_setting(void)
{
	RCC_ADCCLKConfig(RCC_PCLK2_Div2);
	
	RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
	RCC->APB2ENR |= RCC_APB2ENR_IOPDEN;
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
	RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
	
	GPIO_InitTypeDef GPIO_InitStructure;

	// LED
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	
	// DIR
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	// STEP
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	// USART1 TX
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	// USART1 RX
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void Serial_Init(uint32_t baudRate){
	USART_InitTypeDef USART_InitStructure;
	USART_ClockInitTypeDef USART_Clock_InitStructure;
	
	USART_Clock_InitStructure.USART_Clock = USART_Clock_Disable;
	USART_Clock_InitStructure.USART_CPHA = USART_CPHA_2Edge;
	USART_Clock_InitStructure.USART_CPOL = USART_CPOL_Low;
	USART_Clock_InitStructure.USART_LastBit = USART_LastBit_Disable;
	USART_ClockInit(USART1, &USART_Clock_InitStructure);
	
	USART_InitStructure.USART_BaudRate = baudRate;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; 
	USART_Init(USART1, &USART_InitStructure);
	
	USART_Cmd(USART1, ENABLE);
}

int main(){
	GPIO_setting();
	//Serial_Init(115200);
	
	
	GPIO_ResetBits(GPIOC, GPIO_Pin_11);
	
	
	while(1){
		for(int i = 0; i < 200; i++){
			GPIO_SetBits(GPIOC, GPIO_Pin_12);
			GPIO_SetBits(GPIOD, GPIO_Pin_2);
			Delay_us(4000);
			GPIO_ResetBits(GPIOC, GPIO_Pin_12);
			GPIO_ResetBits(GPIOD, GPIO_Pin_2);
			Delay_us(4000);
		}
		
//		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_0)){
//			USART_SendStr("D0");
////			for(int i = 0; i < 200; i++){
////				GPIO_SetBits(GPIOC, GPIO_Pin_12);
////				GPIO_SetBits(GPIOD, GPIO_Pin_2);
////				Delay_us(1000);
////				GPIO_ResetBits(GPIOC, GPIO_Pin_12);
////				GPIO_ResetBits(GPIOD, GPIO_Pin_2);
////				Delay_us(1000);
////			}
//			
//		}
//		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_1)){
//			GPIO_ResetBits(GPIOC, GPIO_Pin_12);
//			USART_SendStr("D1");
//		}
//		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_2)){
//			USART_SendStr("D2");
//		}
//		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_3)){
//			USART_SendStr("D3");
//		}
//		
//		if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7)){
//			USART_SendStr("On\r\n");
//		}
//		
//		if(!GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7)){
//			USART_SendStr("Off\r\n");
//		}
	}
	
//	
//	while(1){
//		for(int i = 0; i < 200; i++){
//			GPIO_SetBits(GPIOC, GPIO_Pin_12);
//			GPIO_SetBits(GPIOD, GPIO_Pin_2);
//			Delay_us(1000);
//			GPIO_ResetBits(GPIOC, GPIO_Pin_12);
//			GPIO_ResetBits(GPIOD, GPIO_Pin_2);
//			Delay_us(1000);
//		}
//	}
	
	return 0;
}