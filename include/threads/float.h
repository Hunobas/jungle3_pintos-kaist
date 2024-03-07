#include <debug.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define f 1 << 14

/* 
 * x and y are fixed-point numbers, n is an integer
 * fixed-point numbers are in signed p.q format where p + q = 31, and f is 1 << q
*/

const int32_t convert_ntox (const int n);
const int convert_xton (const int32_t x);

const int32_t add_xandn (const int32_t x, const int n);
const int32_t add_xandy (const int32_t x, const int32_t y);

const int32_t sub_nfromx (const int32_t x, const int n);
const int32_t sub_yfromx (const int32_t x, const int32_t y);

const int32_t mult_xbyn (const int32_t x, const int n);
const int32_t mult_xbyy (const int32_t x, const int32_t y);

const int32_t divide_xbyn (const int32_t x, const int n);
const int32_t divide_xbyy (const int32_t x, const int32_t y);