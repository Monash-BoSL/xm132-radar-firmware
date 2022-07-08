#include "reg_interface.h"


#include "stm32g0xx_it.h"
#include "stm32g0xx_hal.h"

#include "main.h"
#include "util.h"
#include "radar_dsp.h"

#include <stdio.h>
#include <string.h>




extern UART_HandleTypeDef huart1;

//okay so i think i want to set up multiple servies to range stich.
//we probably need 2 or 3 ranges. I guess we will see.

//registers as defined in xm132 module software
//ADDITIONAL REGISTERS
//0x0A :: sleepMCU
//0x41 ::											:: envelope measurement repeats
//-------------------------
//0xD0 :: Velocity (mm/s)
//0xD1 :: Distance (mm)
//0xD2 :: Amplitude (arb)
//0xD3 :: Mean Square Distance (arb)
//0xD4 :: Mean Square Distance threshold (x1000) 	:: min peak seperation
//0xD5 :: Gaussian Kernal StDev (x1000)
//0xD6 :: Data Eval Mode
//0xD7 :: Focus weight Radius
//0xD8 :: Data zeroing threshold
//0xD9 :: Bandstop velocity filter
//0xDA :: DSP command buffer
//0xDB :: DSP value

//ADDITIONAL COMMANDS
//0x03 05    :: perform measurment
//0x03 06 	 :: evaluate data
//0x03 07    :: accumulant dsp
//0xFA E9    :: dump 256 bytes of data from buffer with given offset
uint32_t* RegInt_regmap(uint8_t reg){
	REGADRERR = -1;
	uint32_t* ptr = &REGADRERR;
	
	if (reg <= 0x12){
		ptr = &GENERAL_REGISTERS[reg];
	}
	if (reg == 0xE9){
		ptr = &OUTPUT_BUFFER_LENGTH;
	}
	if (0x20 <= reg && reg <= 0x42){
		ptr = &SERVICE_REGISTERS[reg - 0x20];
	}
	if (0x81 <= reg && reg <= 0x85){
		ptr = &META_REGISTERS[reg - 0x81];
	}	
	if (0xD0 <= reg && reg <= 0xD9){
		ptr = &EVAL_REGISTERS[reg - 0xD0];
	}
	return ptr;
}

uint32_t RegInt_getreg(uint8_t reg){
	uint32_t* regptr = RegInt_regmap(reg);
	return *regptr;
}

int8_t RegInt_setreg(uint8_t reg, uint32_t val){
	int8_t success = RegInt_setregf(reg, val, 0);
	return success;
}

int8_t RegInt_setregf(uint8_t reg, uint32_t val, uint8_t force){
	if(!force){
        if (!RegInt_writeable(reg)){DBG_PRINTLN("not writable");return 0;}
	}
    
	uint32_t* regptr = RegInt_regmap(reg);
	if (!(*regptr == (uint32_t)-1)){
		*regptr = val;
	}
    
	return 1;
}

int8_t RegInt_writeable(uint8_t reg){
    //if service is created, lock mode
    if(reg == 0x02){
        if(RegInt_getreg(0x06) && 0x00000001){
            return 0;
        }
    }
    
    //read only addresses
    uint8_t read_only_addr[10] = {0x06, 0x10, 0x11, 0x12, 0x81, 0x82,0x83,0x84,0x85, 0xE9}; //please have this list be ordered.
	for(uint8_t i = 0; i < 10; i++){
		if (reg == read_only_addr[i]){return 0;}
		if (reg < read_only_addr[i]){break;}
	}   
    return 1;
}

void Reg_regand(uint8_t reg, uint32_t andbits){
	uint32_t flags = RegInt_getreg(reg); 
	flags &= andbits;
	RegInt_setregf(reg, flags, 1);
}

void Reg_regor(uint8_t reg, uint32_t orbits){
	uint32_t flags = RegInt_getreg(reg); 
	flags |= orbits;
	RegInt_setregf(reg, flags, 1);
}

//begin listning on UART
void RegInt_Init(void){
	queue_cmd_end = 0;
	for(uint8_t i = 0; i < 0xFF; i++){
		// if(i == 3){continue;}//writing to this reg controlls the RSS.//remove if works
		RegInt_setregf(i, 0, 1);
	}
	RegInt_setregf(0x07, 115200, 1);//set default baud rate
	RegInt_setregf(0x0A, 0, 1);//set default baud rate
	RegInt_setregf(0x10, HARDWARE_REVISION, 1);//set product identification register
	RegInt_setregf(0x11, FIRMWARE_REVISION, 1);//set firmware revision register
	RegInt_setregf(0xD4, 600, 1);//set default mean sq distance threshold
	RegInt_setregf(0xD5, 1000, 1);//set default radius for gf kernal
	RegInt_setregf(0xD6, 0x0000000F, 1);//set default eval mode
	RegInt_setregf(0xD7, 0x00000002, 1);//set default radius for averaging
	RegInt_setregf(0xD8, 500, 1);//sets theshold data zeroing
	RegInt_setregf(0xD9, 0x00000000, 1);//sets no bandstop
	
	uart_state = 0;

	HAL_UART_Receive_IT(&huart1, uart_rx_buff, 1);
	DBG_PRINTLN("Registers Initialised");
}

