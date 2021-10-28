#ifndef RegInterface_h
#define RegInterface_h

#include "acc_hal_definitions.h"
#include "acc_hal_integration.h"
#include "acc_rss.h"
#include "acc_service.h"
#include "acc_service_sparse.h"

#define HARDWARE_REVISION 0xBF010000 //0x BF MM II PP -> BoSL Firmware MM.II.PP
#define FIRMWARE_REVISION 0xBD010201 //0x BD MM II PP -> BoSL Device MM.II.PP



#define UART_BUFF 32

#define CONVKER 9//must be odd


#if CONVKER % 2 != 1
	#error CONVKER must be odd
#endif

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
uint16_t **data;
uint16_t sweeps;
uint16_t bins;
uint32_t bufflen;
uint32_t bufflen_far;

float stdev_gss;
float kernel[CONVKER];
float convstack[CONVKER];


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
void Reg_store_metadata(acc_service_sparse_metadata_t, acc_service_sparse_metadata_t*);
void RegInt_parsecmd(void);

void changeUART1baud(uint32_t);

void send_byte_ln(uint8_t);
uint8_t get_byte(uint32_t, uint8_t );
uint32_t roundUp(uint32_t, uint32_t );
uint32_t roundDown(uint32_t, uint32_t );

void rss_control(uint32_t);
void initRSS(void);

void updateConfig(acc_service_configuration_t, uint16_t, uint16_t);
void stopService(void);
int8_t createService(void);
void printf_metadata(acc_service_sparse_metadata_t);
void activateService(void);
void stopService(void);
void sparseMeasure(void);


void makekernel(void);
void convolve1d(uint16_t, uint8_t);
uint16_t getdata(int16_t, int16_t);
void setdata(int16_t, int16_t, uint16_t);
void stackSet(void);
float stackPush(float);

void dcdatarm(void);
float dofft(void);
void doconv(void);

void evalData(void);

int8_t data_malloc(void);
void data_free(void);
void filldata(uint8_t);




#endif
