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
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <IOKit/IOKitLib.h>
#include <sys/ioctl.h>
#include <strings.h>
#include <errno.h>

#include "baseband.h"

const char *DEFAULT_BASEBAND_DEVICE = "/dev/tty.baseband";

static const int DEFAULT_SPEED = 115200;
static const int DEFAULT_TIMEOUT = 1;
static const int WRITE_TIMEOUT = 10;

#pragma pack(1)

typedef struct {
	unsigned short w02;
	unsigned short cmd;
	unsigned short dataSize;
} CmdPacket;
static const int CMD_PACKET_SIZE = 6;

typedef struct {
	unsigned short checksum;
	unsigned short w03;
} CmdPacketEnd;
static const int CMD_PACKET_END_SIZE = 4;

static const int VERSION_PACKET_SIZE = 20;
static const int CFI1_PACKET_SIZE = 0x100;

typedef struct {
	unsigned short unknown;
	unsigned char w01, w02, w03, w00;
} SeekReplyPacket;
static const int SEEK_REPLY_PACKET_SIZE = 6;

typedef struct {
	unsigned int begin;
	unsigned int end;
} ErasePacket;
static const int ERASE_PACKET_SIZE = 8;

typedef struct {
  unsigned char done;
  unsigned int current; // the block that is currently being erased
  unsigned char w00;
} EraseStatusReplyPacket;
static const int ERASE_STATUS_REPLY_PACKET_SIZE = 6;

typedef struct {
	unsigned short unknown1;
	unsigned int unknown2;
} SecpackReplyPacket;
static const int SECPACK_REPLY_PACKET_SIZE = 6;

typedef struct {
	unsigned short status;
	unsigned short error;
} WriteReplyPacket;
static const int WRITE_REPLY_PACKET_SIZE = 4;

#pragma pack()

static int _logLevel = LOGLEVEL_WARN;

#define LOGDO(level, command) if (level <= _logLevel) { command; }
#define LOG(level, message...) LOGDO(level, fprintf(stderr, message))
#define PACKET_SIZE(dataSize) (CMD_PACKET_SIZE + CMD_PACKET_END_SIZE + dataSize)

void setLogLevel(int level) {
	_logLevel = level;
}

int getLogLevel() {
	return _logLevel;
}

static char translateChar(char c) {
	if (
		(c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		(c >= '0' && c <= '9') ||
		c == '"' || c == '!' || c == '@' || c == '$' || c == '%' ||
		c == '^' || c == '&' || c == '*' || c == '(' || c == ')' ||
		c == '-' || c == '=' || c == '+' || c == '[' || c == ']' ||
		c == '{' || c == '}' || c == ';' || c == ':' || c == '\'' ||
		c == '\\' || c == '|' || c == ',' || c == '<' || c == '>' ||
		c == '/' || c == '?' || c == '~' || c == '^'
	) {
		return c;
	} else {
		return '.';
	}
}

void printBuffer(void *buffer, size_t length) {
	if (!buffer) {
		return;
	}
	
	size_t i, j;
	for(i = 0, j = 0; i < length; i++) {
		if (i != 0 && (i & 0xf) == 0) {
			printf("\t");
			for (; j < i; j++) {
				printf("%c", translateChar(((char *) buffer)[j]));
			}
			printf("\n");
		}
		printf("%02X ", ((char *) buffer)[i]);
	}
	if (length) {
		size_t k;
		for (k = 0; k < 16 - (length - j); k++) {
			printf("   ");
		}
		printf("\t");
		for (; j < length; j++) {
			printf("%c", translateChar(((char *) buffer)[j]));
		}
		printf("\n");
	}
}

void resetBaseband() {
	LOG(LOGLEVEL_INFO, "Resetting baseband...\n");
	mach_port_t masterPort;
	kern_return_t result = IOMasterPort(MACH_PORT_NULL, &masterPort);
	CFMutableDictionaryRef matchingDict = IOServiceMatching("AppleBaseband");  
	io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault, matchingDict);
	io_connect_t conn;
	result = IOServiceOpen(service, mach_task_self(), 0, &conn);
	result = IOConnectCallScalarMethod(conn, 0, 0, 0, 0, 0);
	IOServiceClose(conn);
}

static unsigned short checksum(CmdPacket *packet, void *data) {
	unsigned short checksum = packet->cmd + packet->dataSize;
	int i;
	for (i = 0; i < packet->dataSize; i++) {
		checksum += ((unsigned char *) data)[i];
	}
	return checksum;
}

