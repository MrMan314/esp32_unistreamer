#include <stdio.h>
#include <stdint.h>
#include <gf256.h>

uint8_t s[] = "among us";
size_t len = sizeof(s);

int main() {
	uint8_t poly[len];
	gf_polyreg(s, len, poly, 1);

	for (int i = 0; i < len; i++) {
		printf("%c", gf256_polycalc_exp2(poly, len, i));
	}putchar(10);
}
