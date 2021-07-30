#include "reg_interface.h"

#include "stm32g0xx_it.h"
#include "stm32g0xx_hal.h"

#include <stdio.h>

#define cbi(sfr, bit)   (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit)   (_SFR_BYTE(sfr) |= _BV(bit))


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
	uart_state = 0;
	printf("reg_int rec\n");
	HAL_UART_Receive_IT(&huart1, uart_rx_buff, 1);
	printf("reg_int success\n");
}

uint32_t RegInt_getreg(uint8_t reg){
	uint32_t* regptr = RegInt_regmap(reg);
	return *regptr;
}

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
	return ptr;
}

void RegInt_setreg(uint8_t reg, uint32_t val){
	RegInt_setregf(reg, val, 0);
}

void RegInt_setregf(uint8_t reg, uint32_t val, uint8_t force){
	if(!force){
	uint8_t read_only_addr[10] = {0x06, 0x10, 0x11, 0x12, 0x81, 0x82,0x83,0x84,0x85, 0xE9}; //please have this list be ordered.
	for(uint8_t i = 0; i < 10; i++){
		if (reg == read_only_addr[i]){return;}
		if (reg < read_only_addr[i]){break;}
	}
	}
	
	uint32_t* regptr = RegInt_regmap(reg);
	if (!(*regptr == -1)){
		*regptr = val;
	}
	//main control
	if(reg == 0x03){
		rss_control(val);
	}
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
	}else if (uart_rx_buff[0] == 0xFA && uart_rx_buff[1] == 0xE8 && cmd_length == 3){
		uint8_t offst_h = 0;
		uint8_t offst_l = 0;
		offst_l = uart_rx_buff[2];
		offst_h = uart_rx_buff[3];
		uint16_t offst = (offst_h << 8) | offst_l;
		uint32_t datalen = (sparse_metadata.data_length-offst)*sizeof(uint16_t);
		uart_tx_buff[0] = 0xCC;
		uart_tx_buff[1] = get_byte(datalen+1,0);
		uart_tx_buff[2] = get_byte(datalen+1,1);
		uart_tx_buff[3] = 0xF7;
		uart_tx_buff[4] = 0xE8;
		HAL_UART_Transmit(&huart1, uart_tx_buff, 5, 10);
		queue_cmd_end = 1;
		HAL_UART_Transmit_IT(&huart1, sparse_data+offst, datalen);
	}
	uart_state = 0;
	HAL_UART_Receive_IT(&huart1, uart_rx_buff, 1);

}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if (uart_state == 0){
		if (uart_rx_buff[0] == 0xCC){
			uart_state = 1;
			cmd_length = 0;
			HAL_UART_Receive_IT(&huart1, uart_rx_buff, 2);
		}else{
			HAL_UART_Receive_IT(&huart1, uart_rx_buff, 1);
		}
	}else if (uart_state == 1){
		cmd_length = (uart_rx_buff[0]) | (uart_rx_buff[1] << 8);
		uart_state = 3;
		if (cmd_length +2 > UART_BUFF){
			uart_state = 0;
			HAL_UART_Receive_IT(&huart1, uart_rx_buff, 1);
		}
		HAL_UART_Receive_IT(&huart1, uart_rx_buff, 2 + cmd_length);
	}else if (uart_state == 3){
		uart_state = 4;
		//HAL_UART_Receive_IT(&huart1, uart_rx_buff, 1);
	}

}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
	if (queue_cmd_end == 1){
		uint8_t end = 0xCD;
		queue_cmd_end = 0;
		HAL_UART_Transmit_IT(&huart1, &end, 1);
	}
}

void send_byte_ln(uint8_t byte){
	uint8_t nl = '\n';
	HAL_UART_Transmit(&huart1, &byte, 1, 1000);	
	HAL_UART_Transmit(&huart1, &nl, 1, 1000);	
}

uint8_t get_byte(uint32_t val, uint8_t byte){
	return (val & (0xFFL << (8*byte))) >> (8*byte);
}

void rss_control(uint32_t val){
	if (val == 0x00){stopService();}
	if (val == 0x01){createService();}
	if (val == 0x02){activateService();}
	if (val == 0x03){createService(); activateService();}
	if (val == 0x04){//clear error bits
		uint32_t flags = RegInt_getreg(0x06); 
		flags &= 0x000000FF;
		RegInt_setregf(0x06, flags, 1);	
	}
	if (val == 0x05){sparseMeasure();}
}


void initRSS(void){
	radar_hal = *acc_hal_integration_get_implementation();
	
	if (!acc_rss_activate(&radar_hal))
	{
		/* handle error */
		printf("rssfail @%d\n", __LINE__);
	}
	
	sparse_config = acc_service_sparse_configuration_create();
 
	if (sparse_config == NULL)
	{
		/* Handle error */
		printf("config @%d\n", __LINE__);
	}

}

