#include <stdint.h>
#include <debug.h>
#include <stdbool.h>
#include <stddef.h>
#include "threads/float.h"

const int32_t convert_ntox (const int n) {
    // TODO: implement four base operations
   return n*f;
}
const int convert_xton (const int32_t x) {
    // TODO: implement four base operations
    // rounding to nearest
    if (x>=0) return (x+f/2)/f;
    else if(x<=0) return (x-f/2)/f;
    else return x/f;
}

const int32_t add_xandn (const int32_t x, const int n) {
    // TODO: implement four base operations
    return x+n*f;
}
const int32_t add_xandy (const int32_t x, const int32_t y) {
    // TODO: implement four base operations
    return x+y;
}

const int32_t sub_nfromx (const int32_t x, const int n) {
    // TODO: implement four base operations
    return x-n*f;
}
const int32_t sub_yfromx (const int32_t x, const int32_t y) {
    // TODO: implement four base operations
    return x-y;
}

const int32_t mult_xbyn (const int32_t x, const int n) {
    // TODO: implement four base operations
    return x * n;
}
const int32_t mult_xbyy (const int32_t x, const int32_t y) {
    // TODO: implement four base operations
    return ((int64_t)x)*y/f;
}

const int32_t divide_xbyn (const int32_t x, const int n) {
    // TODO: implement four base operations
    return x/n;
}
const int32_t divide_xbyy (const int32_t x, const int32_t y) {
    // TODO: implement four base operations  
    return ((int64_t)x)*f/y;
}