/*
 ============================================================================
 Project Name: uFR ISO 14443-4 ATS Parser
 Name        : file_name.c
 Author      : d-logic
 Version     : 1.0
 Copyright   : 2021.
 Description : Project in C, Ansi-style (Language standard: c99)
 Dependencies: uFR firmware - min. version 5.0.34 {define in ini.h}
 uFRCoder library - min. version 5.0.32 {define in ini.h}
 ============================================================================
 */

/* includes:
 * stdio.h & stdlib.h are included by default (for printf and LARGE_INTEGER.QuadPart (long long) use %lld or %016llx).
 * inttypes.h, stdbool.h & string.h included for various type support and utilities.
 * conio.h is included for windows(dos) console input functions.
 * windows.h is needed for various timer functions (GetTickCount(), QueryPerformanceFrequency(), QueryPerformanceCounter())
 */
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#if __WIN32 || __WIN64
#	include <conio.h>
#	include <windows.h>
#elif linux || __linux__ || __APPLE__
#	define __USE_MISC
#	include <unistd.h>
#	include <termios.h>
#	undef __USE_MISC
#	include "conio_gnu.h"
#else
#	error "Unknown build platform."
#endif
#include <uFCoder.h>
#include "ini.h"
#include "uFR.h"
#include "utils.h"
//------------------------------------------------------------------------------
typedef enum E_MENU_ITEMS
{
	TOP_MENU_LEVEL, APDU_MENU_LEVEL
} menu_levels_t;
menu_levels_t menu_level = TOP_MENU_LEVEL;
//------------------------------------------------------------------------------
void usage(void);
void menu(char key);
UFR_STATUS NewCardInField(uint8_t sak, uint8_t *uid, uint8_t uid_len);
void Operation1(void);
void Operation2(void);
void Operation3(void);
//------------------------------------------------------------------------------
int main(void)
{
	char key;
	bool card_in_field = false;
	uint8_t old_sak = 0, old_uid_size = 0, old_uid[10];
	uint8_t sak, uid_size, uid[10];
	UFR_STATUS status;

#if linux || __linux__ || __APPLE__
	_initTermios(0);
#endif

	usage();
	printf(" --------------------------------------------------\n");
	printf("     Please wait while opening uFR NFC reader.\n");
	printf(" --------------------------------------------------\n");

#ifdef __DEBUG
#if __WIN32 || __WIN64
#	define PORT_NAME	"COM3"
#else
#	define PORT_NAME	"/dev/ttyS3"
#endif
	status = ReaderOpenEx(1, PORT_NAME, 1, NULL);
#else
	status = ReaderOpen();
#endif
	if (status != UFR_OK)
	{
		printf("Error while opening device, status is: 0x%08X\n", status);
		getchar();
		return EXIT_FAILURE;
	}
	if (!CheckDependencies())
	{
		ReaderClose();
		getchar();
		return EXIT_FAILURE;
	}
	status = ReaderReset();
	if (status != UFR_OK)
	{
		printf("Error while opening device, status is: 0x%08X\n", status);
		getchar();
		return EXIT_FAILURE;
	}

	printf(" --------------------------------------------------\n");
	printf("        uFR NFC reader successfully opened.\n");
	printf(" --------------------------------------------------\n");

	do
	{
		while (!_kbhit())
		{
			status = GetCardIdEx(&sak, uid, &uid_size);
			switch (status)
			{
				case UFR_OK:
					if (card_in_field)
					{
						if (old_sak != sak || old_uid_size != uid_size || memcmp(old_uid, uid, uid_size))
						{
							old_sak = sak;
							old_uid_size = uid_size;
							memcpy(old_uid, uid, uid_size);
							NewCardInField(sak, uid, uid_size);
						}
					}
					else
					{
						old_sak = sak;
						old_uid_size = uid_size;
						memcpy(old_uid, uid, uid_size);
						NewCardInField(sak, uid, uid_size);
						card_in_field = true;
					}
					break;
				case UFR_NO_CARD:
					menu_level = TOP_MENU_LEVEL;
					card_in_field = false;
					status = UFR_OK;
					break;
				default:
					ReaderClose();
					printf(" Fatal error while trying to read card, status is: 0x%08X\n", status);
					getchar();
					return EXIT_FAILURE;
			}
#if __WIN32 || __WIN64
			Sleep(300);
#else // if linux || __linux__ || __APPLE__
			usleep(300000);
#endif
		}

		key = _getch();
		menu(key);
	}
	while (key != '\x1b');

	ReaderClose();
#if linux || __linux__ || __APPLE__
	_resetTermios();
	tcflush(0, TCIFLUSH); // Clear stdin to prevent characters appearing on prompt
#endif
	return EXIT_SUCCESS;
}
//------------------------------------------------------------------------------
void menu(char key)
{

	switch (key)
	{
		case '1':
			if (menu_level == APDU_MENU_LEVEL)
				Operation1();
			break;

		case '2':
			if (menu_level == APDU_MENU_LEVEL)
				Operation2();
			break;

		case '3':
			if (menu_level == APDU_MENU_LEVEL)
				Operation3();
			break;

		case '\x1b':
			break;

		default:
			usage();
			break;
	}
}
//------------------------------------------------------------------------------
void usage(void)
{
	switch (menu_level)
	{
		case APDU_MENU_LEVEL:
			printf(" -------------------------------------------------------------------\n");
			printf(" ISO14443-4 tag detected, choose one of the supported APDU commands:\n");
			printf(" -------------------------------------------------------------------\n");
			printf("  (1) - Option 1\n"
					"  (2) - Option 2\n"
					"  (3) - Option 3\n");
			printf(" -------------------------------------------------------------------\n");
			break;

		default:
			printf(" +------------------------------------------------+\n"
					" |              Application title                 |\n"
					" |                 version "APP_VERSION"                    |\n"
			" +------------------------------------------------+\n");
			printf(" When You put ISO14443-4 tag in the reader field,\n"
					" You will be prompted for appropriate APDU to send.\n"
					"\n"
					"                              For exit, hit escape.\n");
			printf(" --------------------------------------------------\n");
			break;
	}
}
//------------------------------------------------------------------------------
UFR_STATUS NewCardInField(uint8_t sak, uint8_t *uid, uint8_t uid_len)
{
	UFR_STATUS status;
	uint8_t dl_card_type;
    uint8_t ats[MAX_ATS_LEN], loc_uid[MAX_UID_LEN], ats_len, loc_uid_len, loc_sak;

	status = GetDlogicCardType(&dl_card_type);
	if (status != UFR_OK)
		return status;

	printf(" \a-------------------------------------------------------------------\n");
	printf(" Card type: %s, sak = 0x%02X, uid[%d] = ", UFR_DLCardType2String(dl_card_type), sak, uid_len);
	print_hex_ln(uid, uid_len, ":");

	if (dl_card_type == DL_GENERIC_ISO14443_4)
	{
	    status = SetISO14443_4_Mode_GetATS(ats, &ats_len, loc_uid, &loc_uid_len, &loc_sak);
	    if (status != UFR_OK) {
	        printf(" Card removed from the field\n");
	        printf(" -------------------------------------------------------------------\n");
	        s_block_deselect(10);
	        return status;
	    }

	    if (uid_len != loc_uid_len || memcmp(uid, loc_uid, uid_len)) {
	        printf(" -------------------------------------------------------------------\n");
	        s_block_deselect(10);
	        return UFR_NO_CARD;
	    }

	    printf(" Card ATS: ");
	    if (ats[0] > 0 && ats_len > 0) {
	    	print_hex_ln(ats, ats_len, ":");
			printf("  Parsed ATS:\n");
			printf("    TL (ATS length in bytes): %d", ats[0]);
			if (ats[0] != ats_len) {
				printf(" - TL does not agree with the number of bytes returned\n");
			} else if (ats[0] > 1) {
				uint8_t fsc = ats[1] & 0xF, ta, tb, tc, iface_bytes_idx = 2;
				bool b_ta = ats[1] & (1 << 4), b_tb = ats[1] & (1 << 5), b_tc = ats[1] & (1 << 6);

				printf("\n    T0 = 0x%02X meaning: FSCI = 0x%X =>", ats[1], fsc);
				if (fsc < 9) {
					printf(" FSC = %d bytes\n", fsc < 5 ? fsc * 8 + 16 : fsc < 8 ? (fsc - 5) * 32 + 64 : 256);
					printf("                       FSC defines the maximum size of a frame accepted by the PICC\n");
				} else {
					printf("FSC violates RFU recommendation\n");
				}

				if (b_ta) {
					ta = ats[iface_bytes_idx++];
					printf("                       *Interface byte TA is present\n");
				}

				if (b_tb) {
					tb = ats[iface_bytes_idx++];
					printf("                       *Interface byte TB is present\n");
				}

				if (b_tc) {
					tc = ats[iface_bytes_idx++];
					printf("                       *Interface byte TC is present\n");
				}

				if (b_ta) {
					printf("    TA = 0x%02X meaning: %s,\n", ta,
							ta & (1 << 7) ?
									"Only the same bit rate for both directions supported" :
									"Different bit rate for each direction supported");
					printf("         Bit rate capability for the PICC --> PCD direction: ");
					if (!(ta & 0x70)) {
						printf("106 kbps only\n");
					} else {
						printf("supported 106");
						if (ta & (1 << 4)) {
							printf(", 212");
						}
						if (ta & (1 << 5)) {
							printf(", 424");
						}
						if (ta & (1 << 6)) {
							printf(", 848");
						}
						printf(" kbps\n");
					}

					printf("         Bit rate capability for the PCD --> PICC direction: ");

					if (!(ta & 0x7)) {
						printf("106 kbps only\n");
					} else {
						printf("supported 106");
						if (ta & 1) {
							printf(", 212");
						}
						if (ta & (1 << 1)) {
							printf(", 424");
						}
						if (ta & (1 << 2)) {
							printf(", 848");
						}
						printf(" kbps\n");
					}

				} else {
					printf("    ATS does not contains TA and this defined usage of the default values by the standard:\n");
					printf("    106 kbps only supported for both directions\n");
				}

				if (b_tb) {
					printf("    TB = 0x%02X meaning: ", tb);
					printf("FWI = %d => FWT = (256 x 16 / fc) x 2^FWI = %f s\n", tb >> 4,
							tb >> 4 ? (256.0 * 16.0 / 13.56e6) * pow(2, tb >> 4) : 0);
					printf("                       SFGI = %d => SFGT = (256 x 16 / fc) x 2^SFGI = %f s\n", tb & 0xF,
							tb & 0xF ? (256.0 * 16.0 / 13.56e6) * pow(2, (tb & 0xF)) : 0);
				} else {
					printf("    ATS does not contains TB and this defined usage of the default values by the standard:\n");
					printf("    Default value of FWI is 4, which gives a FWT value of ~ 4,8 ms\n");
					printf("    The default value of SFGI is 0, indicates no SFGT needed\n");
				}

				if (b_tc) {
					printf("    TC = 0x%02X meaning: ", tc);
					printf("CID %s supported\n", tc & 2 ? "is" : "isn't");
					printf("                       NAD %s supported\n", tc & 1 ? "is" : "isn't");
				} else {
					printf("    ATS does not contains TC and this defined usage of the default values by the standard:\n");
					printf("    CID is supported and NAD isn't supported\n");
				}
				if (ats[0] > iface_bytes_idx) {
					printf("    There is %d historical bytes: ", ats[0] - iface_bytes_idx);
					print_hex_ln(&ats[iface_bytes_idx], ats[0] - iface_bytes_idx, " ");
					printf("         ASCII historical bytes: ");
					for (size_t i = iface_bytes_idx; i < ats[0]; i++) {
						printf("%c", isprint(ats[i]) ? (char) ats[i] : '?');
					}
					printf("\n");
				}
			} else {
				printf("  ATS does not contains T0 and this defined usage of the default values by the standard:\n");
				printf("    FSCI = 2 => FSC = 32 bytes\n");
				printf("                FSC defines the maximum size of a frame accepted by the PICC\n");
			}
	    } else {
	    	printf(" ATS is not parsable\n");
	    }
	    printf(" -------------------------------------------------------------------\n");
	    s_block_deselect(10);

		//menu_level = APDU_MENU_LEVEL;
		//usage();
	}
	else if (menu_level == APDU_MENU_LEVEL)
	{
	    printf(" -------------------------------------------------------------------\n");
		menu_level = TOP_MENU_LEVEL;
	}

	return UFR_OK;
}
//------------------------------------------------------------------------------
void Operation1(void)
{
	printf("1-------------------------------------------------------------------\n");
}
//------------------------------------------------------------------------------
void Operation2(void)
{
	printf("2-------------------------------------------------------------------\n");
}
//------------------------------------------------------------------------------
void Operation3(void)
{
	printf("3-------------------------------------------------------------------\n");
}
//------------------------------------------------------------------------------