void RegInt_parsecmd(void){
	if (uart_state != 4){return;}
//read	
	if (uart_rx_buff[0] == 0xF8 && cmd_length == 1){
		
		uint8_t reg = uart_rx_buff[1];
		uint32_t val = RegInt_getreg(reg);
		uart_tx_buff[0] = 0xCC;
		uart_tx_buff[1] = 0x05;
		uart_tx_buff[2] = 0x00;
		uart_tx_buff[3] = 0xF6;
		uart_tx_buff[4] = reg;
		uart_tx_buff[5] = get_byte(val,0);
		uart_tx_buff[6] = get_byte(val,1);
		uart_tx_buff[7] = get_byte(val,2);
		uart_tx_buff[8] = get_byte(val,3);
		uart_tx_buff[9] = 0xCD;
		HAL_UART_Transmit_IT(&huart1, uart_tx_buff, 10);
//write
	}else if (uart_rx_buff[0] == 0xF9 && cmd_length == 5){
		uint8_t reg = uart_rx_buff[1];
		uint32_t val = 0;
		for(uint8_t i = 0; i < 4; i++){
			val |= uart_rx_buff[2+i] << (i%4)*8;
		}
		if(RegInt_setreg(reg, val)){RegInt_regaction(reg, val);}
        // RegInt_setreg(reg, val);
		val = RegInt_getreg(reg);
		uart_tx_buff[0] = 0xCC;
		uart_tx_buff[1] = 0x05;
		uart_tx_buff[2] = 0x00;
		uart_tx_buff[3] = 0xF5;
		uart_tx_buff[4] = reg;
		uart_tx_buff[5] = get_byte(val,0);
		uart_tx_buff[6] = get_byte(val,1);
		uart_tx_buff[7] = get_byte(val,2);
		uart_tx_buff[8] = get_byte(val,3);
		uart_tx_buff[9] = 0xCD;
		HAL_UART_Transmit_IT(&huart1, uart_tx_buff, 10);
//buffer dump
	}else if (uart_rx_buff[0] == 0xFA && ((uart_rx_buff[1] == 0xE8) || (uart_rx_buff[1] == 0xE9)) && cmd_length == 3){
		uint8_t offst_h = 0;
		uint8_t offst_l = 0;
		offst_l = uart_rx_buff[2];
		offst_h = uart_rx_buff[3];
		uint16_t offst = (offst_h << 8) | offst_l;

		bufflen = (sparse_metadata.data_length)*sizeof(uint16_t);
		bufflen_far = (sparse_metadata_far.data_length)*sizeof(uint16_t);
	
		uint32_t datalen;
		if(uart_rx_buff[1] == 0xE8){
        DBG_PRINTINT(bins);
        DBG_PRINTINT(sweeps);
		datalen = sweeps*bins*sizeof(uint16_t);
		}else{
		datalen = 128*sizeof(uint16_t);
		}
		
		uart_tx_buff[0] = 0xCC;
		uart_tx_buff[1] = get_byte(datalen+1,0);
		uart_tx_buff[2] = get_byte(datalen+1,1);
		uart_tx_buff[3] = 0xF7;
		uart_tx_buff[4] = uart_rx_buff[1];
		
		DBG_PRINTLN("buffer transmitt");
		DBG_PRINTINT(datalen);
		DBG_PRINTINT(bufflen);
		DBG_PRINTINT(bufflen_far);
		
		HAL_UART_Transmit(&huart1, uart_tx_buff, 5, 10);
		queue_cmd_end = 1;
		DBG_PRINTINT(queue_cmd_end);
		HAL_UART_Transmit_IT(&huart1, ((uint8_t*) *data + offst), datalen);
	}
	uart_state = 0;
	HAL_UART_Receive_IT(&huart1, uart_rx_buff, 1);

}

void RegInt_regaction(uint8_t reg, uint32_t val){
    //main control
	if(reg == 0x03){
		rss_control(val);
	}
	if(reg == 0x07){
		changeUART1baud(val);
	}
    if(reg == 0x0A){
		sleepMCU(val);
	}
}


