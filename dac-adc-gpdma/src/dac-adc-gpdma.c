#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_timer.h"

void configADC(void);
void configDMA(void);
void configPin(void);
void computeSignal(void);
void generate_waveform(void);

#define ADC_MEM_DIRECTION 0x2007C000UL
#define DAC_MEM_DIRECTION ADC_MEM_DIRECTION + 0x4000UL
#define SAMPLE_SIZE 1024UL

int Channel0_TC;
int Channel0_Err;

int Channel1_TC;
int Channel1_Err;

GPDMA_Channel_CFG_Type GPDMACfg;

volatile uint32_t wave_form[SAMPLE_SIZE];
volatile uint32_t ADC_Buffer[4096];
//uint32_t wave_form[SAMPLE_SIZE] __attribute__ ((section("AHBSRAM0")));

int main(void) {
	generate_waveform();
	configPin();
	configADC();
	configDAC();
	configTMR0();
	configDMA();
	while(1);
    return 0 ;
}

void configPin(void){

	//EINT0 input
	PINSEL_CFG_Type pinCfg;
	pinCfg.Funcnum =  1;
	pinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	pinCfg.Pinmode = PINSEL_PINMODE_PULLUP;
	pinCfg.Portnum = 2;
	pinCfg.Pinnum = 10;
	PINSEL_ConfigPin(&pinCfg);
	LPC_SC->EXTMODE |= 1;
	//NVIC_EnableIRQ(EINT0_IRQn);

	//A0 INPUT
		pinCfg.Funcnum = 1;
		pinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
		pinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
		pinCfg.Portnum = 0;
		pinCfg.Pinnum = 24;
		PINSEL_ConfigPin(&pinCfg);

	//AOUT Output
	pinCfg.Funcnum = 2;
	pinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	pinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	pinCfg.Portnum = 0;
	pinCfg.Pinnum = 26;
	PINSEL_ConfigPin(&pinCfg);

	//MAT01 OUTPUT
	pinCfg.Funcnum = 3;
	pinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	pinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	pinCfg.Portnum = 1;
	pinCfg.Pinnum = 29;
	PINSEL_ConfigPin(&pinCfg);
	return;
}

void configADC(void){
	ADC_Init(LPC_ADC, 200000);
		//ADC_BurstCmd(LPC_ADC, ENABLE);
		ADC_StartCmd(LPC_ADC, ADC_START_ON_MAT01);
		ADC_ChannelCmd(LPC_ADC, 1, ENABLE);
		ADC_EdgeStartConfig(LPC_ADC, ADC_START_ON_RISING);
		ADC_IntConfig(LPC_ADC, ADC_ADINTEN1, ENABLE);
		NVIC_EnableIRQ(ADC_IRQn);
}

void configDAC(void){
	DAC_Init(LPC_DAC);
	const uint32_t TIMEOUT = 15; //PCLK/(FREQ*SAMPLES)
	DAC_SetDMATimeOut(LPC_DAC, TIMEOUT);
	DAC_CONVERTER_CFG_Type DACCfg;
	DACCfg.CNT_ENA = 1;
	DACCfg.DMA_ENA = 1;
	DAC_ConfigDAConverterControl(LPC_DAC, &DACCfg);
}

