/*
 * example.h
 *
 *  Created on: 03.03.2019
 *      Author: harebit
 */

#ifndef EXAMPLE_H_
#define EXAMPLE_H_

#include "stdlib.h"
#include "stdint.h"
#include "string.h"
#include "Nextion.h"
#ifdef __cplusplus

extern volatile char Transfer_cplt;

void p2b0PopCallback(void *ptr);
void p3b0PopCallback(void *ptr);

extern "C" void setup(void);
extern "C" void loop(void);


//

#endif // c++
#endif /* EXAMPLE_H_ */