// returns a pointer to the data, or NULL if the data was invalid
static void *verifyPacket(void *buffer, size_t length) {
	if (length < CMD_PACKET_SIZE + CMD_PACKET_END_SIZE) {
		LOG(LOGLEVEL_WARN, "Invalid packet size: %lu (minimum is %u)\n", length, CMD_PACKET_SIZE + CMD_PACKET_END_SIZE);
		return NULL;
	}
	CmdPacket *packet = (CmdPacket *) buffer;
	if (packet->w02 != 2) {
		LOG(LOGLEVEL_WARN, "Invalid header: 0x%04x\n", packet->w02);
		return NULL;
	}
	if (length < packet->dataSize + CMD_PACKET_SIZE + CMD_PACKET_END_SIZE) {
		LOG(LOGLEVEL_WARN, "Invalid packet size: %lu (expected is %u)\n", length, packet->dataSize + CMD_PACKET_SIZE + CMD_PACKET_END_SIZE);
		return NULL;
	}
	char *data = &((char *) buffer)[CMD_PACKET_SIZE];
	CmdPacketEnd *end = (CmdPacketEnd *) (&data[packet->dataSize]);
	if (checksum(packet, data) != end->checksum) {
		LOG(LOGLEVEL_WARN, "Invalid checksum: 0x%04x\n", end->checksum);
		return NULL;
	}
	if (end->w03 != 3) {
		LOG(LOGLEVEL_WARN, "Invalid footer: 0x%04x\n", end->w03);
		return NULL;
	}
	LOG(LOGLEVEL_DEBUG, "Packet command: 0x%04x\n", packet->cmd);
	return data;
}

static void writePacket(int fd, short cmd, void *data, size_t length) {
	char *buffer = malloc(PACKET_SIZE(length));
	CmdPacket *packet = (CmdPacket *) buffer;
	packet->w02 = 2;
	packet->cmd = cmd;
	packet->dataSize = length;
	memcpy(buffer + CMD_PACKET_SIZE, data, length);
	CmdPacketEnd *end = (CmdPacketEnd *) (buffer + CMD_PACKET_SIZE + length);
	end->checksum = checksum(packet, data);
	end->w03 = 3;
	
	LOG(LOGLEVEL_TRACE, "Writing packet:\n");
	LOGDO(LOGLEVEL_TRACE, printBuffer(&buffer, PACKET_SIZE(length)));

	write(fd, buffer, PACKET_SIZE(length));
	
	free(buffer);
}

static size_t readPacket(int fd, int timeout, void *buffer, size_t length) {
	fd_set nfd;
	FD_ZERO(&nfd);
	FD_SET(fd, &nfd);
	struct timeval tv = {
		.tv_sec = timeout,
		.tv_usec = timeout ? 0 : 10000
	};
	
	size_t hlen = 0;
	//LOG(LOGLEVEL_INFO, "length=%lu hlen=%lu\n", length, hlen);
	while (length - hlen > 0 && select(fd + 1, &nfd, 0, 0, &tv) > 0) {
		hlen += read(fd, buffer + hlen, length - hlen);
		//LOG(LOGLEVEL_INFO, "hlen=%lu\n", hlen);
		//LOGDO(LOGLEVEL_DEBUG, printBuffer(buffer, hlen));
	}
	
	LOG(LOGLEVEL_TRACE, "Read packet:\n");
	LOGDO(LOGLEVEL_TRACE, printBuffer(buffer, hlen));

	return hlen;
}

int openBaseband(const char *device) {
	LOG(LOGLEVEL_INFO, "Opening %s...\n", device);

	int fd = open(device, O_NOCTTY | O_RDWR);
	ioctl(fd, TIOCNXCL);
	fcntl(fd, F_SETFL, 0);
	struct termios options;
	tcgetattr(fd, &options);
	int tcxonxoff = 0;
	ioctl(fd, 0x8004540A, &tcxonxoff);//_IOW('T', 10, int) = TCXONC?
	cfsetspeed(&options, DEFAULT_SPEED);
	cfmakeraw(&options);
	options.c_cc[VMIN] = 0;
	options.c_cc[VTIME] = 5;
	options.c_iflag = (options.c_iflag | (IGNBRK | IGNPAR)) & ~(ICRNL | IXON | IXOFF | IXANY | INPCK | ISTRIP | BRKINT);
	options.c_oflag = options.c_oflag & ~OPOST;
	options.c_cflag = (options.c_cflag | (CREAD | CS8 | CLOCAL | HUPCL | CRTSCTS)) & ~PARENB;
	options.c_lflag = options.c_lflag & ~(ECHO | ISIG | IEXTEN | ICANON);
	tcsetattr(fd, TCSANOW, &options);
	ioctl(fd, TIOCSDTR);
	ioctl(fd, TIOCCDTR);
	int lineBits = TIOCM_DTR | TIOCM_RTS | TIOCM_CTS | TIOCM_DSR;
	ioctl(fd, TIOCMSET, &lineBits);
	LOG(LOGLEVEL_DEBUG, "Baseband opened.\n");

	char buffer[128] = { '`', '\r' };
	LOG(LOGLEVEL_INFO, "Waiting for initialization...\n");
	size_t length;
	do {
		if (write(fd, buffer, 2) == -1) {
			LOG(LOGLEVEL_ERROR, "Can't write to baseband\n");
			close(fd);
			return -1;
		}
		length = readPacket(fd, 0, buffer, sizeof(buffer));
	} while (length == 0 || buffer[0] != 0xb);
	
	return fd;
}

