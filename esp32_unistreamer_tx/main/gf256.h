#ifndef INCLUDE_GF256_H
#define INCLUDE_GF256_H

#include <stdint.h>

extern const uint8_t gf256_inverse[256];
extern const uint8_t gf256_log2[256];
extern const uint8_t gf256_exp2[256];
extern const uint8_t matrices[];

uint8_t gf256_mul(uint8_t a, uint8_t b);
void poly_mul(uint8_t poly[], int len, uint8_t factor);

uint8_t gf256_pow(uint8_t b, uint32_t exp);

uint8_t gf256_polycalc(uint8_t poly[], int len, uint8_t x);
uint8_t gf256_polycalc_exp2(uint8_t poly[], int len, uint8_t x);

void gf_polyreg(uint8_t data[], uint8_t i_size, uint8_t out[], int frame_size);
void gf_polyreg_indexed(uint8_t data[], uint8_t indices[], uint8_t i_size, uint8_t out[], int frame_size);

#endif
