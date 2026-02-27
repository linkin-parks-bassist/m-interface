#include <stdint.h>
#include <math.h>

#include "m_hfunc.h"

char binary_print_buffer[35];

char *binary_print_n(uint32_t x, int n)
{
	binary_print_buffer[0] = '0';
	binary_print_buffer[1] = 'b';
	
	int i = 0;
	while (i < n && i < 32)
	{
		binary_print_buffer[i+2] = '0' + ((x >> (n - i - 1)) & 1);
		i++;
	}
	
	binary_print_buffer[i+2] = 0;
	
	return binary_print_buffer;
}

char *binary_print_8 (uint8_t x)
{
	return binary_print_n(x, 8);
}

char *binary_print_16(uint16_t x)
{
	
	return binary_print_n(x, 16);
}


char *binary_print_24(uint32_t x)
{
	return binary_print_n(x, 24);
}

char *binary_print_32(uint32_t x)
{
	return binary_print_n(x, 32);
}