void Reg_store_sparse_metadata(acc_service_sparse_metadata_t metadata, acc_service_sparse_metadata_t* metadata_far_ptr){

	RegInt_setregf(0x81, (uint32_t)(metadata.start_m * 1000.0f),1);
	RegInt_setregf(0x82, (uint32_t)(metadata.length_m * 1000.0f),1);
	uint32_t bufflen = metadata.data_length;
	if(metadata_far_ptr != NULL){
		bufflen += (*metadata_far_ptr).data_length;
	}
	RegInt_setregf(0x83, bufflen ,1);
	RegInt_setregf(0x84, (uint32_t)(metadata.sweep_rate * 1000.0f),1);
	RegInt_setregf(0x85, (uint32_t)(metadata.step_length_m * 1.0e6f),1);
}

void Reg_store_envelope_metadata(acc_service_envelope_metadata_t metadata){
    
	RegInt_setregf(0x81, (uint32_t)(metadata.start_m * 1000.0f),1);
	RegInt_setregf(0x82, (uint32_t)(metadata.length_m * 1000.0f),1);
	RegInt_setregf(0x83, (uint32_t)metadata.data_length ,1);
	RegInt_setregf(0x84, (uint32_t)metadata.stitch_count,1);
	RegInt_setregf(0x85, (uint32_t)(metadata.step_length_m * 1.0e6f),1);
   
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if (uart_state == 0){
		if (uart_rx_buff[0] == 0xCC){
			uart_state = 1;
			cmd_length = 0;
			HAL_UART_Receive_IT(huart, uart_rx_buff, 2);
		}else{
			HAL_UART_Receive_IT(huart, uart_rx_buff, 1);
		}
	}else if (uart_state == 1){
		cmd_length = (uart_rx_buff[0]) | (uart_rx_buff[1] << 8);
		uart_state = 3;
		if (cmd_length +2 > UART_BUFF){
			uart_state = 0;
			HAL_UART_Receive_IT(huart, uart_rx_buff, 1);
		}
		HAL_UART_Receive_IT(huart, uart_rx_buff, 2 + cmd_length);
	}else if (uart_state == 3){
		uart_state = 4;
		//HAL_UART_Receive_IT(huart, uart_rx_buff, 1);
	}

}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
	if (queue_cmd_end == 2){
		queue_cmd_end = 1;
		//HAL_UART_Transmit_IT(huart, sparse_data_far, bufflen_far);
	}else if(queue_cmd_end == 1){
		queue_cmd_end = 0;
		uint8_t end = 0xCD;
		HAL_UART_Transmit_IT(huart, &end, 1);
	}
}

void changeUART1baud(uint32_t baudrate){
	HAL_UART_DeInit(&huart1);
	MX_USART1_UART_Init(baudrate);
	HAL_UART_Receive_IT(&huart1, uart_rx_buff, 1);
}

void sleepMCU(uint32_t mode){
    if(mode == 0x00000000){return;}
    if(mode == 0x00000001){    
        stopService();
        
        INF_PRINTLN("STM32 Sleep");
        
        
        HAL_SuspendTick();
        HAL_PWR_DisableSleepOnExit();
        HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFE);
        SystemClock_Config();
        HAL_ResumeTick();
        uint32_t baudrate = RegInt_getreg(0x07);
        changeUART1baud(baudrate);
        
        INF_PRINTLN("STM32 Wake");
        
    }
    RegInt_setregf(0x0A,0x00000000,1);
    return;
}

void rss_control(uint32_t val){
	if (val == 0x00){stopService();}
	if (val == 0x01){createService();}
	if (val == 0x02){activateService();}
	if (val == 0x03){
		if(createService()){activateService();}
	}
	if (val == 0x04){Reg_regand(0x06,0x000000FF);}//clear error bits
	if (val == 0x05){measure();}
	if (val == 0x06){evalData();}
	if (val == 0x07){dsp_burst();}
}


void initRSS(void){
	INF_PRINTLN("build at %s %s", __DATE__, __TIME__);
	INF_PRINTLN("of firmware revision: %d.%d.%d", get_byte(FIRMWARE_REVISION,2),get_byte(FIRMWARE_REVISION,1),get_byte(FIRMWARE_REVISION,0) );
	INF_PRINTLN("for hardware revision: %d.%d.%d", get_byte(HARDWARE_REVISION,2),get_byte(HARDWARE_REVISION,1),get_byte(HARDWARE_REVISION,0) );
	
	radar_hal = *acc_hal_integration_get_implementation();
	
	if (!acc_rss_activate(&radar_hal))
	{
		/* handle error */
		ERR_PRINTLN("RSS activation fail");
	}
	acc_rss_override_sensor_id_check_at_creation(true);
	
	sparse_config = acc_service_sparse_configuration_create();
 
	if (sparse_config == NULL)
	{
		ERR_PRINTLN("sparse config creation fail");
	}
	
	sparse_config_far = acc_service_sparse_configuration_create();
 
	if (sparse_config_far == NULL)
	{
		ERR_PRINTLN("far sparse config creation fail");
	}
    
    envelope_config = acc_service_envelope_configuration_create();
 
	if (envelope_config == NULL)
	{
		printf("envelope config creation fail");
	}

}

