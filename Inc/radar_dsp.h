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

typedef struct uint16v2_t{
	uint16_t x1;
	uint16_t x2;	
}uint16v2_t;

typedef struct floatv2_t{
	float x1;
	float x2;	
}floatv2_t;

float kernel[CONVKER];
float convstack[CONVKER];

uint16_t** _conv_data;
uint16_t _sweeps;
uint16_t _bins;

void dcdatarm(uint16_t**, uint16v2_t);
void detrend(uint16_t** data, uint16v2_t data_size);
float dofft(uint16_t**, uint16v2_t);
void dobandstop(uint16_t**, uint16v2_t, uint32_t);
void doconv(uint16_t**, uint16v2_t, float);

void makekernel(float);
int8_t convolve1d(uint16_t, uint8_t);

uint16_t getdata(int16_t, int16_t);
void setdata(int16_t, int16_t, uint16_t);
void stackSet(void);
float stackPush(float);

uint16v2_t max2d(uint16_t**, uint16v2_t);
float get_msd(uint16_t**, uint16v2_t, uint16v2_t, float);
void null_data(uint16_t**, uint16v2_t, uint16v2_t, float);
floatv2_t center_of_mass(uint16v2_t, uint16_t);

////envelope ////

void getpeaks(uint16_t**, uint16_t, uint16_t*, uint16_t*, uint16_t);
uint16_t next_peak(uint16_t**, uint16_t, uint16_t, uint16_t);
void insert(uint16_t*, uint16_t, uint16_t, uint8_t);
void pack16to32array(uint32_t*, uint16_t*, uint16_t*);

//// accumulant ////

void set_accumulant(float v);
void load_accumulant(void);
void add_accumulant(float v);
void mult_accumulant(float v);
void acc_accumulant(void);
void sq_acc_accumulant(void);
void sqrt_accumulant(void);
void add_data(int16_t v);








#endif