void setBaudRate(int fd, unsigned int speed) {
	LOG(LOGLEVEL_INFO, "Increasing baud rate to %dbps...\n", speed);
	
	writePacket(fd, 0x82, &speed, 4);
	char buffer[PACKET_SIZE(sizeof(unsigned int))];
	int length = readPacket(fd, DEFAULT_TIMEOUT, buffer, sizeof(buffer));
	unsigned int *setSpeed = (unsigned int *) verifyPacket(buffer, length);
	LOG(LOGLEVEL_DEBUG, "Set baudrate returns: %d, %s\n", setSpeed ? *setSpeed : 0, (speed == (setSpeed ? *setSpeed : 0)) ? "ok" : "NOT ok");
	//LOGDO(LOGLEVEL_DEBUG, printBuffer(buffer, length));

	struct termios options;
	tcgetattr(fd, &options);
	cfsetspeed(&options, speed);
	tcsetattr(fd, TCSANOW, &options);
}

VersionPacket getBootVersion(int fd) {
	LOG(LOGLEVEL_INFO, "Getting bootloader version...\n");
	
	writePacket(fd, 0x801, NULL, 0);
	char buffer[PACKET_SIZE(VERSION_PACKET_SIZE)];
	size_t length = readPacket(fd, 5, buffer, sizeof(buffer));
	VersionPacket *packet = (VersionPacket *) verifyPacket(buffer, length);
	
	if (packet) {
		LOG(LOGLEVEL_DEBUG, "Got bootloader version: %d.%d\n", packet->major, packet->minor);
		//LOGDO(LOGLEVEL_DEBUG, printBuffer(buffer, length));
		return *packet;
	}
	
	VersionPacket zeroBoot;
	memset(&zeroBoot, 0, VERSION_PACKET_SIZE);
	return zeroBoot;
}

void prepareFlash(int fd) {
	LOG(LOGLEVEL_INFO, "Preparing for flashing...\n");
	
	short param = 0;
	writePacket(fd, 0x84, &param, sizeof(param));
	char buffer[PACKET_SIZE(CFI1_PACKET_SIZE)];
	size_t length = readPacket(fd, DEFAULT_TIMEOUT, buffer, sizeof(buffer));
	LOG(LOGLEVEL_DEBUG, "CFI Stage 1 returns:\n");
	LOGDO(LOGLEVEL_DEBUG, printBuffer(verifyPacket(buffer, length), CFI1_PACKET_SIZE));

	writePacket(fd, 0x85, NULL, 0);
	length = readPacket(fd, DEFAULT_TIMEOUT, buffer, sizeof(buffer));
	short *unknown = verifyPacket(buffer, length);
	LOG(LOGLEVEL_DEBUG, "CFI Stage 2 returns: %d\n", unknown ? *unknown : 0);
	//LOGDO(LOGLEVEL_DEBUG, printBuffer(buffer, length));
}

void seekBaseband(int fd, unsigned int offset) {
	LOG(LOGLEVEL_INFO, "Seeking to 0x%08x...\n", offset);
	
	writePacket(fd, 0x802, &offset, sizeof(offset));
	char buffer[PACKET_SIZE(SEEK_REPLY_PACKET_SIZE)];
	size_t length = readPacket(fd, DEFAULT_TIMEOUT, buffer, sizeof(buffer));
	LOG(LOGLEVEL_DEBUG, "Seek returns:\n");
	LOGDO(LOGLEVEL_DEBUG, printBuffer(verifyPacket(buffer, length), SEEK_REPLY_PACKET_SIZE));
}

size_t readBaseband(int fd, void *buffer, unsigned short size) {
	LOG(LOGLEVEL_STATUS, "Reading %u bytes from flash...\n", size);
	
	writePacket(fd, 0x803, &size, sizeof(size));
	usleep(500000);
	void *temp = malloc(PACKET_SIZE(size));
	size_t length = readPacket(fd, DEFAULT_TIMEOUT, temp, PACKET_SIZE(size));
	
	void *ret = verifyPacket(temp, length);
	if (ret) {
		CmdPacket *packet = (CmdPacket *) temp;

		LOG(LOGLEVEL_DEBUG, "Read returns %d bytes:\n", packet->dataSize);
		LOGDO(LOGLEVEL_DEBUG, printBuffer(ret, packet->dataSize));

		if (packet->dataSize <= size) {
			memcpy(buffer, ret, packet->dataSize);
		} else {
			LOG(LOGLEVEL_WARN, "Warning: The returned data does not fit into the buffer!\n");
			memcpy(buffer, ret, size);
		}

		free(temp);
		return packet->dataSize;
	} else {
		LOG(LOGLEVEL_DEBUG, "Read returns: ERROR!\n");

		free(temp);
		return 0;
	}
}