void configDMA(void){
    NVIC_DisableIRQ(DMA_IRQn);

	GPDMA_LLI_Type LLI1;
	//Prepare DMA link list item structure
	LLI1.SrcAddr = DAC_MEM_DIRECTION;
	LLI1.DstAddr = (uint32_t) &(LPC_DAC->DACR);
	LLI1.NextLLI = (uint32_t) &LLI1;
	LLI1.Control = SAMPLE_SIZE
				| (2<<18) //source width 32bit
				| (2<<21) //dest. width 32bit
				| (1<<26) //source increment
				;

	GPDMA_LLI_Type LLI2;
	LLI2.SrcAddr = (uint32_t)(ADC_Buffer);
	LLI2.DstAddr = ADC_MEM_DIRECTION;
	LLI2.NextLLI = (uint32_t) &LLI2;
	LLI2.Control = 0xFFF
				| (2<<18) //source width 32bit
				| (2<<21) //dest. width 32bit
				| (1<<27) //dest increment
				;
	//GPDMA block section
	GPDMA_Init();
	// Setup GPDMA Channel
	GPDMACfg.ChannelNum = 2;
	GPDMACfg.SrcMemAddr = DAC_MEM_DIRECTION;
	GPDMACfg.DstMemAddr = (uint32_t) &(LPC_DAC->DACR);
	GPDMACfg.TransferSize = SAMPLE_SIZE;
	GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2P;
	GPDMACfg.SrcConn = 0;
	GPDMACfg.DstConn = GPDMA_CONN_DAC;
	GPDMACfg.DMALLI = (uint32_t) &LLI1;
	GPDMA_Setup(&GPDMACfg);

	//CHANNEL 1 ----------------------------------------------
	GPDMACfg.ChannelNum = 1;
	GPDMACfg.SrcMemAddr = (uint32_t)(wave_form);
	GPDMACfg.DstMemAddr = DAC_MEM_DIRECTION;
	GPDMACfg.TransferSize = (SAMPLE_SIZE);
	GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2M;
	GPDMACfg.SrcConn = 0;
	GPDMACfg.DstConn = 0;
	GPDMACfg.DMALLI = 0;
	GPDMACfg.TransferWidth = 2;

	GPDMA_Setup(&GPDMACfg);

	//CHANNEL 0 -----------------------------------------------
	GPDMACfg.ChannelNum = 0;
	GPDMACfg.SrcMemAddr = (uint32_t)(ADC_Buffer);
	GPDMACfg.DstMemAddr = ADC_MEM_DIRECTION;
	GPDMACfg.TransferSize = 0xFFF;
	GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2P;
	GPDMACfg.SrcConn = 0;
	GPDMACfg.DstConn = GPDMA_CONN_DAC;
	GPDMACfg.DMALLI = (uint32_t) &LLI2;
	GPDMACfg.TransferWidth = 2;

	GPDMA_Setup(&GPDMACfg);

	//GPDMA_ChannelCmd(0, ENABLE);
	//GPDMA_ChannelCmd(1, ENABLE);
	//GPDMA_ChannelCmd(2, ENABLE);

}

void configTMR0(void){
	LPC_SC->PCONP |= (1 << 1);
	LPC_SC->PCLKSEL0 |= (1 << 2); // PCLK = cclk

	LPC_TIM0 -> PR = 24;  //Tick every 250ns
	LPC_TIM0 -> MR1 = 125;//Match on 31.25us
	LPC_TIM0 -> MCR |= (1<<4); //Reset on Match
	LPC_TIM0 -> EMR |= (1<<1) | (11<<6); //Toggle MA0.1
	LPC_TIM0 -> IR |= 0x3F;      // Clear all interrupt flag
	LPC_TIM0 -> TCR |= 3; 		//RESET Timer0
	LPC_TIM0 -> TCR ^= 2;
	//CTCR -> 00 on reset
	//
	//NVIC_EnableIRQ(TIMER0_IRQn);
}


void generate_waveform(void){
	int i;
	for(i=0; i<=1023; i++){
		if (i<512){
			wave_form[i] = (i+512);
		}
		else{
			wave_form[i] = (i-512);
		}
		wave_form[i] = (wave_form[i]<<6);
	}
}

void EINT0_IRQHandler(void){

}

void ADC_IRQHandler(){
	static int i;

	if(i<=4095){
		ADC_Buffer[i] = ((LPC_ADC -> ADDR1)&(0xFFF8));
		i++;
	}
	else{
		i=0;
		ADC_Buffer[i] = ((LPC_ADC -> ADDR1)&(0xFFF8));
		i++;
	}

		LPC_ADC -> ADGDR &= LPC_ADC -> ADGDR;

}

void TIMER0_IRQHandler(){
	int i;
	i = 3;
}