int8_t envelope_data_malloc(void){
    bins = envelope_metadata.data_length;
    sweeps = 1;
    return data_malloc(sweeps,bins);
}

int8_t sparse_data_malloc(void){
	sweeps = acc_service_sparse_configuration_sweeps_per_frame_get (sparse_config);
	
	bins = sparse_metadata.data_length/sweeps;
	if(far_active){
		bins += sparse_metadata_far.data_length/sweeps;
	}
	accumulant_malloc(sweeps/2,bins);
	return data_malloc(sweeps,bins);
}

int8_t accumulant_malloc(uint16_t sweeps, uint16_t bins){
	//this creates data array where data[i][j] is the i-th sweep and the j-th distance bin.
	DBG_PRINTLN("accumulant malloc sweeps: %d", sweeps);
	DBG_PRINTLN("accumulant malloc bins: %d", bins);
	
	uint32_t len = 0;
	uint16_t r=sweeps, c=bins;//r: sweeps, c: bins
    float *ptr;
 
    len = sizeof(float *) * r + sizeof(float) * c * r;
    accumulant = (float **)malloc(len);
 
	if (accumulant == NULL){
		ERR_PRINTLN("accumulant buffer allociation failed");
		return -1;
	}else{
		DBG_PRINTLN("accumulant buffer allociation success");
		DBG_PRINTLN("accumulant buffer len: %ld", len);
	}
 
    // ptr is now pointing to the first element in of 2D array
    ptr = (float *)(accumulant + r);
 
    // for loop to point rows pointer to appropriate location in 2D array
    for(uint16_t i = 0; i < r; i++){
        accumulant[i] = (ptr + c * i);
	}
	
	return 0;
}

int8_t data_malloc(uint16_t sweeps, uint16_t bins){
	//this creates data array where data[i][j] is the i-th sweep and the j-th distance bin.
	DBG_PRINTLN("malloc sweeps: %d", sweeps);
	DBG_PRINTLN("malloc bins: %d", bins);
	
	uint32_t len = 0;
	uint16_t r=sweeps, c=bins;//r: sweeps, c: bins
    uint16_t *ptr;
 
    len = sizeof(uint16_t *) * r + sizeof(uint16_t) * c * r;
    data = (uint16_t **)malloc(len);
 
	if (data == NULL){
		ERR_PRINTLN("data buffer allociation failed");
		return -1;
	}else{
		DBG_PRINTLN("data buffer allociation success");
		DBG_PRINTLN("data buffer len: %ld", len);
	}
 
    // ptr is now pointing to the first element in of 2D array
    ptr = (uint16_t *)(data + r);
 
    // for loop to point rows pointer to appropriate location in 2D array
    for(uint16_t i = 0; i < r; i++){
        data[i] = (ptr + c * i);
	}
	
	return 0;
}

void data_free(void){
	free(accumulant);
	accumulant = NULL;
	free(data);
	data = NULL;
	sweeps = 0;
	bins = 0;
}

void filldata_sparse(uint8_t far){
	uint16_t bins_near = sparse_metadata.data_length/sweeps;
	uint16_t bins_far;
	if(!far){
		for(uint16_t sweep = 0; sweep < sweeps; sweep++){
			memcpy(data[sweep],sparse_data+(sweep*bins_near), bins_near*sizeof(uint16_t));
		}
	}else{	
		bins_far = sparse_metadata_far.data_length/sweeps;
		for(uint16_t sweep = 0; sweep < sweeps; sweep++){
			memcpy(data[sweep]+bins_near,sparse_data_far+(sweep*bins_far), bins_far*sizeof(uint16_t));
		}
	}	
}

void filldata_envelope(void){
	uint16_t bins = envelope_metadata.data_length;
	memcpy(data[0],envelope_data, bins*sizeof(uint16_t));
}

