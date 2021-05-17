/*
 * uFR.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <uFCoder.h>
#include "ini.h"
#include "uFR.h"

//------------------------------------------------------------------------------
bool CheckDependencies(void) {
#if defined(EXIT_ON_WRONG_FW_DEPENDENCY) || defined(EXIT_ON_WRONG_LIB_DEPENDENCY) || defined(DISPLAY_FW_VER) || defined(DISPLAY_LIB_VER)
	uint8_t version_major, version_minor, build;
	UFR_STATUS status;
#elif defined(EXIT_ON_WRONG_FW_DEPENDENCY) || defined(EXIT_ON_WRONG_LIB_DEPENDENCY)
    bool wrong_version = false;
#endif

#if defined(EXIT_ON_WRONG_LIB_DEPENDENCY) || defined(DISPLAY_LIB_VER)
	uint32_t dwDllVersion = 0;

	dwDllVersion = GetDllVersion();

	// "explode" the uFCoder library version:
	version_major = (uint8_t)dwDllVersion;
	version_minor = (uint8_t)(dwDllVersion >> 8);

	// Get the uFCoder library build number.
	build = (uint8_t)(dwDllVersion >> 16);
#endif
#ifdef DISPLAY_LIB_VER
	printf(" uFCoder library version: %d.%d.%d\n", version_major, version_minor, build);
#endif
#ifdef EXIT_ON_WRONG_LIB_DEPENDENCY
	if (version_major < MIN_DEPEND_LIB_VER_MAJOR) {
		wrong_version = true;
	} else if (version_major == MIN_DEPEND_LIB_VER_MAJOR && version_minor < MIN_DEPEND_LIB_VER_MINOR) {
		wrong_version = true;
	} else if (version_major == MIN_DEPEND_LIB_VER_MAJOR && version_minor == MIN_DEPEND_LIB_VER_MINOR && build < MIN_DEPEND_LIB_VER_BUILD) {
		wrong_version = true;
	}

	if (wrong_version) {
		printf("Wrong uFCoder library version (%d.%d.%d).\n"
			   "Please update uFCoder library to at last %d.%d.%d version.\n",
			   version_major, version_minor, build,
			   MIN_DEPEND_LIB_VER_MAJOR, MIN_DEPEND_LIB_VER_MINOR, MIN_DEPEND_LIB_VER_BUILD);
		return false;
	}
#endif
#if defined(EXIT_ON_WRONG_FW_DEPENDENCY) || defined(DISPLAY_FW_VER)
	status = GetReaderFirmwareVersion(&version_major, &version_minor);
	if (status != UFR_OK) {
		printf("Error while checking firmware version, status is: 0x%08X\n", status);
	}
	status = GetBuildNumber(&build);
#endif
#ifdef DISPLAY_FW_VER
    printf(" uFR firmware version: %d.%d.%d\n", version_major, version_minor, build);
#endif
#ifdef EXIT_ON_WRONG_FW_DEPENDENCY
    wrong_version = false;
	if (status != UFR_OK) {
		printf("Error while firmware version, status is: 0x%08X\n", status);
	}
	if (version_major < MIN_DEPEND_FW_VER_MAJOR) {
		wrong_version = true;
	} else if (version_major == MIN_DEPEND_FW_VER_MAJOR && version_minor < MIN_DEPEND_FW_VER_MINOR) {
		wrong_version = true;
	} else if (version_major == MIN_DEPEND_LIB_VER_MAJOR && version_minor == MIN_DEPEND_FW_VER_MINOR && build < MIN_DEPEND_FW_VER_BUILD) {
		wrong_version = true;
	}

	if (wrong_version) {
		printf("Wrong uFR NFC reader firmware version (%d.%d.%d).\n"
			   "Please update uFR firmware to at last %d.%d.%d version.\n",
			   version_major, version_minor, build,
			   MIN_DEPEND_FW_VER_MAJOR, MIN_DEPEND_FW_VER_MINOR, MIN_DEPEND_FW_VER_BUILD);
		return false;
	}
#endif
	return true;
}
//------------------------------------------------------------------------------

// Ako postoji PDOL, pre slanja komande "Get Processing Options", se mora  sračunati dužinu bajtova i to poslati u pomenutoj komandi.
// Ako PDOL sadrži Terminal Capabilities Tag '9F33' onda se po mnogima mogu poslati sve nule.
// Ako PDOL sadrži Terminal Transaction Qualifiers (TTQ) Tag '9F66' onda tu treba postaviti bar "28 00 00 00"
//                 da ne bi kartica vratila grešku "69 85" = "Conditions of use not satisfied". Radi i sa
//                 "20 00 00 00".
// Posle ovoga pronaći Application File Locator (AFL) Tag '94' za dalje parsiranje - "sfi" & "record range".
// Ako ne postoji PDOL, Onda se šalje GPO komanda u formi: "80 A8 00 00 02 83 00 00" nakon koje se očekuje
// da kartica vrati Response Message Template Format 1 Tag '80' {Contains the data objects (without tags and lengths)
//        returned by the ICC in response to a command}. U tom slučaju prva dva bajta u okviru vraćene binarne vrednosti
//        Taga '80' predstavljaju AIP a ostatak AFL u grupama po 4 bajta koje treba parsirati isto kao Tag '94':
//        - prvi bajt je SFI šiftovan 3 bita levo (za read record cmd samo treba još orovati sa 4)
//        - drugi bajt je prvi record tog SFI-a
//        - treći bajt je poslednji record tog SFI-a
//        - broj zapisa koji učestvuju u "offline data authentication" tog SFI-a počevši od prvog record-a.
// Ako kartica ne vrati Tag '80' onda treba očekivati da vrati tagove '82' za AIP i '94' za AFL.

//	char *sz_hex_r_apdu;
/*			status = ApduCommand("80 A8 00 00 15 83 13 28 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00", &sz_hex_r_apdu, sw);
			if (status != UFR_OK) {
				printf(" Error while executing APDU command, uFR status is: 0x%08X\n", status);
				break;
			} else {
				Ne = strlen(sz_hex_r_apdu) / 2;
				if (Ne) {
					printf(" APDU command executed: response data length = %d bytes\n", (int) Ne);
					printf("  [R] %s\n", sz_hex_r_apdu);
				}
				printf(" [SW] ");
				print_hex_ln(sw, 2, " ");
				if (*sw16_ptr != 0x90) {
					printf(" Could not continue execution due to an APDU error.\n");
				} else {
					hex2bin(r_apdu, sz_hex_r_apdu);
					emv_status = newEmvTag(&temp, r_apdu, Ne, false);
					...CHECK status...
					tail->next = temp;
					tail = tail->next;
				}
			}
*/
