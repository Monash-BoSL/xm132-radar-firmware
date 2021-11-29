#ifndef Util_h
#define Util_h

#include "acc_hal_integration.h"

#define cbi(sfr, bit)   (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit)   (_SFR_BYTE(sfr) |= _BV(bit))


#define DBG_PRINTINT(var)	printf("%s: %ld at %s:%d\n", #var, (int32_t)var, __FILE__,__LINE__)
#define DBG_PRINTFLT(var)	printf("%s: %ld at %s:%d\n", #var, (int32_t)(var*1000), __FILE__,__LINE__)

#define INF_PRINTLN(...) printf("INF: ");printf(__VA_ARGS__);printf(" at %s:%d\n", __FILE__,__LINE__)
#define DBG_PRINTLN(...) printf("DBG: ");printf(__VA_ARGS__);printf(" at %s:%d\n", __FILE__,__LINE__)
#define WRN_PRINTLN(...) printf("WRN: ");printf(__VA_ARGS__);printf(" at %s:%d\n", __FILE__,__LINE__)
#define ERR_PRINTLN(...) printf("ERR: ");printf(__VA_ARGS__);printf(" at %s:%d\n", __FILE__,__LINE__)


uint8_t get_byte(uint32_t, uint8_t );
uint32_t roundUp(uint32_t, uint32_t );
uint32_t roundDown(uint32_t, uint32_t );


#endif