void updateSparseConfig(acc_service_configuration_t config, uint16_t sweep_start, uint16_t sweep_length){
	DBG_PRINTLN("updating config");
	acc_service_profile_set(config, RegInt_getreg(0x28));
	
	uint32_t rep_mode = RegInt_getreg(0x22); 
	if(rep_mode == 0x01){
		acc_service_repetition_mode_streaming_set(config, ((float)RegInt_getreg(0x23))/1000.0f);
	}else if (rep_mode == 0x02){
		acc_service_repetition_mode_on_demand_set(config);
	}else{
		/* error handle */
	}

    acc_service_tx_disable_set(config, RegInt_getreg(0x26));
		
	acc_service_power_save_mode_set(config, RegInt_getreg(0x25));
	
	acc_service_asynchronous_measurement_set(config,RegInt_getreg(0x33));
	
	acc_service_requested_start_set (config, (float)sweep_start/1000.0f);
	
	acc_service_requested_length_set (config, (float)sweep_length/1000.0f);
	
	acc_service_receiver_gain_set (config, (float)RegInt_getreg(0x24)/1000.0f);
	
	acc_service_hw_accelerated_average_samples_set (config, RegInt_getreg(0x30));
    
    acc_service_maximize_signal_attenuation_set (config, RegInt_getreg(0x32));
	
	acc_service_sparse_configuration_sweeps_per_frame_set (config, RegInt_getreg(0x40));
	
	acc_service_sparse_configuration_sweep_rate_set (config, (float)RegInt_getreg(0x41)/1000.0f);
	
	acc_service_sparse_sampling_mode_set (config, RegInt_getreg(0x42));
	
	acc_service_sparse_downsampling_factor_set (config, RegInt_getreg(0x29));
			
}


//custom register
//0x41 :: Number of measurements (mm/s)
void updateEnvelopeConfig(acc_service_configuration_t config){
    DBG_PRINTLN("updating config");
    acc_service_profile_set(config, RegInt_getreg(0x28));
	
	uint32_t rep_mode = RegInt_getreg(0x22); 
	if(rep_mode == 0x01){
		acc_service_repetition_mode_streaming_set(config, ((float)RegInt_getreg(0x23))/1000.0f);
	}else if (rep_mode == 0x02){
		acc_service_repetition_mode_on_demand_set(config);
	}else{
		/* error handle */
	}
	
    acc_service_tx_disable_set(config, RegInt_getreg(0x26));
    
	acc_service_envelope_downsampling_factor_set(config, RegInt_getreg(0x29));
		
	acc_service_power_save_mode_set(config, RegInt_getreg(0x25));
	
	acc_service_asynchronous_measurement_set(config,RegInt_getreg(0x33));
	
	acc_service_requested_start_set (config, (float)RegInt_getreg(0x20)/1000.0f);
	
	acc_service_requested_length_set (config, (float)RegInt_getreg(0x21)/1000.0f);
	
	acc_service_receiver_gain_set (config, (float)RegInt_getreg(0x24)/1000.0f);
	
	acc_service_hw_accelerated_average_samples_set (config, RegInt_getreg(0x30));
    
    acc_service_envelope_noise_level_normalization_set (config, RegInt_getreg(0x31));
    
    acc_service_maximize_signal_attenuation_set (config, RegInt_getreg(0x32));
    
    acc_service_mur_set (config, RegInt_getreg(0x34));
	
	acc_service_envelope_running_average_factor_set(config, (float)RegInt_getreg(0x40)/1000.0f);
    
}

int8_t createService(void){
    DBG_PRINTLN("creating service");
    uint32_t service_type = RegInt_getreg(0x02);
    int8_t success;
    if(service_type == 0x02){success = createEnvelopeService();}
    else if(service_type == 0x04){success = createSparseService();}
    else {Reg_regor(0x06, 0x0040000); return 0;}
    
    if(success){
        Reg_regor(0x06, 0x00000001);
        return 1;
    }else{
        Reg_regor(0x06, 0x00080000);
        return 0;
    }
}

int8_t createEnvelopeService(void){
    updateEnvelopeConfig(envelope_config);
    // // //gauge free mem with this function
    // // for(int k = 0; k < 0x0F; k++){
    // // unsigned int ptr = (unsigned int)malloc(0x1000);
    // // printf("mem allocation: 0x%08X\n", ptr);
    // // }
    // // while(1){}
   	envelope_handle = acc_service_create(envelope_config);	
	
	if (envelope_handle == NULL){//handles error
		ERR_PRINTLN("envelope service creation failed");
		return 0;
	}else{
		acc_service_envelope_get_metadata(envelope_handle, &envelope_metadata);
		
        if(envelope_data_malloc() == -1){
            DBG_PRINTLN("data buffer allocation failed");
            stopService();
            }

		Reg_store_envelope_metadata(envelope_metadata);
		//this does not work with active far enabled
		
		printf_envelope_metadata(envelope_metadata);
	}
	return 1;
}

