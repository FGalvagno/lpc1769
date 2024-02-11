#include <LPC17xx.h>
#include "lpc17xx_adc.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_dac.h"

#define DMA_SIZE 60
#define NUM_SINE_SAMPLE 60
#define SINE_FREQ_IN_HZ 50
#define PCLK_DAC_IN_MHZ 25 //CCLK divided by 4

void confPin(void);
void confDMA(void);
void confDAC(void);

int Channel0_TC;
int Channel0_Err;


GPDMA_Channel_CFG_Type GPDMACfg;

uint32_t dac_sine_lut[NUM_SINE_SAMPLE];

int main(void){
	uint32_t i;
	uint32_t sin_0_to_90_16_samples[16] = {
			0,1045,2079,3090,4067,
			5000,5877,6691,7431,8090,
			8660,9135,9510,9781,9945,10000
	};
	confPin();
	confDAC();

	for(i=0; i<NUM_SINE_SAMPLE; i++){
		if(i<=15){
			dac_sine_lut[i] = 512 + 512*sin_0_to_90_16_samples[i]/10000;
			if(i==15) dac_sine_lut[i] = 1023;
		}
		else if(i<=30){
			dac_sine_lut[i] = 512 + 512*sin_0_to_90_16_samples[30-i]/10000;
		}
		else if(i<=45){
			dac_sine_lut[i] = 512 - 512*sin_0_to_90_16_samples[i-30]/10000;
		}
		else{
			dac_sine_lut[i] = 512 - 512*sin_0_to_90_16_samples[60-i]/10000;
		}
		dac_sine_lut[i] = (dac_sine_lut[i]<<6);
	}
	confDMA();
	while(1);
}

void confPin(void){
	PINSEL_CFG_Type PinCfg;
	/*
	 * Init DAC pin connect
	 * AOUT on P0.26
	 *
	 */
	PinCfg.Funcnum = 2;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Pinnum = 26;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	return;
}

void confDMA(void){
	GPDMA_LLI_Type LLI1;
	//Prepare DMA link list item structure
	LLI1.SrcAddr = (uint32_t) dac_sine_lut;
	LLI1.DstAddr = (uint32_t) &(LPC_DAC->DACR);
	LLI1.NextLLI = (uint32_t) &LLI1;
	LLI1.Control = DMA_SIZE
			| (2<<18) //source width 32bit
			| (2<<21) //dest. width 32bit
			| (1<<26) //source increment
			;
	//GPDMA block section
	GPDMA_Init();
	// Setup GPDMA Channel
	GPDMACfg.ChannelNum = 0;
	GPDMACfg.SrcMemAddr = (uint32_t)(dac_sine_lut);
	GPDMACfg.DstMemAddr = 0;
	GPDMACfg.TransferSize = DMA_SIZE;
	GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2P;
	GPDMACfg.SrcConn = 0;
	GPDMACfg.DstConn = GPDMA_CONN_DAC;
	GPDMACfg.DMALLI = (uint32_t) &LLI1;
	GPDMA_Setup(&GPDMACfg);

	//NVIC_EnableIRQ(DMA_IRQn);

	GPDMA_ChannelCmd(0, ENABLE);
	return;

}

void confDAC(void){
	uint32_t tmp;
	DAC_CONVERTER_CFG_Type DAC_ConverterConfigStruct;
	DAC_ConverterConfigStruct.CNT_ENA = SET;
	DAC_ConverterConfigStruct.DMA_ENA = SET;
	DAC_Init(LPC_DAC);

	//Timeout for DAC
	tmp = (PCLK_DAC_IN_MHZ*1000000)/(SINE_FREQ_IN_HZ*NUM_SINE_SAMPLE);
	DAC_SetDMATimeOut(LPC_DAC,tmp);
	DAC_ConfigDAConverterControl(LPC_DAC, &DAC_ConverterConfigStruct);
	return;
}



void DMA_IRQHandler (void)
{
	// check GPDMA interrupt on channel 0
	if (GPDMA_IntGetStatus(GPDMA_STAT_INT, 0)){ //check interrupt status on channel 0
		// Check counter terminal status
		if(GPDMA_IntGetStatus(GPDMA_STAT_INTTC, 0)){
			// Clear terminate counter Interrupt pending
			GPDMA_ClearIntPending (GPDMA_STATCLR_INTTC, 0);
				Channel0_TC++;

		}
		if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, 0)){
			// Clear error counter Interrupt pending
			GPDMA_ClearIntPending (GPDMA_STATCLR_INTERR, 0);
			Channel0_Err++;
		}
	}
}
