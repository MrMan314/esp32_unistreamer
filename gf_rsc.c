#include <stdio.h>
#include <gf256.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define FEC_RATE 3

char s[] = "skibidi toilet";

void poly_mod(uint8_t a[], int a_len, uint8_t b[], int b_len, uint8_t rem[]) {
	memcpy(rem, a, a_len);
	for (int i = 0; i <= a_len - b_len; i++) {
		uint8_t factor = rem[i];
		if (!factor) continue;
		for (int j = 0; j < b_len; j++) {
			rem[j + i] ^= gf256_mul(factor, b[b_len - j - 1]);
		}
	}
}

int main() {
	int size = sizeof(s), eccsize = (sizeof(s) + FEC_RATE - 1)/FEC_RATE;

	// generation
	uint8_t code[size];
	gf_polyreg(s, size, code, 1);

	// determine parity bytes
	uint8_t recv[size + eccsize];
	memcpy(recv, s, size);
	for (int i = 0; i < eccsize; i++) {
		recv[size + i] = gf256_polycalc_exp2(code, size, size + i);
	}

	printf("FEC RATE: %d/%d\n", FEC_RATE, FEC_RATE + 1);
	// print the tx string
	for (int i = 0; i < size + eccsize; i++) {
		printf("%02x ", recv[i]);
	}
	putchar(10);
	
	
	// new encoding thing
	uint8_t g[eccsize + 1];
	bzero(g, eccsize + 1);
	g[0] = 1;
	for (int i = 0; i < eccsize; i++) {
		poly_mul(g, size, gf256_exp2[i]);
	}
	for (int i = eccsize + 1; i--;) {
		printf("%02x ", g[i]);
	}
	putchar(10);

	uint8_t parity[size], b[size];
	for (int i = 0; i < size; i++) {
		b[size - i - 1] = s[i];
	}
	poly_mod(s, size, g, eccsize + 1, parity);
	
	for (int i = 0; i < size; i++) {
		printf("%02x ", parity[i]);
	}

	putchar(10);
	// decoding

	// simulate packet loss
	uint8_t indices[] = {2, 4, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 19};
	gf_polyreg_indexed(recv, indices, size, code, 1);

	uint8_t recover[size];

	for (int i = size; i--;) {
		recover[i] = gf256_polycalc(code, size, gf256_exp2[i]);
	}

	printf("%s\n", recover);
}

