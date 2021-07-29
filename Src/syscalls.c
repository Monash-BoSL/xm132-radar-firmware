// Copyright (c) Acconeer AB, 2019-2020
// All rights reserved
// This file is subject to the terms and conditions defined in the file
// 'LICENSES/license_acconeer.txt', (BSD 3-Clause License) which is part
// of this source code package.

#include <stdint.h>

#include "main.h"

extern UART_HandleTypeDef DEBUG_UART_HANDLE;

int _write(int file, char *ptr, int len);

int _write(int file, char *ptr, int len)
{
	(void)file;
	HAL_UART_Transmit(&DEBUG_UART_HANDLE, (uint8_t*)ptr, len, 0xFFFF);
	return len;
}
