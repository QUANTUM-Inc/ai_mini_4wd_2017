/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#ifndef TINYX4A_ADC_H_
#define TINYX4A_ADC_H_

#define ADC_OK								(0)
#define ADC_UNKNOWN_REF						(-41)

typedef enum {
	ADC_CH0 = 0,
	ADC_CH1 = 1,
	ADC_CH2 = 2,
	ADC_CH3 = 3,
} TINY_X4A_ADC_CH;

typedef enum {
	ADC_REF_VCC = 0,
	ADC_REF_1_1V = 1
} TINY_X4A_ADC_REF;


int8_t adcInit(TINY_X4A_ADC_REF ref, uint8_t ch_bitmap);
int8_t adcReadPhase1(TINY_X4A_ADC_CH ch);
int8_t adcReadPhase2(uint16_t *val);
int8_t adcRead(TINY_X4A_ADC_CH ch, uint16_t *val);


#endif /* TINYX4A_ADC_H_ */