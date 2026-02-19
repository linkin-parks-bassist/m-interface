#ifndef M_INT_HELPER_FUNCTIONS_H_
#define M_INT_HELPER_FUNCTIONS_H_

/* Helper functions! Yay! */

#define binary_max(x, y) ((x > y) ? x : y)
#define binary_min(x, y) ((x > y) ? y : x)

#define sqr(x) (x * x)

// For some reason, printf on esp32 does not support %b
char *binary_print_n (uint32_t x, int n);

char *binary_print_8 (uint8_t  x);
char *binary_print_16(uint16_t x);
char *binary_print_24(uint32_t x);
char *binary_print_32(uint32_t x);

// IBM = "initial bit mask"; 0b000..0111...1 with "x" 1s
#define IBM(x) ((1u << (x)) - 1)
// Extracts n bits, starting at x, and shifted to the start 
#define range_bits(x, n, start) (((x) >> (start)) & IBM(n))
// Gives a word with bits y to x being the initial bits of "val"
#define place_bits(x, y, val) ((IBM((x)-(y)+1) & ((uint32_t)val)) << y)

#endif