void eraseBaseband(int fd, unsigned int begin, unsigned int end) {
	// correct the end address as the boot loader does
	if (begin == 0xa0020000) end = 0xa0310000;
	
	LOG(LOGLEVEL_INFO, "Erasing flash range 0x%08x-0x%08x...\n", begin, end);
	
	unsigned int base = end - begin;
	if (base == 0) base = 1; // no div by 0, rather wrong values
	
	ErasePacket packet = {
		.begin = begin,
		.end = end
	};
	writePacket(fd, 0x805, &packet, ERASE_PACKET_SIZE);
	char buffer[PACKET_SIZE(sizeof(unsigned short))];
	size_t length = readPacket(fd, WRITE_TIMEOUT, buffer, sizeof(buffer));
	unsigned short *job = verifyPacket(buffer, length);
	LOG(LOGLEVEL_DEBUG, "Erase returns: %d\n", job ? *job : 0);
	
	if (job) {
		EraseStatusReplyPacket *reply;
		int previous = -1;
		do {
			writePacket(fd, 0x806, job, sizeof(*job));
			char buffer2[PACKET_SIZE(ERASE_STATUS_REPLY_PACKET_SIZE)];
			length = readPacket(fd, DEFAULT_TIMEOUT, buffer2, sizeof(buffer2));
			reply = verifyPacket(buffer2, length);
			if (reply) {
				LOG(LOGLEVEL_DEBUG, "Erase status returns: done=%s current=0x%08x w00=%d\n", reply->done ? "yes" : "no", reply->current, reply->w00);
				if (reply->current >= begin && reply->current <= end) {
					int percent = (reply->current - begin) * 100 / base;
					if (percent != previous) {
						LOG(LOGLEVEL_INFO, "Current progress: %u%%\n", percent);
						previous = percent;
					}
				} else if (reply->current == 0xa0000000 && reply->done) {
					LOG(LOGLEVEL_ERROR, "Looks like the erase command failed due to an invalid secpack.\n");
				} else {
					LOG(LOGLEVEL_INFO, "Current position: 0x%08x\n", reply->current);
				}
			}
		} while (reply && !reply->done);
	} else {
		LOG(LOGLEVEL_ERROR, "Erase command failed!\n");
	}
}

void writeBaseband(int fd, void *data, size_t length) {
	LOG(LOGLEVEL_STATUS, "Writing %lu bytes to flash...\n", length);
	
	writePacket(fd, 0x804, data, length);
	char buffer[PACKET_SIZE(WRITE_REPLY_PACKET_SIZE)];
	size_t rlength = readPacket(fd, WRITE_TIMEOUT, buffer, sizeof(buffer));
	WriteReplyPacket *ret = verifyPacket(buffer, rlength);
	if (ret && ret->status == 0) {
		LOG(LOGLEVEL_DEBUG, "Write returns: %d/%d\n", ret->status, ret->error);
	} else {
		LOG(LOGLEVEL_ERROR, "Error flashing %lu bytes: %d\n", length, ret->error);
	}
}

void secPack(int fd, void *secpack) {
	LOG(LOGLEVEL_INFO, "Sending secpack...\n");
	
	writePacket(fd, 0x204, secpack, 0x800);
	char buffer[PACKET_SIZE(SECPACK_REPLY_PACKET_SIZE)];
	size_t rlength = readPacket(fd, DEFAULT_TIMEOUT, buffer, sizeof(buffer));
	SecpackReplyPacket *reply = verifyPacket(buffer, rlength);
	LOG(LOGLEVEL_DEBUG, "Secpack returns: unknown1=%d unknown2=0x%x\n", reply ? reply->unknown1 : 0, reply ? reply->unknown2 : 0);
}

void endSecPack(int fd) {
	LOG(LOGLEVEL_INFO, "Ending secpack...\n");
	
	unsigned short unknown = 0;
	writePacket(fd, 0x205, &unknown, sizeof(unknown));
	char buffer[PACKET_SIZE(sizeof(unsigned short))];
	size_t rlength = readPacket(fd, DEFAULT_TIMEOUT, buffer, sizeof(buffer));
	unsigned short *ret = verifyPacket(buffer, rlength);
	LOG(LOGLEVEL_DEBUG, "End secpack returns: %d\n", ret ? *ret : 0);
}