void updateConfig(void){
	
	acc_service_profile_set(sparse_config, RegInt_getreg(0x28));
	
	uint32_t rep_mode = RegInt_getreg(0x22); 
	if(rep_mode == 0x01){
		acc_service_repetition_mode_streaming_set(sparse_config, ((float)RegInt_getreg(0x23))/1000.0f);
	}else if (rep_mode == 0x02){
		acc_service_repetition_mode_on_demand_set(sparse_config);
	}else{
		/* error handle */
	}
	
	acc_service_sparse_downsampling_factor_set(sparse_config, RegInt_getreg(0x29));
	
	acc_service_hw_accelerated_average_samples_set(sparse_config, RegInt_getreg(0x30));
	
	acc_service_power_save_mode_set(sparse_config, RegInt_getreg(0x25));
	
	acc_service_asynchronous_measurement_set(sparse_config,RegInt_getreg(0x33));
	
	acc_service_requested_start_set (sparse_config, (float)RegInt_getreg(0x20)/1000.0f);
	
	acc_service_requested_length_set (sparse_config, (float)RegInt_getreg(0x21)/1000.0f);
	
	acc_service_receiver_gain_set (sparse_config, (float)RegInt_getreg(0x24)/1000.0f);
	
	acc_service_hw_accelerated_average_samples_set (sparse_config, RegInt_getreg(0x30));
	
	acc_service_sparse_configuration_sweeps_per_frame_set (sparse_config, RegInt_getreg(0x40));
	
	acc_service_sparse_configuration_sweep_rate_set (sparse_config, (float)RegInt_getreg(0x41)/1000.0f);
	
	acc_service_sparse_sampling_mode_set (sparse_config, RegInt_getreg(0x42));
	
	acc_service_sparse_downsampling_factor_set (sparse_config, RegInt_getreg(0x29));
	
}

void createService(void){
	updateConfig();
	
	sparse_handle = acc_service_create(sparse_config);
	
	if (sparse_handle == NULL){//handles error
		uint32_t flags = RegInt_getreg(0x06); 
		flags |= 0x0008000;
		RegInt_setregf(0x06, flags, 1); 
		printf("creation failed\n");
	}else{
		sparse_metadata.start_m = 0;
		sparse_metadata.length_m = 0;
		sparse_metadata.data_length = 0;
		sparse_metadata.sweep_rate = 0;
		sparse_metadata.step_length_m = 0;
		
		acc_service_sparse_get_metadata(sparse_handle, &sparse_metadata);
		RegInt_setregf(0x81, (uint32_t)(sparse_metadata.start_m * 1000.0f),1);
		RegInt_setregf(0x82, (uint32_t)(sparse_metadata.length_m * 1000.0f),1);
		RegInt_setregf(0x83, (uint32_t)(sparse_metadata.data_length),1);
		RegInt_setregf(0x84, (uint32_t)(sparse_metadata.sweep_rate * 1000.0f),1);
		RegInt_setregf(0x85, (uint32_t)(sparse_metadata.step_length_m * 1000.0f),1);
		
		uint32_t flags = RegInt_getreg(0x06); 
		flags |= 0x0000001;
		RegInt_setregf(0x06, flags, 1);


		printf("Start: %ld mm\n", (int32_t)(sparse_metadata.start_m * 1000.0f));
		printf("Length: %lu mm\n", (uint32_t)(sparse_metadata.length_m * 1000.0f));
		printf("Data length: %lu\n", (uint32_t)sparse_metadata.data_length);
		printf("Sweep rate: %lu Hz\n", (uint32_t)(sparse_metadata.sweep_rate * 1000.0f));
		printf("Step length: %lu mm\n", (uint32_t)(sparse_metadata.step_length_m * 1000.0f));
	}
}

void activateService(void){
	if (!acc_service_activate(sparse_handle))
	{
		printf("acc_service_activate() failed\n");
		acc_service_destroy(&sparse_handle);
		acc_rss_deactivate();
		
		uint32_t flags = RegInt_getreg(0x06); 
		flags |= 0x00100000;
		RegInt_setregf(0x06, flags, 1); 
	}else{
		uint32_t flags = RegInt_getreg(0x06); 
		flags |= 0x00000002;
		RegInt_setregf(0x06, flags, 1); 
		printf("activato\n");
	}
	
	
	
}

void stopService(void){
	printf("sparce_handle: %ld\n", sparse_handle);
	printf("sparce_handle_val: %ld\n", &sparse_handle);
	if(acc_service_deactivate(sparse_handle)){
		acc_service_destroy(&sparse_handle);	
		printf("service destroyed @%d\n", __LINE__);
	}else{
		printf("deactivation fail @%d\n", __LINE__);
	}
	
}

void sparseMeasure(void){
		//something about a periodic interrupt timer. 
	printf("Start measurement\n");
	acc_service_sparse_get_next_by_reference(sparse_handle, &sparse_data, &sparse_result_info);
	printf("Done measurement\n");
}


