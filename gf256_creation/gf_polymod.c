#include <stdio.h>
#include <stdint.h>

#define BIG_BIT_COUNT 16
#define BIT_COUNT 8

const uint16_t modulus = 0x11b;

void print_poly(uint16_t);

uint16_t poly_mul(uint8_t a, uint8_t b) {
	uint16_t res = 0;
	for (uint8_t i = 0; i < BIT_COUNT; i++) {
		if ((a >> i) & 1) {
			res ^= (uint16_t) b << i;
		}
	}
	return res;
}

uint8_t poly_mod(uint16_t a, uint16_t b) {
	if (!a) return 0;
	uint8_t max_a = BIG_BIT_COUNT, max_b = BIG_BIT_COUNT;

	for (;max_b--;) if (b & (1 << max_b)) break;
	while (1) {
		for (;max_a--;) if (a & (1 << max_a)) break;
		if (max_a < BIT_COUNT) break;
		a ^= b << (max_a - max_b);
	}

	return a;

}

void print_poly(uint16_t poly) {
	bool flag = false;
	for (uint8_t i = 16;i--;) {
		if (poly & (1 << i)) {
			if(flag) putchar('+');
			else flag = true;
			printf(i ? i == 1 ? "x" : "x^%d" : "1", i);
		}
	}
	putchar(10);
}

#define gf256_mul(a, b) poly_mod(poly_mul(a,b),modulus)

uint8_t i, j;

// to check my answers
uint8_t gf_mul_chatgpt(uint8_t a, uint8_t b) {
    uint8_t res = 0;

    while (b) {
        if (b & 1)
            res ^= a;

        uint8_t hi = a & 0x80;
        a <<= 1;

        if (hi)
            a ^= 0x1B; // reduction polynomial

        b >>= 1;
    }

    return res;
}

int main() {

	printf("#ifndef INCLUDE_GF256TABLE_H\n#define INCLUDE_GF256TABLE_H\n#define MODULUS %d\n#include <stdint.h>\nconst uint8_t gf256_mul[256][256]={", modulus);
	for (int i = 0; i < 256; i++) {
		printf("{");
		for (int j = 0; j < 256; j++) {
			printf(j == 255 ? "%d" : "%d,", gf256_mul(i, j) % 256);
		}
		printf(i == 255 ? "}" : "},");
	}
	printf("};\nconst uint8_t gf256_inverse[256]={");
	for (int i = 0; i < 256; i++) {
		int j = 0;
		for (; j < 256; j++) {
			if (gf256_mul(i, j) == 1) break;
		}
		printf(i == 255 ? "%d" : "%d,", j % 256);
	}
	printf("};\n#endif\n");

}
