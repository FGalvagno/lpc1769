#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"

void configPin(void);
void configADC(void);

uint32_t GetVU(uint16_t value);

int main(void) {
	configPin();
	configADC();

    while(1);
    return 0;
}

void configPin(void){
	PINSEL_CFG_Type pinCfg;
	pinCfg.Funcnum = 1;
	pinCfg.OpenDrain = 0;
	pinCfg.Pinmode = 0;
	pinCfg.Portnum = 0;
	pinCfg.Pinnum = 23;
	PINSEL_ConfigPin(&pinCfg);

	//P1(18:27) as outputs for LEDS GPIO
	pinCfg.Funcnum = 0;
	pinCfg.OpenDrain = 0;
	pinCfg.Pinmode = 2;
	pinCfg.Portnum = 1;

	int pin;
	for(pin = 18;  pin <= 28; pin++){
		pinCfg.Pinnum = pin;
		PINSEL_ConfigPin(&pinCfg);
	}

	GPIO_SetDir(1, 0X0FFC0000, 1); //Set as Output
}

void configADC(void){
	ADC_Init(LPC_ADC, 80000);                             //Power-Up ADC, 80Khz
	ADC_BurstCmd(LPC_ADC, ENABLE);
	ADC_ChannelCmd(LPC_ADC, 0, ENABLE);
	ADC_ChannelCmd(LPC_ADC, 1, ENABLE);
	//LPC_ADC->ADGDR &= LPC_ADC->ADGDR;
	//ADC_IntConfig(LPC_ADC, ADC_ADINTEN0, ENABLE);
	ADC_IntConfig(LPC_ADC, ADC_ADINTEN1, ENABLE);		//Interrupt on ADC finish.
	NVIC_EnableIRQ(ADC_IRQn);
}

void ADC_IRQHandler(void){
	uint16_t data;
	//VUmeter chann 0
	data = ADC_ChannelGetData(LPC_ADC, 0);
	LPC_GPIO1 -> FIOMASK = !(0x003C0000);
	uint32_t port_value = GetVU(data)<<18;
	LPC_GPIO1 -> FIOPIN  = port_value;
	//VUmeter chann 1
	data = ADC_ChannelGetData(LPC_ADC, 1);
	LPC_GPIO1 -> FIOMASK = !(0x0FC00000);
	port_value = GetVU(data)<<23;
	LPC_GPIO1 -> FIOPIN  = port_value;
}

uint32_t GetVU(uint16_t value){
	uint8_t temp = 0;
	uint8_t i;
	for(i=0; i<value/819 ;i++){
		temp |= 1<<i;
	}
	return temp;
}
