/*!
  @header iPhone baseband library

  This library is easily integratable into programs that must access
  the iPhone baseband flash.
  
  As stated in the GPL license below, there is NO WARRANTY, using
  this code inappropriately can cause permanent damage to your
  iPhone.
  
  Supported commands include:
  <ul>
  <li>Reading</li>
  <li>Writing</li>
  <li>Sending a secpack</li>
  </ul>
  
  Thanks & kudos to: geohot for reverse engineering iUnlock/anySIM and
  gray and the dev team for reverse engineering bbupdater and the baseband
  
  As the dev team changed their stance towards use of their code, this might be
  duplicate effort. In fact, I've started working on a modified version
  of their first opensource release that would build correctly with
  the 0.5 toolchain, but decided to start over to avoid license issues.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
	
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <a href=http://www.gnu.org/licenses/>http://www.gnu.org/licenses/</a>.

  @copyright 2007 onitake
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

/*!
Utitility function that will print a hex dump of the given buffer
*/
void printBuffer(void *buffer, size_t length);

/*!
Send a reset signal to the baseband via IOKit
*/
void resetBaseband();

const char *DEFAULT_BASEBAND_DEVICE;
/*!
Open the given baseband port and configure the serial port for 115000bps
*/
int openBaseband(const char *device);

/*!
Change the baud rate
"Good" values include 921600 and 750000
*/
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
/*!
Get version information from the baseband
*/
VersionPacket getBootVersion(int fd);

/*!
Send the CFI commands (prepare for flashing?)
*/
void prepareFlash(int fd);

/*!
Sets the offset for read and write.
The offset will be updated by those commands.
*/
void seekBaseband(int fd, unsigned int offset);

/*!
Read a block of data from the current address.

Maximum block size is 2048 (or 1024?) bytes (use multiple calls to get more data).

Interesting locations:
<ul>
<li>0xa0020000 start of main firmware</li>
<li>0xa002c000 (original?) firmware version string (SGOLD2 something)</li>
<li>0xa0030000 firmware version string</li>
<li>0xa0310000 last block of main firmware (one block is 0x10000=65536 bytes)</li>
<li>0xa03a0000 seczone (0x800=2048 bytes) or</li>
<li>0xa03e0000 seczone (which one is it?)</li>
</ul>
*/
size_t readBaseband(int fd, void *buffer, unsigned short size);

/*!
Write a block of data to the current flash address.

Maximum block size is 2048 (or 1024?) bytes (use multiple calls to write more data).
*/
void writeBaseband(int fd, void *data, size_t length);

/*!
Erase one or multiple flash blocks

Only certain combinations make sense.
The boot loader will automatically change the value to a sane range,
like 0xa0020000 to 0xa0310000 (length 0x2F0000).
*/
void eraseBaseband(int fd, unsigned int begin, unsigned int end);

#define SECPACK_SIZE 2048
/*!
Send the secpack

The supplied secpack must be exactly 2048 bytes long
and match (boot loader 3.9 and before)
or supercede (all versions) the baseband firmware currently installed.
*/
void secPack(int fd, void *secpack);

/*!
Send the end secpack command (how useful is that, actually?) after flashing.
*/
void endSecPack(int fd);
