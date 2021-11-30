#ifndef Util_h
#define Util_h

#include "acc_hal_integration.h"

#define cbi(sfr, bit)   (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit)   (_SFR_BYTE(sfr) |= _BV(bit))

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

//define various debug statements 
#define _DBG_PRINTINT(var)	printf("DBG: ");printf("%s: %ld", #var, (int32_t)var)
#define _DBG_PRINTFLT(var)	printf("DBG: ");printf("%s: %ld", #var, (int32_t)(var*1000))

#define _INF_PRINTLN(...) printf("INF: ");printf(__VA_ARGS__)
#define _DBG_PRINTLN(...) printf("DBG: ");printf(__VA_ARGS__)
#define _WRN_PRINTLN(...) printf("WRN: ");printf(__VA_ARGS__)
#define _ERR_PRINTLN(...) printf("ERR: ");printf(__VA_ARGS__)

//add line location printing
#ifdef DBG_LOGGING
	#define PRINT_AT printf(" at %s:%d\n", __FILE__,__LINE__)
#else
	#define PRINT_AT printf("\n")
#endif

#define DBG_PRINTINT(var)	_DBG_PRINTINT(var);PRINT_AT
#define DBG_PRINTFLT(var)	_DBG_PRINTFLT(var);PRINT_AT

#define INF_PRINTLN(...) _INF_PRINTLN(__VA_ARGS__);PRINT_AT
#define DBG_PRINTLN(...) _DBG_PRINTLN(__VA_ARGS__);PRINT_AT
#define WRN_PRINTLN(...) _WRN_PRINTLN(__VA_ARGS__);PRINT_AT
#define ERR_PRINTLN(...) _ERR_PRINTLN(__VA_ARGS__);PRINT_AT


//disable debug statements
#ifndef DBG_LOGGING
	#undef	DBG_PRINTINT 
	#define DBG_PRINTINT(var) 
	#undef	DBG_PRINTFLT
	#define DBG_PRINTFLT(var) 
	#undef	DBG_PRINTLN
	#define DBG_PRINTLN(...) 
#endif




uint8_t get_byte(uint32_t, uint8_t );
uint32_t roundUp(uint32_t, uint32_t );
uint32_t roundDown(uint32_t, uint32_t );


#endif