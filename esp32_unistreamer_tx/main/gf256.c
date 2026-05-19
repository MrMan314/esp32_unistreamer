#include <gf256table.h>
#include <gf256.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

uint8_t gf256_mul(uint8_t a, uint8_t b) {
	if (a && b) {
		uint16_t log_sum = gf256_log2[a] + gf256_log2[b];
		if (log_sum >= 255) log_sum -= 255;
		return gf256_exp2[log_sum];
	}
	return 0;
}

void poly_mul(uint8_t poly[], int len, uint8_t factor) {
	uint8_t *copy = malloc(len);
	bzero(copy, len);
	memcpy(copy + 1, poly, len - 1);
	for (int i = 0; i < len; i++) {
		copy[i] ^= gf256_mul(poly[i], factor);
	}
	memcpy(poly, copy, len);
	free(copy);
}

uint8_t gf256_pow(uint8_t b, uint32_t exp) {
	exp %= 255;
	uint8_t res = 1;
	while (exp > 0) {
		if (exp & 1) res = gf256_mul(res, b);
		b = gf256_mul(b, b);
		exp >>= 1;
	}
	return res;
}

uint8_t gf256_polycalc(uint8_t poly[], int len, uint8_t x) {
	uint8_t res = 0;
	while (len--) {
		res ^= gf256_mul(poly[len], gf256_pow(x, len));
	}
	return res;
}

uint8_t gf256_polycalc_exp2(uint8_t poly[], int len, uint8_t x) {
	uint8_t res = 0;
	while (len--) {
		res ^= gf256_mul(poly[len], gf256_exp2[(x * len) % 255]);
	}
	return res;
}

void gf_polyreg(uint8_t data[], uint8_t size, uint8_t out[], int frame_size) {
	if (size > 39 || size < 1) return;

	int offset =  size * (size - 1) * (2 * size - 1) / 6;

	uint8_t col[39];
	for (int i = 0; i < size; i++) {
		col[i] = data[i * frame_size];
	}

	for (int i = 0; i < size; i++) {
		uint8_t result = 0;
		for (int j = 0; j < size; j++) {
			result ^= gf256_mul(matrices[offset + i * size + j], col[j]);
		}
		out[i] = result;
	}
}

void gf_polyreg_indexed(uint8_t data[], uint8_t indices[], uint8_t i_size, uint8_t out[], int frame_size) {
	bzero(out, i_size);
	// index from indices
	for (uint8_t *i = indices; i < indices + i_size; i++) {
		uint8_t poly[i_size]; bzero(poly, i_size);
		poly[0] = 1;
		for (uint8_t *j = indices; j < indices + i_size; j++) {
			if (j != i) poly_mul(poly, i_size, gf256_exp2[*j]);
		}

		uint8_t coeff = gf256_mul(data[*i * frame_size], gf256_inverse[gf256_polycalc(poly, i_size, gf256_exp2[*i])]);

		for (uint8_t j = i_size; j--;) {
			out[j] ^= gf256_mul(coeff, poly[j]);
		}
	}
}
