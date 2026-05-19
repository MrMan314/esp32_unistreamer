#include <stdio.h>
#include <stdint.h>
#include <gf256.h>
#include <stdlib.h>
#include <string.h>

//const char s[] = "skibidi toilet";
//const size_t size = sizeof(s);

int main() {
	printf("const uint8_t matrices[] = {");
	for (int size = 1; size < 40; size++) {
		uint8_t mat[size][size], inv[size][size];
		bzero(mat, size * size);
		bzero(inv, size * size);

		// create identity in inverse
		for (int i = 0; i < size; i++) {
			inv[i][i] = 1;
		}

		// create vandemronde
		for (int i = 0; i < size; i++) {
			for (int j = 0; j < size; j++) {
				mat[i][j] = gf256_exp2[(i * j) % 255];
			}
		}

		// invert vandermonde
		// there do exist edge cases where this code fails to invert, but it will never happen
		for (int i = 0; i < size; i++) {
			for (int j = 0; j < size; j++) {
				uint8_t factor = mat[j][i];
				for (int k = 0; k < size; k++) {
					mat[j][k] = gf256_mul(mat[j][k], gf256_inverse[factor]);
					inv[j][k] = gf256_mul(inv[j][k], gf256_inverse[factor]);
				}
			}

			for (int j = 0; j < size; j++) {
				if (j != i) {
					for (int k = 0; k < size; k++) {
						mat[j][k] ^= mat[i][k];
						inv[j][k] ^= inv[i][k];
					}
				}
			}

		}
		for (int i = 0; i < size; i++) {
			uint8_t factor = mat[i][i];
			for (int j = 0; j < size; j++) {
				mat[i][j] = gf256_mul(mat[i][j], gf256_inverse[factor]);
				inv[i][j] = gf256_mul(inv[i][j], gf256_inverse[factor]);
			}
		}

		// print vandermonde
		for (int i = 0; i < size; i++) {
			for (int j = 0; j < size; j++) {
				printf("%d,", inv[i][j]);
			}
//			putchar(10);
		}
//		putchar(10);
	}
	printf("};\n");
/*
	// compute coefficients
	uint8_t poly[size]; bzero(poly, size);

	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			poly[i] ^= gf256_mul(inv[i][j], s[j]);
		}
	}
	
	// print polynomial
	for (int i = 0; i < size; i++) {
		printf("%02x ", poly[i]);
	}
	putchar(10);

	// check regressed polynomial
	for (int i = 0; i < size; i++) {
		printf("%c", gf256_polycalc_exp2(poly, size, i));
	}
	putchar(10);
*/
}