int8_t createSparseService(void){
	uint32_t start_reg = roundDown(RegInt_getreg(0x20),60);
	uint32_t len_reg = roundDown(RegInt_getreg(0x21),60);
	if (len_reg < 1891){
		far_active = 0;
        DBG_PRINTLN("updating sparse config");
		updateSparseConfig(sparse_config,start_reg, len_reg);
		//single service will do
	}else if (len_reg < 3811){
		far_active = 1;
		DBG_PRINTLN("updating sparse config");
		updateSparseConfig(sparse_config,start_reg, 1890);
		DBG_PRINTLN("updating sparse far config");
		int16_t far_len; 
		far_len = (len_reg > 1920) ? len_reg-1920 : 1; 
		DBG_PRINTINT(far_len);
		updateSparseConfig(sparse_config_far,start_reg+1920,far_len);
	}else{
		ERR_PRINTLN("sparse service creation failed (too long)");
		return 0;
	}
	sparse_handle = acc_service_create(sparse_config);	
	
	if (sparse_handle == NULL){//handles error
		ERR_PRINTLN("sparse service creation fail");
		return 0;
	}else{
		acc_service_sparse_get_metadata(sparse_handle, &sparse_metadata);
		if(!far_active){
			if(sparse_data_malloc() == -1){
                DBG_PRINTLN("data buffer allocation failed");
                stopService();
                }
		}

		Reg_store_sparse_metadata(sparse_metadata, NULL);
		
		printf_sparse_metadata(sparse_metadata);
	}
	
	if(far_active){
	sparse_handle_far = acc_service_create(sparse_config_far);
	
	if (sparse_handle_far == NULL){//handles error		
		ERR_PRINTLN("sparse far service creation fail");
		return 0;
	}else{
		acc_service_sparse_get_metadata(sparse_handle_far, &sparse_metadata_far);
		if(sparse_data_malloc() == -1){
            DBG_PRINTLN("data buffer allocation failed");
            stopService();
            }
		
		Reg_store_sparse_metadata(sparse_metadata, &sparse_metadata_far);
        
		printf_sparse_metadata(sparse_metadata_far);
	}
	}
	return 1;
}


int8_t activateService(void){
    DBG_PRINTLN("activating service");
    uint32_t service_type = RegInt_getreg(0x02);
    int8_t success;
    if(service_type == 0x02){success = activateService_handle(envelope_handle);}
    else if(service_type == 0x04){success = activateService_handle(sparse_handle);}
    else {Reg_regor(0x06, 0x0040000); return 0;}
    
    if(success){
        Reg_regor(0x06, 0x00000002);
    }else{
        Reg_regor(0x06, 0x00100000);
    }
    return success;
}

int8_t activateService_handle(acc_service_handle_t handle){
    
	if (!acc_service_activate(handle))
	{
		ERR_PRINTLN("acc_service_activate() failed");
		acc_service_destroy(&handle);
		data_free();
		
		if(far_active){acc_service_destroy(&sparse_handle_far);}
		
		// acc_rss_deactivate();

		return 0;
	}else{
		DBG_PRINTLN("service handle activated");
        return 1;
	}
	
}

void stopService(void){
    DBG_PRINTLN("stopping service");
	//the first bit is set when a service is active
	// DBG_PRINTINT(RegInt_getreg(0x06));
    if(!(RegInt_getreg(0x06) && 0x00000001)){DBG_PRINTLN("no active service"); return;}
	
	uint32_t service_type = RegInt_getreg(0x02);
    
    acc_service_handle_t handle = NULL;
    if(service_type == 0x02){handle = envelope_handle;}
    else if(service_type == 0x04){handle = sparse_handle;}
    else{return;}
    
	if(acc_service_deactivate(handle)){
		acc_service_destroy(&handle);	
		data_free();//maybe a memory leak?
		DBG_PRINTLN("sparse service destroyed");
	}else{
		ERR_PRINTLN("sparse service deactivation fail");
	}
	
	if(far_active){
	if(acc_service_deactivate(sparse_handle_far)){
		acc_service_destroy(&sparse_handle_far);	
		data_free();
		far_active = 0;
		DBG_PRINTLN("far sparse service destroyed");
	}else{
		ERR_PRINTLN("far sparse service deactivation fail");
	}
	}
    
	//clear least two bits
    uint32_t setbits = RegInt_getreg(0x06);
    setbits &= 0xFFFFFFFC;
    RegInt_setregf(0x06, setbits, 1);
	// DBG_PRINTINT(RegInt_getreg(0x06));
	
}

void measure(void){
    uint32_t service_type = RegInt_getreg(0x02);
    if(service_type == 0x02){envelopeMeasure();}
    else if(service_type == 0x04){sparseMeasure();}
}

void sparseMeasure(void){
		//something about a periodic interrupt timer. 
	INF_PRINTLN("Start Sparse measurement");
	acc_service_sparse_get_next_by_reference(sparse_handle, &sparse_data, &sparse_result_info);
	//filling the data buffer for near data
	filldata_sparse(0);
	DBG_PRINTLN("Sparse measurement complete");
		
	if(far_active){
		if(!acc_service_deactivate(sparse_handle)){
			ERR_PRINTLN("acc_service_deactivate() for sparse failed");
		}
		
		if (!acc_service_activate(sparse_handle_far)){
			ERR_PRINTLN("acc_service_activate() for sparse far failed");
			//handle error
		}
			
		acc_service_sparse_get_next_by_reference(sparse_handle_far, &sparse_data_far, &sparse_result_info_far);
		//filling the data buffer for far data
		filldata_sparse(1);
		
		if(!acc_service_deactivate(sparse_handle_far)){
			ERR_PRINTLN("acc_service_deactivate() for sparse far failed");
		}
		if (!acc_service_activate(sparse_handle)){
			ERR_PRINTLN("acc_service_activate() for sparse failed");
			//handle error
		}
		
		DBG_PRINTLN("Sparse Far measurement end");
	}
}

