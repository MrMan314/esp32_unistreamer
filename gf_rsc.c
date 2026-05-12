#include <stdio.h>
#include <gf256.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define FEC_RATE 3

char s[] = "skibidi toilet";

int main() {
	int size = sizeof(s), eccsize = (sizeof(s) + FEC_RATE - 1)/FEC_RATE;

	// generation
	uint8_t code[size];
	gf_polyreg(s, size, code);

	// determine parity bytes
	uint8_t recv[size + eccsize];
	memcpy(recv, s, size);
	for (int i = 0; i < eccsize; i++) {
		recv[size + i] = gf256_polycalc(code, size, size + i);
	}

	printf("FEC RATE: %d/%d\n", FEC_RATE, FEC_RATE + 1);
	// print the tx string
	for (int i = 0; i < size + eccsize; i++) {
		printf("%02x ", recv[i]);
	}
	putchar(10);

	// decoding

	// simulate packet loss
	uint8_t indices[] = {2, 4, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 19};
	gf_polyreg_indexed(recv, indices, size, code);

	uint8_t recover[size];

	for (int i = size; i--;) {
		recover[i] = gf256_polycalc(code, size, i);
	}

	printf("%s\n", recover);
}

