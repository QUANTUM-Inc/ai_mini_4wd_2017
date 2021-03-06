/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#ifndef UTILS_H_
#define UTILS_H_

/*---------------------------------------------------------------------------*/
#ifndef NULL
#define NULL 0
#endif

/*---------------------------------------------------------------------------*/
#define BIT(x)		(1 << x)

/*---------------------------------------------------------------------------*/
typedef uint8_t BOOL_T;
#define BOOL_TRUE						(1)
#define BOOL_FALSE						(0)

/*---------------------------------------------------------------------------*/
/* Interrupt controller */
#define Enable_Int()        {PMIC.CTRL = PMIC_HILVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm; sei();}
#define Disable_Int()       cli()


#endif /* UTILS_H_ */