// #include "arm_math.h"

#include "util.h"
#include "arduinoFFTfix.h"

#include <stdio.h>
#include <math.h>

#include "radar_dsp.h"

void dcdatarm(uint16_t** data, uint8v2_t data_size){
	//remove dc component
	uint8_t sweeps = data_size.x1;
	uint8_t bins = data_size.x2;
	for(uint16_t i = 0; i<bins; i++){
		uint32_t accumulator = 0;
		for(uint16_t j = 0; j<sweeps; j++){
			accumulator += data[j][i];
		}

		uint32_t average = accumulator/sweeps;
		
		for(uint16_t j = 0; j<sweeps; j++){
			data[j][i] -= average;

		}
	}
}

//do fft on each row of data
float dofft(uint16_t** data, uint8v2_t data_size){
	uint8_t sweeps = data_size.x1;
	uint8_t bins = data_size.x2;
	
	float scales[bins];
	int16_t real[sweeps];
	int16_t imag[sweeps];
	
	for(uint16_t i = 0; i<bins; i++){
		//note that this method of copying data to another array to do the FFT becomes less relatively memory efficient the fewer distance bins you have.
		
		//set real componet of FFT to data
		for (uint16_t j = 0; j < sweeps; j++) {
		  real[j] = data[j][i];
		}
		//set imaginary componet of FFT to zero
		for (uint16_t j = 0; j < sweeps; j++) {
		  imag[j] = 0;
		}
		
		//Compute FFT
		scales[i] = fftRangeScaling(real, sweeps);
		fftWindowing(real, sweeps, FFT_FORWARD); //0.311s
		fftCompute(real, imag, sweeps, FFT_FORWARD);//0.758s
		fftComplexToMagnitude(real, imag, sweeps);//0.135
		
		//load data back in to data array
		//set real componet of FFT to data
		for (uint16_t j = 0; j < sweeps/2; j++) {
		  data[j][i] = real[j];
		}
		for (uint16_t j = sweeps/2; j < sweeps; j++) {
		  data[j][i] = 0;
		}
	}
	//the data has been normalised in each row but to make meaningful comparisons we need in normalised across the entire array, we shall do this now

	//we will normalise by scaling everything realtive to the smallest scaling
	
	//first to find it
	float min_scale = scales[0];
	for(uint16_t i =1; i<bins; i++){
		if (scales[i] < min_scale){
			min_scale = scales[i];		
		}
	}
	
	//now just multiply each row by the relevant scaling factor
	for(uint16_t i =0; i<bins; i++){
		float scaling_factor = min_scale/scales[i]; 
		for(uint16_t j = 0; j < sweeps/2; j++){
			data[j][i] *= scaling_factor;
		}
	}
	return min_scale;
}

//zeros out certain bins (distances) based on mask
void dobandstop(uint16_t** data, uint8v2_t data_size, uint32_t mask){
	uint8_t sweeps = data_size.x1;
	uint8_t bins = data_size.x2;
	
	for(uint16_t j = 0; j<sweeps/2; j++){
		if(mask & (1<<j)){
			for(uint16_t i = 0; i<bins; i++){
					data[j][i] = 0;
			}
		}
	}
}

void doconv(uint16_t** data, uint8v2_t data_size, float st_dev){
	
	_conv_data = data;
	_sweeps = data_size.x1;
	_bins = data_size.x2;
	
	//gaussian filter, seperably calulated
	makekernel(st_dev);
	
	for(uint16_t i = 0; i < _bins; i++){
		convolve1d(i,0);
	}
	for(uint16_t j = 0; j < _sweeps/2; j++){
		convolve1d(j,1);
	}
}


void makekernel(float st_dev){
	float norm = 0;
	for(uint16_t i = 0; i < CONVKER; i++){
		float t = (i - (CONVKER-1)/2)/(st_dev);
		t = -t*t/2;
		kernel[i] =  expf(t);
		norm += kernel[i];
	}
	float sclfact = 1/norm;
	for(uint16_t i = 0; i < CONVKER; i++){
		kernel[i] *= sclfact;
	}
}

int8_t convolve1d(uint16_t indx, uint8_t dir){
	stackSet();
	uint8_t cent = (CONVKER-1)/2;
	if(dir == 0){	
		// stackSet();
		if(indx >= _bins){
			ERR_PRINTLN("bin count too great to convolve");
			return 0;
		}
		for(int i = 0; i< _sweeps/2 + CONVKER-2; i++){
			float sum = 0.0f;
			float pop = 0.0f;
			for(int j = 0; j < CONVKER; j++){
				sum += kernel[j]*getdata(i-j+cent, indx);
			}
			pop = stackPush(sum);
			setdata(i-cent, indx, pop);
		}
		//setdata(0, 0);

	}else{
		// stackSet();
		if(indx >= _sweeps){
			ERR_PRINTLN("sweep count too great to convolve");
			return 0;
		}
		for(int i = 0; i< _bins + CONVKER-2; i++){
			float sum = 0.0f;
			float pop = 0.0f;
			for(int j = 0; j < CONVKER; j++){
				sum += kernel[j]*getdata(indx, i-j+cent);
			}
			pop = stackPush(sum);
			setdata(indx, i-cent, pop);
		}
		//setdata(0, 0);	
	}
	return 1;
}


uint16_t getdata(int16_t sweep, int16_t bin){
	if(
		(sweep >= 0) && (sweep < _sweeps) &&
		(bin >= 0) && (bin < _bins)
	){
		return _conv_data[sweep][bin];
	}else{
		return 0;
	}
}

