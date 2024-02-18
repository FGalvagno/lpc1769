/*
 * Copyright 2022 NXP
 * NXP confidential.
 * This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms.  By expressly accepting
 * such terms or by downloading, installing, activating and/or otherwise using
 * the software, you are agreeing that you have read, and that you agree to
 * comply with and are bound by, such license terms.  If you do not agree to
 * be bound by the applicable license terms, then you may not retain, install,
 * activate or otherwise use the software.
 */


#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_dac.h"
volatile uint32_t duty[10];

void configTimer2(void);
void configDAC(void);



int main(void) {
	configTimer2();
	SysTick_Config(50000000); //Interrupt every 500ms
	configDAC();
	while(1);

}

void configDAC(void){
	PINSEL_CFG_Type pinCfg;
	//AOUT Output
	pinCfg.Funcnum = 2;
	pinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	pinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	pinCfg.Portnum = 0;
	pinCfg.Pinnum = 26;
	PINSEL_ConfigPin(&pinCfg);



	DAC_Init(LPC_DAC);
}

void configTimer2(void){
	PINSEL_CFG_Type pinCfg;
	pinCfg.Funcnum = 3;
	pinCfg.Portnum = 0;
	pinCfg.Pinnum = 4;
	pinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	pinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PINSEL_ConfigPin(&pinCfg);

	LPC_SC -> PCONP |= (1<<22);
	LPC_TIM2->PR = 0;
	LPC_TIM2->CCR |= (1<<1) | (1<<3); //Enable INT on capture.
    LPC_TIM2->TCR = 3;    // Enable and Reset
    LPC_TIM2->TCR ^= 2;

    NVIC_EnableIRQ(TIMER2_IRQn);

}

void TIMER2_IRQHandler(void){
	//Compute Period
	static uint32_t tiempo0;
	static uint32_t	tiempo1;
	static uint32_t period;
	static uint32_t i;
	switch(LPC_TIM2->CCR & 0x3){
		case 1:
			tiempo0 = (LPC_TIM2 -> CR0) ;//period on 0
			LPC_TIM2->CCR ^= 0x3;
			break;
		case 2:
			tiempo1 = (LPC_TIM2 -> CR0); //period on 1
			LPC_TIM2->CCR ^= 0x3;
			break;

	}
	period = tiempo0 + tiempo1;

	if(i<=9){
		duty[i] = tiempo1/period;
		i++;
	}
	else{
		i = 0;
		duty[i] = tiempo1/period;
		i++;
	}

	LPC_TIM2->IR &= ~(1<<4); //Clear IF
    LPC_TIM2->TCR = 3;    // Enable and Reset
    LPC_TIM2->TCR ^= 2;
}

void SysTick_Handler(void){
	int i, result;
	for(i=0; i<=9; i++){
		result += duty[i];
	}
	result /= 10;

	DAC_UpdateValue(LPC_DAC, 620*(result/100));
}




