#define HALF_STEP
#include "st_basic.h"

unsigned int GetADCResult(void);

const int stepperPin[4] = { 2, 3, 6, 7 };
const unsigned int stepperFullState[4][4] = { { 1, 1, 0, 0 },
                                              { 0, 1, 1, 0 },
																							{ 0, 0, 1, 1 },
																							{ 1, 0, 0, 1 } };
const unsigned int stepperHalfState[8][4] = { { 1, 0, 0, 0 },
                                              { 1, 1, 0, 0 },
																			    		{ 0, 1, 0, 0 },
																			    		{ 0, 1, 1, 0 },
																			    		{ 0, 0, 1, 0 },
																				    	{ 0, 0, 1, 1 },
																				    	{ 0, 0, 0, 1 },
																				    	{ 1, 0, 0, 1 } };
int currentStep = 0;

void ADC1_Init(void);
void Step(int step);

int main(void)
{
	ClockInit();
	ADC1_Init();
	for (int i = 0; i < 4; i++)
		GPIO_Init(GPIOB, stepperPin[i], GPIO_OUTPUT);
	
	while (1)
	{
		ADC1->CR |= ADC_CR_ADSTART;	//: Write 1 to start regular conversions. Read 1 means that the ADC is operating and eventually converting a regular channel.
		while (ADC1->CR & ADC_CR_ADSTART);	//0: No ADC regular conversion is ongoing.
		unsigned int ADC_Value = ADC1->DR;	//They contain the conversion result from the last converted regular channel.
		
		unsigned int speed = (ADC_Value > 2047) ? ADC_Value - 2047 : 2047 - ADC_Value;
		if (speed > 50)
		{
			Step((ADC_Value > 2047) ? 1 : -1);
			Delay(2047 / speed);
		}
	}
}

void ADC1_Init(void)
{
	RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;	//ADC Clock Enabled
	
	GPIO_Init(GPIOA, 5, GPIO_ANALOG);
	GPIOA->ASCR |= GPIO_ASCR_ASC5;
	
	ADC1->SMPR2 = (5 << ADC_SMPR2_SMP10_Pos);	//92.5 ADC clock cycles
	ADC1->SQR1 = (10 << ADC_SQR1_SQ1_Pos);	//1st conversion = channel 10
	ADC123_COMMON->CCR = ADC_CCR_PRESC_3 | ADC_CCR_PRESC_0	//input ADC clock divided by 64
                     | ADC_CCR_CKMODE_1 | ADC_CCR_CKMODE_0;	//HCLK/4 (Synchronous clock mode)
	
	ADC1->CR = ADC_CR_ADVREGEN;	//ADC Voltage regulator enabled.
	Delay(1);
	ADC1->CR |= ADC_CR_ADCAL;	//Write 1 to calibrate the ADC. Read at 1 means that a calibration in progress.
	while (ADC1->CR & ADC_CR_ADCAL);	//0: Calibration complete
	ADC1->CR |= ADC_CR_ADEN;	//Write 1 to enable the ADC.
	while (!(ADC1->ISR & ADC_ISR_ADRDY));	//ADC is ready to start conversion
}

void Step(int step)
{
	int direction = (step > 0) ? 1 : -1; // if step > 0 -> direction : 1, if step <= 0 -> direction : -1
	while (step != 0)
	{
#ifdef FULL_STEP // if 'FULL_STEP' is defined
		currentStep = (currentStep + direction + 4) % 4; // calculate currentStep for 'FULL_STEP' case
#elif defined HALF_STEP // else if 'HALF_STEP' is defined
		currentStep = (currentStep + direction + 8) % 8; // calculate currentStep for 'HALF_STEP' case
#endif

		for (int i = 0; i < 4; i++)
		{
#ifdef FULL_STEP
			GPIOB->BSRR = (stepperFullState[currentStep][i] << stepperPin[i]);
			GPIOB->BSRR = (!stepperFullState[currentStep][i] << (stepperPin[i] + 16));
#elif defined HALF_STEP
			GPIOB->BSRR = (stepperHalfState[currentStep][i] << stepperPin[i]); // set pin through controling BSRR
			GPIOB->BSRR = (!stepperHalfState[currentStep][i] << (stepperPin[i] + 16)); // reset pin through controling BSRR
#endif
		}

		step -= direction;
		Delay(2);
	}
}
