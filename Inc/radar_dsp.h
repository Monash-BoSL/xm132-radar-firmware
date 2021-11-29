#ifndef Radar_dsp_H
#define Radar_dsp_H

#include "acc_hal_integration.h"

#define CONVKER 9//must be odd

#if (CONVKER % 2) != 1
	#error CONVKER must be odd
#endif

typedef struct uint8v2_t{
	uint8_t x1;
	uint8_t x2;	
}uint8v2_t;

typedef struct floatv2_t{
	float x1;
	float x2;	
}floatv2_t;

float kernel[CONVKER];
float convstack[CONVKER];

uint16_t** _conv_data;
uint8_t _sweeps;
uint8_t _bins;

void dcdatarm(uint16_t**, uint8v2_t);
float dofft(uint16_t**, uint8v2_t);
void dobandstop(uint16_t**, uint8v2_t, uint32_t);
void doconv(uint16_t**, uint8v2_t, float);

void makekernel(float);
int8_t convolve1d(uint16_t, uint8_t);

uint16_t getdata(int16_t, int16_t);
void setdata(int16_t, int16_t, uint16_t);
void stackSet(void);
float stackPush(float);

uint8v2_t max2d(uint16_t**, uint8v2_t);
float get_msd(uint16_t**, uint8v2_t, uint8v2_t, float);
void null_data(uint16_t**, uint8v2_t, uint8v2_t, float);
floatv2_t center_of_mass(uint8v2_t, uint8_t);











#endif



