#include <gf256table.h>
#include <gf256.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

void poly_mul(uint8_t poly[], int len, uint8_t factor) {
	uint8_t *copy = malloc(len);
	bzero(copy, len);
	memcpy(copy + 1, poly, len - 1);
	for (int i = 0; i < len; i++) {
		copy[i] ^= gf256_mul[poly[i]][factor];
	}
	memcpy(poly, copy, len);
	free(copy);
}

uint8_t gf256_pow(uint8_t b, uint8_t exp) {
	uint8_t res = 1;
	while (exp > 0) {
		if (exp & 1) res = gf256_mul[res][b];
		b = gf256_mul[b][b];
		exp >>= 1;
	}
	return res;
}

uint8_t gf256_polycalc(uint8_t poly[], int len, uint8_t x) {
	uint8_t res = 0;
	while (len--) {
		res ^= gf256_mul[poly[len]][gf256_pow(x, len)];
	}
	return res;
}

void gf_polyreg(uint8_t data[], uint8_t i_size, uint8_t out[], int frame_size) {
	bzero(out, i_size);
	for (uint8_t i = 0; i < i_size; i++) {

		// build polynomial excluding (x - i)
		uint8_t poly[i_size]; bzero(poly, i_size);
		poly[0] = 1;
		for (uint8_t j = 0; j < i_size; j++) {
			if (j != i) poly_mul(poly, i_size, j);
		}

		// compute coefficient = GF(256)[x]/p(x)
		uint8_t coeff = gf256_mul[data[i * frame_size]][gf256_inverse[gf256_polycalc(poly, i_size, i)]];

		// accumulate regressed polynomial
		for (uint8_t j = i_size; j--;) {
			out[j] ^= gf256_mul[coeff][poly[j]];
		}
	}
}

void gf_polyreg_indexed(uint8_t data[], uint8_t indices[], uint8_t i_size, uint8_t out[], int frame_size) {
	bzero(out, i_size);
	// index from indices
	for (uint8_t *i = indices; i < indices + i_size; i++) {
		uint8_t poly[i_size]; bzero(poly, i_size);
		poly[0] = 1;
		for (uint8_t *j = indices; j < indices + i_size; j++) {
			if (j != i) poly_mul(poly, i_size, *j);
		}

		uint8_t coeff = gf256_mul[data[*i * frame_size]][gf256_inverse[gf256_polycalc(poly, i_size, *i)]];

		for (uint8_t j = i_size; j--;) {
			out[j] ^= gf256_mul[coeff][poly[j]];
		}
	}
}