void envelopeMeasure(void){
	
	INF_PRINTLN("Start Envelope measurement");
	uint32_t repeats = RegInt_getreg(0x41);
	DBG_PRINTINT(repeats);
	for(uint32_t i=0; i < repeats; i++){
		acc_service_envelope_get_next_by_reference(envelope_handle, &envelope_data, &envelope_result_info);
	}
	//filling the data buffer for near data
	filldata_envelope();
	DBG_PRINTLN("Envelope measurement complete");
	
}

void printf_sparse_metadata(acc_service_sparse_metadata_t metadata){
    INF_PRINTLN("Sparse* Serivce Metadata");
	INF_PRINTLN("Start: %ld mm", (int32_t)(metadata.start_m * 1000.0f));
    if(! far_active){
        INF_PRINTLN("Length: %lu mm", (uint32_t)(metadata.length_m * 1000.0f));
    }else{
       INF_PRINTLN("Length (far): %lu mm", (uint32_t)((metadata.length_m+ sparse_metadata_far.length_m) * 1000.0f)); 
    }
    INF_PRINTLN("Data length: %lu", (uint32_t)metadata.data_length);
    INF_PRINTLN("Sweep rate: %lu mHz", (uint32_t)(metadata.sweep_rate * 1000.0f));
    INF_PRINTLN("Step length: %lu um", (uint32_t)(metadata.step_length_m * 1.0e6f));
}

void printf_envelope_metadata(acc_service_envelope_metadata_t metadata){
    INF_PRINTLN("Envelope Serivce Metadata");
	INF_PRINTLN("Start: %ld mm", (int32_t)(metadata.start_m * 1000.0f));
    INF_PRINTLN("Length: %lu mm", (uint32_t)(metadata.length_m * 1000.0f));
    INF_PRINTLN("Data length: %lu", (uint32_t)metadata.data_length);
    INF_PRINTLN("Sweep rate: %lu mHz", (uint32_t)(metadata.stitch_count));
    INF_PRINTLN("Step length: %lu um", (uint32_t)(metadata.step_length_m * 1.0e6f));
}


void evalData(void){
	uint32_t service_type = RegInt_getreg(0x02);
    if(service_type == 0x02){evalEnvelopeData();}
    else if(service_type == 0x04){evalSparseData();}
}

//mode
/*
Bit.
1.	dc removal
2.	do fft
3.	do convolution
4.	get velocity
5.	null below threshold
6.	bandstop
7.  detrend
*/
void evalSparseData(void){
    // #warning implentation removed for debuging
    // return;
	uint16_t dist_res = (uint16_t)(sparse_metadata.step_length_m*1000.0f);
	uint16_t dist_start = (uint16_t)(sparse_metadata.start_m*1000.0f);
	float sweep_rate = sparse_metadata.sweep_rate;
	uint16v2_t data_size = {sweeps,bins};
	
	float min_scale = 1.0f;
	float thrstd = RegInt_getreg(0xD4)/1000.0f;
	float thrnull = RegInt_getreg(0xD8)/1000.0f;
	uint32_t mode = RegInt_getreg(0xD6);
	uint16_t roi_radius = RegInt_getreg(0xD7);
	uint32_t band_filt = RegInt_getreg(0xD9);
	
	float velocity;
	float distance;
	float amplitude;
	float meansqdist;

	DBG_PRINTINT(mode);

	//dc removal
	if(mode & 0x00000001){dcdatarm(data, data_size);}
	if(mode & 0x00000040){detrend(data, data_size);}
	
	//do fft on each row of data
	if(mode & 0x00000002){min_scale = dofft(data, data_size);}
	
	//bandstop filter
	if(mode & 0x00000020){dobandstop(data, data_size, band_filt);}
	
	//do convolution
	if(mode & 0x00000004){
		float st_dev = RegInt_getreg(0xD5)/1000.0f;
		doconv(data, data_size, st_dev);
	}
	//calulate velocity parameters
	if(mode & 0x00000008){
			//get maximum index and maximum
		uint16v2_t max_index = max2d(data, data_size);
		uint16_t apex = data[max_index.x1][max_index.x2];
		
		DBG_PRINTINT(apex);
		DBG_PRINTINT(max_index.x1);
		DBG_PRINTINT(max_index.x2);
		
		//get mean square distnace from maximum
		meansqdist = get_msd(data, data_size, max_index, thrstd);
				
		//null data less than threshold
		if(mode & 0x00000010){null_data(data,data_size,max_index, thrnull);}
		
		//the center of mass of the image need to be computed
		floatv2_t com = center_of_mass(max_index, roi_radius);
		
		distance = dist_res*com.x2 + dist_start;	//converts data to distance (mm)
		velocity = com.x1 * (float)(sweep_rate/sweeps) * 2.445f; //converts data to velocity (mm/s)
		
		if(min_scale != 0.0f){amplitude = apex/min_scale;}
		else{amplitude = 0;}
		
		//store results
		RegInt_setregf(0xD0,(uint32_t)velocity, 1);
		RegInt_setregf(0xD1,(uint32_t)distance, 1);
		RegInt_setregf(0xD2,(uint32_t)amplitude, 1);
		RegInt_setregf(0xD3,(uint32_t)meansqdist, 1);
			
		print_sparse_results();
	}
}


