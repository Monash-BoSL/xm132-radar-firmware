#ifndef RegInterface_h
#define RegInterface_h

#include "acc_hal_definitions.h"
#include "acc_hal_integration.h"
#include "acc_rss.h"
#include "acc_service.h"
#include "acc_service_sparse.h"



#define UART_BUFF 32

uint32_t GENERAL_REGISTERS[0x13];
uint32_t OUTPUT_BUFFER_LENGTH;
uint32_t SERVICE_REGISTERS[0x23];
uint32_t META_REGISTERS[0x05];
uint32_t REGADRERR;


uint8_t uart_rx_buff[UART_BUFF];
uint8_t uart_tx_buff[UART_BUFF];
uint8_t uart_state;
uint8_t cmd_length;
uint8_t queue_cmd_end;
uint8_t far_active;
uint16_t **data;
uint16_t sweeps;
uint16_t bins;
uint32_t bufflen;
uint32_t bufflen_far;

acc_hal_t 							radar_hal;
acc_service_configuration_t 		sparse_config;
acc_service_configuration_t 		sparse_config_far;
acc_service_handle_t 				sparse_handle; 
acc_service_handle_t 				sparse_handle_far; 
uint16_t                         	*sparse_data;
uint16_t                         	*sparse_data_far;
acc_service_sparse_result_info_t 	sparse_result_info;
acc_service_sparse_result_info_t 	sparse_result_info_far;
acc_service_sparse_metadata_t 		sparse_metadata;
acc_service_sparse_metadata_t 		sparse_metadata_far;

void RegInt_Init(void);
uint32_t RegInt_getreg(uint8_t);
uint32_t* RegInt_regmap(uint8_t);
void RegInt_setreg(uint8_t, uint32_t);
void RegInt_setregf(uint8_t, uint32_t, uint8_t);
void Reg_regand(uint8_t, uint32_t);
void Reg_regor(uint8_t, uint32_t);
void Reg_store_metadata(acc_service_sparse_metadata_t);
void RegInt_parsecmd(void);

void send_byte_ln(uint8_t);
uint8_t get_byte(uint32_t, uint8_t );
uint32_t roundUp(uint32_t, uint32_t );

void rss_control(uint32_t);
void initRSS(void);

void updateConfig(acc_service_configuration_t, uint16_t, uint16_t);
void stopService(void);
int8_t createService(void);
void printf_metadata(acc_service_sparse_metadata_t);
void activateService(void);
void stopService(void);
void sparseMeasure(void);
void evalData(void);

int8_t data_malloc(void);
void data_free(void);
void filldata(uint8_t);




#endif
