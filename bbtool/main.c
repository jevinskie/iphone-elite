/*
  Copyright (c) 2007 onitake
  Thanks & kudos to:
  - geohot for norz
  - gray and the dev team for reverse engineering and iUnlock/anySIM

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
	
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "baseband.h"

static const unsigned int BUFFER_SHIFT = 10;
static const unsigned int BUFFER_SIZE = 1024;
static const unsigned int BUFFER_MASK = 1023;

void usage() {
	fprintf(stderr, "bbtool - bbupdater replacement under GPL license\n");
	fprintf(stderr, "Usage: bbtool [-v] [-h] [-D device] [-B baud rate] [-s secpack] [-b memory address] [-n length] [-r file | -x | -w file]\n");
	//fprintf(stderr, "For compatibility, [-f FLS file], [-f FLS file] and [-e EEP file] are also supported.\n");
	fprintf(stderr, "Address and length may be specified in decimal or hex (with a preceding 0x).\n");
	fprintf(stderr, "If no operation (-x, -r or -w) is given, the baseband will be reset and the boot loader version displayed.\n");
	fprintf(stderr, "   -D: set serial device\n");
	fprintf(stderr, "   -B: set baud rate\n");
	fprintf(stderr, "   -b: memory region start\n");
	fprintf(stderr, "   -n: length (can be omitted for write mode)\n");
	fprintf(stderr, "   -s: specify secpack for erase operation\n");
	fprintf(stderr, "   -r: read region into the given file\n");
	fprintf(stderr, "   -x: erase region\n");
	fprintf(stderr, "   -w: write region from the given file\n");
	//fprintf(stderr, "   -f: write flash from FLS file, also use secpack from there if not specified\n");
	//fprintf(stderr, "   -l: write bootloader from FLS file\n");
	//fprintf(stderr, "   -e: write EEPROM area from EEP file\n");
	fprintf(stderr, "   -h: show this help\n");
	fprintf(stderr, "   -v: increase verbosity (default is INFO, -v raises to DEBUG)\n");
}

int readFlash(int fd, const char *output, unsigned int start, unsigned int length) {
	if (!output) {
		fprintf(stderr, "Error, no output file specified.\n");
		return 10;
	}
	
	fprintf(stderr, "Reading from 0x%08x to 0x%08x\n", start, start + length);
	if (start == 0 || length == 0) {
		fprintf(stderr, "Invalid address/length\n");
		return 5;
	}
		
	FILE *fp = fopen(output, "wb");
	if (!fp) {
		fprintf(stderr, "Can't open %s: %s\n", output, strerror(errno));
		return 3;
	}
	
	seekBaseband(fd, start);
	
	unsigned char buffer[BUFFER_SIZE];
	unsigned int i;
	for (i = 0; i < length >> BUFFER_SHIFT; i++) {
		unsigned int len = length >> BUFFER_SHIFT;
		fprintf(stderr, "Reading block %d of %d - %d%%\n", i + 1, len, len ? i * 100 / len : 0);
		size_t count = readBaseband(fd, buffer, sizeof(buffer));
		if (fwrite(buffer, 1, count, fp) != count) {
			fprintf(stderr, "Can't write data: %s\n", strerror(errno));
			return 4;
		}
	}
	unsigned int remainder = length & BUFFER_MASK;
	if (remainder) {
		fprintf(stderr, "Reading remainder - 100%%\n");
		size_t count = readBaseband(fd, buffer, remainder);
		if (fwrite(buffer, 1, count, fp) != count) {
			fprintf(stderr, "Can't write data: %s\n", strerror(errno));
			return 4;
		}
	}
	
	fclose(fp);
}

void sendSecPack(int fd, const char *secfile) {
	if (!secfile) {
		fprintf(stderr, "No secpack specified, trying without.\n");
		return;
	}
	
	FILE *fp = fopen(secfile, "rb");
	if (!fp) {
		fprintf(stderr, "Can't open secpack: %s\n", strerror(errno));
		return;
	}
	
	char buffer[SECPACK_SIZE];
	if (fread(buffer, sizeof(buffer), 1, fp) != 1) {
		fprintf(stderr, "Can't read secpack: %s\n", strerror(errno));
		fclose(fp);
		return;
	}
	
	fclose(fp);
	secPack(fd, buffer);
}

int writeFlash(int fd, const char *input, unsigned int start) {
	if (!input) {
		fprintf(stderr, "Error, no input file specified.\n");
		return 10;
	}
	
	FILE *fp = fopen(input, "rb");
	if (!fp) {
		fprintf(stderr, "Can't open %s: %s\n", input, strerror(errno));
		return 3;
	}
	
	fseek(fp, 0, SEEK_END);
	long ofs = (unsigned int) ftell(fp);
	unsigned int length = 0;
	if (ofs != -1) length = (unsigned int) ofs;
	fseek(fp, 0, SEEK_SET);
	
	fprintf(stderr, "Writing to 0x%08x to 0x%08x\n", start, start + length);
	if (start == 0 || length == 0) {
		fprintf(stderr, "Invalid address/length\n");
		return 5;
	}
		
	seekBaseband(fd, start);
	
	unsigned char buffer[BUFFER_SIZE];
	unsigned int i;
	for (i = 0; i < length >> BUFFER_SHIFT; i++) {
		unsigned int len = length >> BUFFER_SHIFT;
		fprintf(stderr, "Writing block %d of %d - %d%%\n", i + 1, len, len ? i * 100 / len : 0);
		size_t count = fread(buffer, 1, sizeof(buffer), fp);
		writeBaseband(fd, buffer, count);
	}
	unsigned int remainder = length & BUFFER_MASK;
	if (remainder) {
		fprintf(stderr, "Writing remainder - 100%%\n");
		size_t count = fread(buffer, 1, remainder, fp);
		writeBaseband(fd, buffer, count);
	}
	
	fclose(fp);
}

int main(int argc, char **argv) {
	setLogLevel(LOGLEVEL_INFO);
	
	const char *device = DEFAULT_BASEBAND_DEVICE;
	unsigned int rate = 921600;
	char mode = '\0';
	char *secpack = NULL;
	char *binfile = NULL;
	unsigned int start = 0;
	unsigned int length = 0;
	
	int ch;
	while ((ch = getopt(argc, argv, "D:B:b:n:s:r:xw:vh")) != -1) {
		switch (ch) {
		case 'v':
			setLogLevel(LOGLEVEL_TRACE);
			break;
		case 'x':
			mode = 'x';
			break;
		case 'h':
			usage();
			return 0;
		case 'D':
			device = optarg;
			break;
		case 'B':
			rate = (unsigned int) strtoul(optarg, NULL, 10);
			break;
		case 'b':
			start = (unsigned int) strtoul(optarg, NULL, 0);
			if (start < 0xa0000000 || start >= 0xb0000000) {
				start = (start & ~0xf0000000) | 0xa0000000;
				fprintf(stderr, "Start address is not in flash range, corrected to 0x%08x.\n", start);
			}
			break;
		case 'n':
			length = (unsigned int) strtoul(optarg, NULL, 0);
			break;
		case 's':
			secpack = optarg;
			break;
		case 'r':
			mode = 'r';
			binfile = optarg;
			break;
		case 'w':
			mode = 'w';
			binfile = optarg;
			break;
		default:
			usage();
			return 1;
		}
	}
	
	resetBaseband();
	int fd = openBaseband(device);
	if (fd == -1) {
		fprintf(stderr, "Can't open %s: %s", device, strerror(errno));
		return 2;
	}
	setBaudRate(fd, rate);
	
	VersionPacket version = getBootVersion(fd);
	fprintf(stderr, "Got %d.%d, %s\n", version.major, version.minor, version.major <= 3 ? "GOOD." : "BAD. You need a secpack that is more recent than the installed firmware.");
	
	prepareFlash(fd);
	
	int err = 0;
	switch (mode) {
	case 'x':
		sendSecPack(fd, secpack);
		eraseBaseband(fd, start, start + length);
		break;
	case 'r':
		err = readFlash(fd, binfile, start, length);
		break;
	case 'w':
		sendSecPack(fd, secpack);
		eraseBaseband(fd, start, start + length);
		err = writeFlash(fd, binfile, start);
		break;
	default:
		break;
	}
	
	close(fd);

	return err;
}