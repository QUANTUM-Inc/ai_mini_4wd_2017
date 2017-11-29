/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#ifndef ADNS_9800_H_
#define ADNS_9800_H_

#define ADNS9800_SROM_VERSION					(0xA6)

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
typedef struct Adn9800MotionData_t
{
	uint8_t motion;
	uint8_t observation;
	int16_t delta_x;
	int16_t delta_y;
	uint8_t surface_quality;

	uint8_t pixel_sum;
	uint8_t maximum_pixel;
	uint8_t minimum_pixel;
	uint8_t shutter_upper;
	uint8_t shutter_lower;
	uint8_t frame_period_upper;
	uint8_t frame_period_lower;
} Adn9800MotionData;


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
int8_t adns9800_prove(void);


/*---------------------------------------------------------------------------*/
int8_t adns9800_getValue(Adn9800MotionData *motion_data);
int8_t adns9800_getProductId(uint8_t *product_id);
int8_t adns9800_getInvProductId(uint8_t *inv_product_id);
int8_t adns9800_getRevisionId(uint8_t *revision_id);
int8_t adns9800_getSromId(uint8_t *srom_id);

#endif /* ADNS-9800_H_ */