void setdata(int16_t sweep, int16_t bin, uint16_t val){
	if(
		(sweep >= 0) && (sweep < _sweeps) &&
		(bin >= 0) && (bin < _bins)
	){
		_conv_data[sweep][bin] = val;
	}
}

void stackSet(void){
	for(int i = 0; i < CONVKER; i++){
	convstack[i] = 0.0f;
	}
}

float stackPush(float val){
	float popped = convstack[0];
	for(int i = 0; i < CONVKER-1; i++){
	convstack[i] = convstack[i+1];
	}
	convstack[(CONVKER-1)/2 -1] = val;
	return popped;
}

uint8v2_t max2d(uint16_t** data, uint8v2_t data_size){
	uint8_t sweeps = data_size.x1;
	uint8_t bins = data_size.x2;
	
	uint16_t apex = 0;
	uint8_t mbin = 0;
	uint8_t msweep = 0;
	
	for(uint16_t i = 0; i<bins; i++){
		for(uint16_t j = 0; j<sweeps/2; j++){
				if(data[j][i] > apex){
					apex = data[j][i];
					mbin = i;
					msweep = j;
				}
		}
		}
	uint8v2_t max_index = {msweep,mbin};
	return max_index;
}

float get_msd(uint16_t** data, uint8v2_t data_size, uint8v2_t max, float threshold){
	uint8_t sweeps = data_size.x1;
	uint8_t bins = data_size.x2;
	
	uint8_t msweep = max.x1;
	uint8_t mbin = max.x2;
	
	float meansqdist = 0.0f;
	float mass = 0.0f;
	
	uint16_t apex = data[msweep][mbin];

	uint16_t halfpex = (uint16_t)(apex * threshold);
	//calulate the mean square distance from peak if above half max
	for(int16_t i = 0; i<bins; i++){
	for(int16_t j = 0; j<sweeps/2; j++){
		if (data[j][i] > halfpex){
			mass += data[j][i];
			uint32_t dist = ((j-msweep)*(j-msweep) + (i-mbin)*(i-mbin));
			meansqdist += (float)abs(data[j][i])*(float)(dist);
		}
	}
	}
	if(mass != 0.0f){
		meansqdist /= mass;
	}
	return meansqdist;
}

void null_data(uint16_t** data, uint8v2_t data_size, uint8v2_t max, float threshold){
	uint8_t sweeps = data_size.x1;
	uint8_t bins = data_size.x2;
	
	uint8_t msweep = max.x1;
	uint8_t mbin = max.x2;
	
	uint16_t apex = data[msweep][mbin];
	uint16_t halfpex = apex*threshold;
	
	for(uint16_t i = 0; i<bins; i++){
	for(uint16_t j = 0; j<sweeps/2; j++){
		if(data[j][i] > halfpex){
			data[j][i] = 0;
		}
	}
	}
}

floatv2_t center_of_mass(uint8v2_t max, uint8_t r){
	uint8_t msweep = max.x1;
	uint8_t mbin = max.x2;
	
	float mass = 0.0f;	
	floatv2_t center = {0.0f,0.0f};
	
	for(int16_t i = mbin-r; i<=mbin+r; i++){
	for(int16_t j = msweep-r; j<=msweep+r; j++){
			mass += getdata(j,i);
			center.x1 += (float)getdata(j,i)*(float)j;
			center.x2 += (float)getdata(j,i)*(float)i;
	}
	}
	if(mass != 0.0f){	
		center.x1 /= mass;
		center.x2 /= mass;
	}
	return center;
}


//////////////envelope methods

void getpeaks(uint16_t** data, uint16_t data_len, uint16_t* indexes, uint16_t* amplitudes, uint16_t min_sep){
	const int n = 4;//number of peaks to find
	uint16_t bins = data_len;
	
	for(uint8_t i = 0; i < n; i++){
		indexes[i] = 0;
		amplitudes[i] = 0;
	}
	
	for(uint16_t i = 0; i < bins; i++){
		uint8_t insert_indx = n;
		uint16_t nxpeak = next_peak(data, bins, i, min_sep);
		if (nxpeak == 0){
			for(int8_t j = n-1; j >= 0; j--){
				if(data[0][i] > amplitudes[j]){insert_indx = j;}
				else{break;}
			}
			if(insert_indx < 4){
				insert(indexes, n, i,insert_indx);
				insert(amplitudes, n, data[0][i],insert_indx);
				i += min_sep;
			}
		}else{
			i = nxpeak;
		}
	}
}

uint16_t next_peak(uint16_t** data, uint16_t bins, uint16_t index, uint16_t min_sep){
	uint16_t amp = data[0][index];
	uint16_t indexend = MIN(bins,index + min_sep);
	for(uint16_t i = index; i < indexend; i ++){
		if(amp < data[0][i]){return i;}
	}
	return 0;
}

void insert(uint16_t* a, uint16_t a_len, uint16_t v, uint8_t indx){
	const int n = a_len;//number of peaks to find
	for(int8_t i = n-1; i > indx; i-- ){
		a[i] = a[i-1];
	}
	a[indx] = v;
}

void pack16to32array(uint32_t* a, uint16_t* b, uint16_t* c){
	const int n = 4;//number of peaks to find
	for(uint8_t i = 0; i < n; i++ ){
		a[i] = (((uint32_t)b[i])<<16) | c[i];
	}
}