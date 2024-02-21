#include "LPC17xx.h"


#include <cr_section_macros.h>

#include "lpc17xx_gpdma.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_pinsel.h"

#define DMA_SIZE	0xFFF

uint8_t ch0[4096];
uint8_t ch1[4096];
uint8_t ch2[4096];
GPDMA_Channel_CFG_Type GPDMACfg;
GPDMA_LLI_Type LLI_CH0;
GPDMA_LLI_Type LLI_CH1;
GPDMA_LLI_Type LLI_CH2;

int main(void) {
	configPIN();
	configADC();
	configDAC();
	configTIMER();
	configDMA();
    while(1);
    return 0;
}

void configPIN(void){
	//EINT0 input
	PINSEL_CFG_Type pinCfg;
	pinCfg.Funcnum =  1;
	pinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	pinCfg.Pinmode = PINSEL_PINMODE_PULLUP;
	pinCfg.Portnum = 2;
	pinCfg.Pinnum = 10;
	PINSEL_ConfigPin(&pinCfg);
	LPC_SC->EXTMODE |= 1;
	NVIC_EnableIRQ(EINT0_IRQn);

	//A0 INPUT
	pinCfg.Funcnum = 1;
	pinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	pinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	pinCfg.Portnum = 0;
	pinCfg.Pinnum = 23;
	PINSEL_ConfigPin(&pinCfg);

	//A1 INPUT
	pinCfg.Funcnum = 1;
	pinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	pinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	pinCfg.Portnum = 0;
	pinCfg.Pinnum = 24;
	PINSEL_ConfigPin(&pinCfg);

	//A2 INPUT
	pinCfg.Funcnum = 1;
	pinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	pinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	pinCfg.Portnum = 0;
	pinCfg.Pinnum = 25;
	PINSEL_ConfigPin(&pinCfg);

	//AOUT OUTPUT
	pinCfg.Funcnum = 2;
	pinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	pinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	pinCfg.Portnum = 0;
	pinCfg.Pinnum = 26;
	PINSEL_ConfigPin(&pinCfg);

}

void configTIMER(void){
	TIM_TIMERCFG_Type configTimer;
	configTimer.PrescaleValue = 1; //TC incrementa cada 10ns
	configTimer.PrescaleOption = TIM_PRESCALE_TICKVAL;

	TIM_MATCHCFG_Type configMatch;
	configMatch.IntOnMatch = DISABLE;
	configMatch.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
	configMatch.MatchChannel = 1;
	configMatch.MatchValue = 2083; //2083*10ns -> 20.83us
	configMatch.ResetOnMatch = ENABLE;
	configMatch.StopOnMatch = DISABLE;

	TIM_ConfigMatch(LPC_TIM0, &configMatch);
	LPC_SC->PCLKSEL0 |= (1<<2); //sobreescribimos configuración del driver

	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &configTimer);
	TIM_Cmd(LPC_TIM0, ENABLE);

	return;
}

void configDMA(void){
	LLI_CH0.SrcAddr = (uint32_t) ch0;
	LLI_CH0.DstAddr = (uint32_t) &(LPC_DAC->DACR);
	LLI_CH0.NextLLI = (uint32_t) &LLI_CH0;
	LLI_CH0.Control = DMA_SIZE
					| (2<<18) //source width 32bit
					| (2<<21) //dest. width 32bit
					| (1<<26) //source increment
					;

	LLI_CH1.SrcAddr = (uint32_t) ch1;
	LLI_CH1.DstAddr = (uint32_t) &(LPC_DAC->DACR);
	LLI_CH1.NextLLI = (uint32_t) &LLI_CH1;
	LLI_CH1.Control = DMA_SIZE
					| (2<<18) //source width 32bit
					| (2<<21) //dest. width 32bit
					| (1<<26) //source increment
					;

	LLI_CH2.SrcAddr = (uint32_t) ch2;
	LLI_CH2.DstAddr = (uint32_t) &(LPC_DAC->DACR);
	LLI_CH2.NextLLI = (uint32_t) &LLI_CH2;
	LLI_CH2.Control = DMA_SIZE
					| (2<<18) //source width 32bit
					| (2<<21) //dest. width 32bit
					| (1<<26) //source increment
					;

	GPDMACfg.ChannelNum = 0;
	GPDMACfg.SrcMemAddr = (uint32_t) ch0;
	GPDMACfg.DstMemAddr = (uint32_t) &(LPC_DAC->DACR);
	GPDMACfg.TransferSize = 0xFFF;
	GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2P;
	GPDMACfg.SrcConn = 0;
	GPDMACfg.DstConn = GPDMA_CONN_DAC;
	GPDMACfg.DMALLI = (uint32_t) &LLI_CH0;
	GPDMACfg.TransferWidth = 2;

	GPDMA_Setup(&GPDMACfg);
	GPDMA_ChannelCmd(0, ENABLE);
}

void configDAC(void){
	DAC_Init(LPC_DAC);
	const uint32_t TIMEOUT = 521;
	DAC_SetDMATimeOut(LPC_DAC, TIMEOUT);
	DAC_CONVERTER_CFG_Type DACCfg;
	DACCfg.CNT_ENA = 1;
	DACCfg.DMA_ENA = 1;
	DAC_ConfigDAConverterControl(LPC_DAC, &DACCfg);
}

void configADC(void){
	ADC_Init(LPC_ADC, 200000);
	ADC_StartCmd(LPC_ADC, ADC_START_ON_MAT01);
	ADC_ChannelCmd(LPC_ADC, 0, ENABLE);
	ADC_ChannelCmd(LPC_ADC, 1, ENABLE);
	ADC_ChannelCmd(LPC_ADC, 2, ENABLE);
	ADC_IntConfig(LPC_ADC, ADC_ADINTEN2, ENABLE);
	NVIC_EnableIRQ(ADC_IRQn);
}

void EINT0_IRQHandler(void){
	static uint32_t i;
	GPDMA_ChannelCmd(0, DISABLE);
	switch(i){
		case 0:
			GPDMACfg.SrcMemAddr = (uint32_t) ch0;
			GPDMACfg.DMALLI = (uint32_t) &LLI_CH0;
			GPDMA_Setup(&GPDMACfg);
			break;

		case 1:
			GPDMACfg.SrcMemAddr = (uint32_t) ch1;
			GPDMACfg.DMALLI = (uint32_t) &LLI_CH1;
			GPDMA_Setup(&GPDMACfg);
			break;

		case 2:
			GPDMACfg.SrcMemAddr = (uint32_t) ch2;
			GPDMACfg.DMALLI = (uint32_t) &LLI_CH2;
			GPDMA_Setup(&GPDMACfg);
	}
	GPDMA_ChannelCmd(0, ENABLE);
	i++;
	if(i>=3){
		i=0;
	}
	LPC_SC->EXTINT |= 1; //Limpiamos flag
}

void ADC_IRQHandler(void){
	static uint32_t i;
	if(i<=4095){
		ch0[i] = (LPC_ADC ->ADDR0)&0xFFC0;
		ch1[i] = (LPC_ADC ->ADDR1)&0xFFC0;
		ch2[i] = (LPC_ADC ->ADDR2)&0xFFC0;
	}
	else{
		i=0;
		ch0[i] = (LPC_ADC ->ADDR0)&0xFFC0;
		ch1[i] = (LPC_ADC ->ADDR1)&0xFFC0;
		ch2[i] = (LPC_ADC ->ADDR2)&0xFFC0;
	}
	i++;
}
