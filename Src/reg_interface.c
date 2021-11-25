#include "reg_interface.h"

#include "arduinoFFTfix.h"
#include "stm32g0xx_it.h"
#include "stm32g0xx_hal.h"

#include "main.h"
#include "util.h"

#include <stdio.h>
#include <string.h>
#include <math.h>



extern UART_HandleTypeDef huart1;

//okay so i think i want to set up multiple servies to range stich.
//we probably need 2 or 3 ranges. I guess we will see.


//begin listning on UART
void RegInt_Init(void){
	queue_cmd_end = 0;
	for(uint8_t i = 0; i < 0xFF; i++){
		if(i == 3){continue;}//writing to this reg controlls the RSS.
		RegInt_setreg(i, 0);
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

uint32_t RegInt_getreg(uint8_t reg){
	uint32_t* regptr = RegInt_regmap(reg);
	return *regptr;
}

//registers as defined in xm132 module software
//for custom registers
//0xD0 :: Velocity (mm/s)
//0xD1 :: Distance (mm)
//0xD2 :: Amplitude (arb)
//0xD3 :: Mean Square Distance (arb)
//0xD4 :: Mean Square Distance threshold (x1000)
//0xD5 :: Gaussian Kernal StDev (x1000)
//0xD6 :: Data Eval Mode
//0xD7 :: Focus weight Radius
//0xD8 :: Data zeroing threshold
//0xD9 :: Bandstop velocity filter
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

void RegInt_setreg(uint8_t reg, uint32_t val){
	RegInt_setregf(reg, val, 0);
}

int8_t RegInt_writeable(uint8_t reg){
    //if service is activeated lock mode
    if(reg == 0x02){
        if(RegInt_getreg(0x06) || 0x00000001){
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

void RegInt_setregf(uint8_t reg, uint32_t val, uint8_t force){
	if(!force){
        if (!RegInt_writeable(reg)){return;}
	}
    
	uint32_t* regptr = RegInt_regmap(reg);
	if (!(*regptr == (uint32_t)-1)){
		*regptr = val;
	}
    
    RegInt_regaction(reg, val);
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

void Reg_store_envelope_metadata(acc_service_envelope_metadata_t metadata){
    
	RegInt_setregf(0x81, (uint32_t)(metadata.start_m * 1000.0f),1);
	RegInt_setregf(0x82, (uint32_t)(metadata.length_m * 1000.0f),1);
	RegInt_setregf(0x83, (uint32_t)metadata.data_length ,1);
	RegInt_setregf(0x84, (uint32_t)metadata.stitch_count,1);
	RegInt_setregf(0x85, (uint32_t)(metadata.step_length_m * 1000.0f),1);
   
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
	RegInt_setregf(0x85, (uint32_t)(metadata.step_length_m * 1000.0f),1);
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
		RegInt_setreg(reg, val);
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

void changeUART1baud(uint32_t baudrate){
	HAL_UART_DeInit(&huart1);
	MX_USART1_UART_Init(baudrate);
	HAL_UART_Receive_IT(&huart1, uart_rx_buff, 1);
}



void rss_control(uint32_t val){
	if (val == 0x00){stopService();}
	if (val == 0x01){createService();}
	if (val == 0x02){activateService();}
	if (val == 0x03){
		if(createService()){activateService();}
	}
	if (val == 0x04){Reg_regand(0x06,0x000000FF);}//clear error bits
	if (val == 0x05){sparseMeasure();}
	if (val == 0x06){evalData();}
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
		/* Handle error */
		ERR_PRINTLN("sparse config creation fail");
	}
	
	sparse_config_far = acc_service_sparse_configuration_create();
 
	if (sparse_config_far == NULL)
	{
		/* Handle error */
		ERR_PRINTLN("far sparse config creation fail");
	}
    
    envelope_config = acc_service_envelope_configuration_create();
 
	if (envelope_config == NULL)
	{
		/* Handle error */
		printf("envelope config creation fail");
	}

}

void updateSparseConfig(acc_service_configuration_t config, uint16_t sweep_start, uint16_t sweep_length){
	
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

void updateEnvelopeConfig(acc_service_configuration_t config){
    
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
    
   	envelope_handle = acc_service_create(envelope_config);	
	
	if (envelope_handle == NULL){//handles error
		ERR_PRINTLN("envelope service creation failed");
		return 0;
	}else{
		acc_service_envelope_get_metadata(envelope_handle, &envelope_metadata);
		
        if(envelope_data_malloc() == -1){stopService();}

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
			if(sparse_data_malloc() == -1){stopService();}
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
		if(sparse_data_malloc() == -1){stopService();}
		
		Reg_store_sparse_metadata(sparse_metadata, &sparse_metadata_far);
        
		printf_sparse_metadata(sparse_metadata_far);
	}
	}
	return 1;
}

int8_t envelope_data_malloc(void){
    bins = envelope_metadata.data_length;
    return data_malloc(1,bins);
}

int8_t sparse_data_malloc(void){
	sweeps = acc_service_sparse_configuration_sweeps_per_frame_get (sparse_config);
	
	bins = sparse_metadata.data_length/sweeps;
	if(far_active){
		bins += sparse_metadata_far.data_length/sweeps;
	}
	return data_malloc(sweeps,bins);
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
	free(data);
	data = NULL;
	sweeps = 0;
	bins = 0;
}

void printf_sparse_metadata(acc_service_sparse_metadata_t metadata){
    printf("Sparse* Serivce Metadata\n");
	printf("Start: %ld mm\n", (int32_t)(metadata.start_m * 1000.0f));
    if(! far_active){
        printf("Length: %lu mm\n", (uint32_t)(metadata.length_m * 1000.0f));
    }else{
       printf("Length (far): %lu mm\n", (uint32_t)((metadata.length_m+ sparse_metadata_far.length_m) * 1000.0f)); 
    }
    printf("Data length: %lu\n", (uint32_t)metadata.data_length);
    printf("Sweep rate: %lu mHz\n", (uint32_t)(metadata.sweep_rate * 1000.0f));
    printf("Step length: %lu mm\n", (uint32_t)(metadata.step_length_m * 1000.0f));
}

void printf_envelope_metadata(acc_service_envelope_metadata_t metadata){
    printf("Envelope Serivce Metadata\n");
	printf("Start: %ld mm\n", (int32_t)(metadata.start_m * 1000.0f));
    printf("Length: %lu mm\n", (uint32_t)(metadata.length_m * 1000.0f));
    printf("Data length: %lu\n", (uint32_t)metadata.data_length);
    printf("Sweep rate: %lu mHz\n", (uint32_t)(metadata.stitch_count));
    printf("Step length: %lu mm\n", (uint32_t)(metadata.step_length_m * 1000.0f));
}

int8_t activateService(void){
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
		DBG_PRINTLN("service handle activated\n");
        return 1;
	}
	
}

void stopService(void){
    uint32_t service_type = RegInt_getreg(0x02);
    acc_service_handle_t handle;
    if(service_type == 0x02){handle = envelope_handle;}
    else if(service_type == 0x04){handle = sparse_handle;}
    
	if(acc_service_deactivate(handle)){
		acc_service_destroy(&handle);	
		data_free();
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
	
}


void sparseMeasure(void){
		//something about a periodic interrupt timer. 
	INF_PRINTLN("Start Sparse measurement");
	acc_service_sparse_get_next_by_reference(sparse_handle, &sparse_data, &sparse_result_info);
	//filling the data buffer for near data
	filldata(0);
	INF_PRINTLN("Sparse measurement complete");
		
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
		filldata(1);
		
		if(!acc_service_deactivate(sparse_handle_far)){
			ERR_PRINTLN("acc_service_deactivate() for sparse far failed");
		}
		if (!acc_service_activate(sparse_handle)){
			ERR_PRINTLN("acc_service_activate() for sparse failed");
			//handle error
		}
		
		INF_PRINTLN("Sparse Far measurement complete\n");
	}
}

void filldata(uint8_t far){
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

void makekernel(void){
	float norm = 0;
	for(uint16_t i = 0; i < CONVKER; i++){
		float t = (i - (CONVKER-1)/2)/(stdev_gss);
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
		if(indx >= bins){
			ERR_PRINTLN("bin count too great to convolve");
			return 0;
		}
		for(int i = 0; i< sweeps/2 + CONVKER-2; i++){
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
		if(indx >= sweeps){
			ERR_PRINTLN("sweep count too great to convolve");
			return 0;
		}
		for(int i = 0; i< bins + CONVKER-2; i++){
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
		(sweep >= 0) && (sweep < sweeps) &&
		(bin >= 0) && (bin < bins)
	){
		return data[sweep][bin];
	}else{
		return 0;
	}
}

void setdata(int16_t sweep, int16_t bin, uint16_t val){
	if(
		(sweep >= 0) && (sweep < sweeps) &&
		(bin >= 0) && (bin < bins)
	){
		data[sweep][bin] = val;
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


void dcdatarm(void){
	//remove dc component
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
float dofft(void){
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
		fftWindowing(real, sweeps, FFT_FORWARD);
		fftCompute(real, imag, sweeps, FFT_FORWARD);
		fftComplexToMagnitude(real, imag, sweeps);
		
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

void doconv(void){
	stdev_gss = RegInt_getreg(0xD5)/1000.0f;

	//gaussian filter, seperably calulated
	makekernel();
	
	for(uint16_t i = 0; i < bins; i++){
		convolve1d(i,0);
	}
	for(uint16_t j = 0; j < sweeps/2; j++){
		convolve1d(j,1);
	}
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
*/
void evalData(void){
	uint16_t dist_res = (uint16_t)(sparse_metadata.step_length_m*1000.0f);
	uint16_t dist_start = (uint16_t)(sparse_metadata.start_m*1000.0f);
	float sweep_rate = sparse_metadata.sweep_rate;
	
	float min_scale = 1;
	float thrstd = RegInt_getreg(0xD4)/1000.0f;
	float thrnull = RegInt_getreg(0xD8)/1000.0f;
	uint32_t mode = RegInt_getreg(0xD6);
	uint32_t band_filt = RegInt_getreg(0xD9);
	
	float velocity;
	float distance;
	float amplitude;
	float meansqdist;

	DBG_PRINTINT(mode);

	if(mode & 0x00000001){
	//dc removal
	dcdatarm();
	}
	
	
	if(mode & 0x00000002){
	//do fft on each row of data
	min_scale = dofft();
	}
	
	
	//bandstop filter
	if(mode & 0x00000020){	
		for(uint16_t j = 0; j<sweeps/2; j++){
		if(band_filt & (1<<j)){
			for(uint16_t i = 0; i<bins; i++){
					data[j][i] = 0;
			}
		}
		}	
	}
	
	
	if(mode & 0x00000004){
	//do convolution
	doconv();
	}
	
	if(mode & 0x00000008){
	//now we find the maximum value in the data
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
	
	DBG_PRINTINT(apex);
	DBG_PRINTINT(mbin);
	DBG_PRINTINT(msweep);
	
	meansqdist = 0.0f;
	float mass = 0.0f;

	uint16_t halfpex = (uint16_t)(apex * thrstd);
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
	
	if(mode & 0x00000010){
		uint16_t halfpex = apex*thrnull;
		for(uint16_t i = 0; i<bins; i++){
		for(uint16_t j = 0; j<sweeps/2; j++){
				if(data[j][i] > halfpex){
					data[j][i] = 0;
				}
		}
		}
	}
	
	//the center of mass of the image need to be computed
	mass = 0.0f;
	float weight_d = 0.0f; 
	float weight_s = 0.0f;
	uint8_t r = RegInt_getreg(0xD7);
	for(int16_t i = mbin-r; i<=mbin+r; i++){
	for(int16_t j = msweep-r; j<=msweep+r; j++){
			mass += getdata(j,i);
			weight_d += (float)getdata(j,i)*(float)i;
			weight_s += (float)getdata(j,i)*(float)j;
	}
	}
	if(mass != 0.0f){	
		weight_d /= mass;
		weight_s /= mass;
	}
	distance = dist_res*weight_d + dist_start;	//converts data to distance (mm)
	velocity = weight_s * (float)(sweep_rate/sweeps) * 2.445f; //converts data to velocity (mm/s)
	
	if(min_scale != 0.0f){
	amplitude = apex/min_scale;
	}else{
	amplitude = 0;
	}
	
	RegInt_setregf(0xD0,(uint32_t)velocity, 1);
	RegInt_setregf(0xD1,(uint32_t)distance, 1);
	RegInt_setregf(0xD2,(uint32_t)amplitude, 1);
	RegInt_setregf(0xD3,(uint32_t)meansqdist, 1);
	
	
	INF_PRINTLN("RESULTS");
	INF_PRINTLN("Velocity: %ld mm\\s", (uint32_t)velocity);
	INF_PRINTLN("Distance: %ld mm", (uint32_t)distance);
	INF_PRINTLN("Amplitude: %ld arb", (uint32_t)amplitude);
	INF_PRINTLN("Mean Square Distance: %ld arb", (uint32_t)meansqdist);
	}

	return;
}
