#include "util.h"

uint8_t get_byte(uint32_t val, uint8_t byte){
	return (val & (0xFFL << (8*byte))) >> (8*byte);
}

uint32_t roundUp(uint32_t numToRound, uint32_t multiple)
{
    if (multiple == 0)
        return numToRound;

    uint32_t remainder = numToRound % multiple;
    if (remainder == 0)
        return numToRound;

    return numToRound + multiple - remainder;
}


uint32_t roundDown(uint32_t numToRound, uint32_t multiple)
{
    if (multiple == 0)
        return numToRound;

    uint32_t remainder = numToRound % multiple;
    if (remainder == 0)
        return numToRound;

    return numToRound - remainder;
}