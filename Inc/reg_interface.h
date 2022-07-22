#ifndef RegInterface_h
#define RegInterface_h

#include "acc_hal_definitions.h"
#include "acc_hal_integration.h"
#include "acc_rss.h"
#include "acc_service.h"
#include "acc_service_sparse.h"
#include "acc_service_envelope.h"

/*/////////changelog/////////////
Version 1.4.0
  -refactored code into files by function
  -added envelope service

///////////////////////////////*/


#define FIRMWARE_REVISION 0xBF010500 //0x BF MM II PP -> BoSL Firmware MM.II.PP
#define HARDWARE_REVISION 0xBD010100 //0x BD MM II PP -> BoSL Device MM.II.PP

#define UART_BUFF 64

uint32_t GENERAL_REGISTERS[0x13];
uint32_t OUTPUT_BUFFER_LENGTH;
uint32_t SERVICE_REGISTERS[0x23];
uint32_t META_REGISTERS[0x05];
uint32_t EVAL_REGISTERS[0x10];
uint32_t REGADRERR;

uint8_t uart_rx_buff[UART_BUFF];
uint8_t uart_tx_buff[UART_BUFF];
uint8_t uart_state;
uint8_t cmd_length;
uint8_t queue_cmd_end;
uint8_t far_active;
uint16_t** data;
float** accumulant;
float dsp_value_f;
uint16_t sweeps;
uint16_t bins;
uint32_t bufflen;
uint32_t bufflen_far;



acc_hal_t 							radar_hal;

acc_service_configuration_t 		sparse_config;
acc_service_configuration_t 		sparse_config_far;
acc_service_configuration_t 		envelope_config;

acc_service_handle_t 				sparse_handle; 
acc_service_handle_t 				sparse_handle_far; 
acc_service_handle_t 				envelope_handle; 

uint16_t*                         	sparse_data;
uint16_t*                         	sparse_data_far;
uint16_t*                         	envelope_data;

acc_service_sparse_result_info_t 	sparse_result_info;
acc_service_sparse_result_info_t 	sparse_result_info_far;
acc_service_envelope_result_info_t 	envelope_result_info;

acc_service_sparse_metadata_t 		sparse_metadata;
acc_service_sparse_metadata_t 		sparse_metadata_far;
acc_service_envelope_metadata_t     envelope_metadata;

uint32_t* RegInt_regmap(uint8_t);
uint32_t RegInt_getreg(uint8_t);
int8_t RegInt_setreg(uint8_t, uint32_t);
int8_t RegInt_setregf(uint8_t, uint32_t, uint8_t);
int8_t RegInt_writeable(uint8_t);
void Reg_regand(uint8_t, uint32_t);
void Reg_regor(uint8_t, uint32_t);

void RegInt_Init(void);
void RegInt_parsecmd(void);
void RegInt_regaction(uint8_t, uint32_t);

void Reg_store_sparse_metadata(acc_service_sparse_metadata_t, acc_service_sparse_metadata_t*);
void Reg_store_envelope_metadata(acc_service_envelope_metadata_t);


void changeUART1baud(uint32_t);
void sleepMCU(uint32_t);

// void send_byte_ln(uint8_t);


void rss_control(uint32_t);
void initRSS(void);

int8_t envelope_data_malloc(void);
int8_t sparse_data_malloc(void);
int8_t accumulant_malloc(uint16_t sweeps, uint16_t bins);
int8_t data_malloc(uint16_t, uint16_t);
void data_free(void);
void filldata_sparse(uint8_t);
void filldata_envelope(void);

void updateSparseConfig(acc_service_configuration_t, uint16_t, uint16_t);
void updateEnvelopeConfig(acc_service_configuration_t);

int8_t createService(void);
int8_t createEnvelopeService(void);
int8_t createSparseService(void);

int8_t activateService(void);
int8_t activateService_handle(acc_service_handle_t);

void stopService(void);

void measure(void);
void sparseMeasure(void);
void envelopeMeasure(void);

void printf_sparse_metadata(acc_service_sparse_metadata_t);
void printf_envelope_metadata(acc_service_envelope_metadata_t);

void evalData(void);
void evalSparseData(void);
void evalEnvelopeData(void);

void dsp_burst(void);

void print_sparse_results(void);
void print_envelope_results(void);


#endif