//this should return the four tallest peaks and their amplitudes in D0 - D3
//format 0xYYYYZZZZ, YYYY distance in mm, ZZZZ amplitude (little endian)
void evalEnvelopeData(void){
	const int n = 4;
	uint16_t dist_res = (uint16_t)(envelope_metadata.step_length_m*1.0e6f);
	uint16_t dist_start = (uint16_t)(envelope_metadata.start_m*1000.0f);
	
	uint16_t min_sep = (uint16_t)((1000*(uint32_t)RegInt_getreg(0xD4))/dist_res);
	DBG_PRINTINT(min_sep);
	uint16_t indexes[n];
	uint16_t amplitudes[n];

	getpeaks(data, bins, indexes, amplitudes, min_sep);
			
	uint16_t distances[n];
	for(uint8_t i = 0; i < n; i++){
		distances[i] = (uint16_t)(((uint32_t)indexes[i]*(uint32_t)dist_res)/1e3) + dist_start;
	}

	uint32_t distamp_pack[n];
	pack16to32array(distamp_pack, distances, amplitudes);
	
	//store results
	RegInt_setregf(0xD0,(uint32_t)distamp_pack[0], 1);
	RegInt_setregf(0xD1,(uint32_t)distamp_pack[1], 1);
	RegInt_setregf(0xD2,(uint32_t)distamp_pack[2], 1);
	RegInt_setregf(0xD3,(uint32_t)distamp_pack[3], 1);
	
	print_envelope_results();
}

void dsp_burst(void){
	uint32_t dsp_cmds = RegInt_getreg(0xDA);
	float	 dsp_val = RegInt_getreg(0xDB);
	
	//0x87 65 43 21 // cmd order
	for(uint8_t i = 0; i < 8; i++){
		uint8_t cmd = (dsp_cmds >> 4*i) & 0x0F;
		
		switch (cmd){
		case 0x00:
			break;
		case 0x01:
			set_accumulant(dsp_val);
			break;
		case 0x02:
			load_accumulant();
			break;
		case 0x03:
			add_accumulant(dsp_val);
			break;
		case 0x04:
			mult_accumulant(dsp_val);
			break;
		case 0x05:
			acc_accumulant();
			break;
		case 0x06:
			sq_acc_accumulant();
			break;
		case 0x07:
			sqrt_accumulant();
			break;
		case 0x08:
			add_data(dsp_val);
			break;
		default:
			break;
		}
		
	}
	
}

void print_sparse_results(void){
		INF_PRINTLN("RESULTS");
		INF_PRINTLN("Velocity: %ld mm\\s", RegInt_getreg(0xD0));
		INF_PRINTLN("Distance: %ld mm", RegInt_getreg(0xD1));
		INF_PRINTLN("Amplitude: %ld arb", RegInt_getreg(0xD2));
		INF_PRINTLN("Mean Square Distance: %ld arb", RegInt_getreg(0xD3));
}


void print_envelope_results(void){
	INF_PRINTLN("RESULTS");
	INF_PRINTLN("Peak 1: %d mm | %d arb.", get_short(RegInt_getreg(0xD0),2), get_short(RegInt_getreg(0xD0),0));
	INF_PRINTLN("Peak 2: %d mm | %d arb.", get_short(RegInt_getreg(0xD1),2), get_short(RegInt_getreg(0xD1),0));
	INF_PRINTLN("Peak 3: %d mm | %d arb.", get_short(RegInt_getreg(0xD2),2), get_short(RegInt_getreg(0xD2),0));
	INF_PRINTLN("Peak 4: %d mm | %d arb.", get_short(RegInt_getreg(0xD3),2), get_short(RegInt_getreg(0xD3),0));

}