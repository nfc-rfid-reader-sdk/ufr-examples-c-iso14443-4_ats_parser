/*
 * utils.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "ini.h"
#include "utils.h"

//------------------------------------------------------------------------------
size_t hex2bin(uint8_t *dst, const char *src) {
	size_t dst_len = 0;
	char s_tmp[3];

	s_tmp[2] = '\0';

	while (*src) {
		while (((char)*src < '0' || (char)*src > '9')
				&& ((char)*src < 'a' || (char)*src > 'f')
				&& ((char)*src < 'A' || (char)*src > 'F'))
			src++; // skip delimiters, white spaces, etc.

		s_tmp[0] = (char) *src++;

		// Must be pair of the hex digits:
		if (!(*src))
			break;

		// And again, must be pair of the hex digits:
		if (((char)*src < '0' || (char)*src > '9')
				&& ((char)*src < 'a' || (char)*src > 'f')
				&& ((char)*src < 'A' || (char)*src > 'F'))
			break;

		s_tmp[1] = (char) *src++;

		*dst++ = strtoul(s_tmp, NULL, 16);
		dst_len++;
	}

	return dst_len;
}
//------------------------------------------------------------------------------
void print_ln_len(char symbol, uint8_t cnt) {

	for (int i = 0; i < cnt; i++)
		printf("%c", symbol);

	printf("\n");
}
//------------------------------------------------------------------------------
inline void print_ln(char symbol) {

	print_ln_len(symbol, DEFAULT_LINE_LEN);
}
//------------------------------------------------------------------------------
void print_hex(const uint8_t *data, uint32_t len, const char *delimiter) {

	for (int i = 0; i < len; i++) {
		printf("%02X", data[i]);
		if ((delimiter != NULL) && (i < (len - 1)))
			printf("%c", *delimiter);
	}
}
//------------------------------------------------------------------------------
void print_hex_ln(const uint8_t *data, uint32_t len, const char *delimiter) {

	print_hex(data, len, delimiter);
	printf("\n");
}
//==============================================================================



