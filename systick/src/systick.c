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

void configST(void);
void configEINT(void);
void configPin(void);

int main(void) {
	configPin();
	configST();
	configEINT();
	while(1);
	return 0 ;
}

void configPin(void){
	// Config GPIO0.20 as output
	PINSEL_CFG_Type pinCfg;
	pinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	pinCfg.Funcnum = 0;
	pinCfg.Pinnum = 20;
	pinCfg.Portnum = 0;
	pinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PINSEL_ConfigPin(&pinCfg);

	LPC_GPIO0 -> FIODIR |= (1<<20);

	// Config EINT0
	pinCfg.Pinmode = PINSEL_PINMODE_PULLUP;
	pinCfg.Funcnum = 1;
	pinCfg.Pinnum = 12;
	pinCfg.Portnum = 2;
	pinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PINSEL_ConfigPin(&pinCfg);

}
void configST(void){
	//SysTick -> LOAD = ((SystemCoreClock/100)-1); //100ms
	//SysTick -> VAL = 0;
	//SysTick -> CTRL |= (1) | (1<<1);
	//NVIC_SetPriority (SysTick_IRQn, (0x1F);

	SysTick_Config(SystemCoreClock/100);
}

void configEINT(void){
	LPC_SC -> EXTMODE |= (1<<2); //Edge sensitive EINT0
	LPC_SC -> EXTPOLAR &= ~(1<<2); //INT on falling edge
	LPC_SC -> EXTINT |= (1<<2); //Clear flag

	NVIC_EnableIRQ(EINT2_IRQn);
}

void SysTick_Handler(void){
	static uint8_t i;
	if(i>=5){
		LPC_GPIO0 -> FIOPIN ^= (1<<20);
		i=0;
		return;
	}
	i++;
	return;
}

void EINT2_IRQHandler(void){
	SysTick -> CTRL ^= 0b11;
	LPC_SC -> EXTINT |= (1<<2); //Clear flag
}
