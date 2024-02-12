/*
 * Example of LPC1769 using GPIO ports to drive a display. With a switch selects between
 * counter and time mode
 *
 */

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#endif

#define INPUT_PIN 	1<<20
#define OUTPUT_7SEG 0x7F<<18
void init_GPIO();
void init_TMR0();

char seg[10] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f};
char state;
// P1(18:24) display
// P0(20) button
int main(void) {
	init_GPIO();
	init_TMR0();

    while(1) {
    }
    return 0 ;
}

void init_GPIO(void){
	// Config GPIO
	LPC_PINCON ->  PINSEL3 &= !(0xFFFFF0); //bit 4 a bit 19 en 0
	LPC_PINCON ->  PINSEL1 &= !(0x3<<8); //bit 8 a bit 9 en 0
	// Config RESISTORS
	LPC_PINCON ->  PINMODE3 |= 0x2AAAA0;
	LPC_PINCON ->  PINMODE1 &= !(0x3<<8); //bit 8 a bit 9 en 0

	// Config DIRECTIONS
	LPC_GPIO1 -> FIODIR |= OUTPUT_7SEG;  //P1(18:24) OUTPUT
	LPC_GPIO0 -> FIODIR &= !(INPUT_PIN);

	LPC_GPIOINT->IO0IntEnF |= INPUT_PIN;	//INT on falling edge

	NVIC_EnableIRQ(EINT3_IRQn);

}

void init_TMR0(void){
	LPC_SC->PCONP |= (1 << 1);
	LPC_SC->PCLKSEL0 |= (1 << 2); // PCLK = cclk

	LPC_TIM0 -> PR = 249999;
	LPC_TIM0 -> MR0 = 1;
	LPC_TIM0 -> MCR = 3;
	LPC_TIM0->IR |= 0x3F;      // Clear all interrupt flag
	LPC_TIM0 -> TCR |= 3; 		//RESET Timer0
	//CTCR -> 00 on reset
	//
	NVIC_EnableIRQ(TIMER0_IRQn);
}

void EINT3_IRQHandler(void){
	LPC_TIM0-> TCR ^= 1<<1; //Enable Timer0

	LPC_GPIOINT->IO0IntClr |= INPUT_PIN; //Clear INT flag

}


void TIMER0_IRQHandler(void){
	LPC_TIM0 -> TCR |= 3; 		//RESET Timer0
	switch_display();
	NVIC_ClearPendingIRQ(TIMER0_IRQn);
}

void switch_display(){
	static int i;
		i++;
		if(i >= 10){
			i = 0;
		}

		LPC_GPIO1 -> FIOMASK = !(OUTPUT_7SEG);
		LPC_GPIO1 -> FIOPIN = seg[i]<<18;
}
