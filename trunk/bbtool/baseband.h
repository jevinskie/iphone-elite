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

#include <unistd.h>

enum {
	LOGLEVEL_ERROR = 1,
	LOGLEVEL_WARN,
	LOGLEVEL_INFO,
	LOGLEVEL_STATUS,
	LOGLEVEL_DEBUG,
	LOGLEVEL_TRACE,
};
void setLogLevel(int level);
int getLogLevel();

// utitility function that will print a hex dump of the given buffer
void printBuffer(void *buffer, size_t length);

// send a reset signal to the baseband via IOKit
void resetBaseband();

const char *DEFAULT_BASEBAND_DEVICE;
// open the given baseband port and configure the serial port for 115000bps
int openBaseband(const char *device);

// change the baud rate
// "good" values include 921600 and 750000
void setBaudRate(int fd, unsigned int speed);

typedef struct {
	unsigned int unknown1;
	unsigned short unknown2;
	unsigned short major;
	unsigned short minor;
	unsigned short micro;
	// no \0 terminator!
	char text[8];
} VersionPacket;
// get version information from the baseband
VersionPacket getBootVersion(int fd);

// send the CFI commands (prepare for flashing?)
void prepareFlash(int fd);

// sets the offset for read and write
// the offset will be updated by those commands
void seekBaseband(int fd, unsigned int offset);

// like read()
// maximum size is 2048 (or 1024?) bytes (use multiple calls to get more data)
// interesting locations:
// 0xa0020000 start of main firmware
// 0xa002c000 (original?) firmware version string (SGOLD2 something)
// 0xa0030000 firmware version string
// 0xa0310000 last block of main firmware (one block is 0x10000=65536 bytes)
// 0xa03e0000 seczone (0x800=2048 bytes)
size_t readBaseband(int fd, void *buffer, unsigned short size);

// like write()
// maximum size is 2048 (or 1024?) bytes (use multiple calls to write more data)
void writeBaseband(int fd, void *data, size_t length);

// erase one or multiple flash blocks
// only certain combinations make sense
// the boot loader will automatically change the value to a sane range
// like 0xa0020000 to 0xa0310000 (length 0x2F0000)
void eraseBaseband(int fd, unsigned int begin, unsigned int end);

// send the secpack
// the block must be exactly 2048 bytes long
// and match (boot loader 3.9 and before)
// or supercede (all versions) the baseband firmware currently installed
const unsigned int SECPACK_SIZE;
void secPack(int fd, void *secpack);

// send the end secpack command (how useful is that, actually?)
// after flashing
void endSecPack(int fd);
