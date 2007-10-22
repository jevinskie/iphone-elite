/*
speedynor: ver 0.1
Copyright (C) 2007 geohot
thanks to gray and the dev team for the i-mode reversing

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


kern_return_t IOConnectCallScalarMethod(io_connect_t,int,int,int,int,int);

struct termios term;
int hlen,t,u,fp;
unsigned char *data, *secpack;
FILE *f;


int InitConn(int speed)
{
  int fd = open("/dev/tty.baseband", O_RDWR | 0x20000 | O_NOCTTY);
  unsigned int blahnull = 0;
  unsigned int handshake = TIOCM_DTR | TIOCM_RTS | TIOCM_CTS | TIOCM_DSR;

  if(fd == -1)
  {
    fprintf(stderr, "%i(%s)\n", errno, strerror(errno));
    exit(1);
  }

  ioctl(fd, 0x2000740D);
  fcntl(fd, 4, 0);
  tcgetattr(fd, &term);

  ioctl(fd, 0x8004540A, &blahnull);
  cfsetspeed(&term, speed);
  cfmakeraw(&term);
  term.c_cc[VMIN] = 0;
  term.c_cc[VTIME] = 5;

  term.c_iflag = (term.c_iflag & 0xFFFFF0CD) | 5;
  term.c_oflag =  term.c_oflag & 0xFFFFFFFE;
  term.c_cflag = (term.c_cflag & 0xFFFC6CFF) | 0x3CB00;
  term.c_lflag =  term.c_lflag & 0xFFFFFA77;

  term.c_cflag = (term.c_cflag & ~CSIZE) | CS8;
  term.c_cflag &= ~PARENB;
  term.c_lflag &= ~ECHO;

  tcsetattr(fd, TCSANOW, &term);

  ioctl(fd, TIOCSDTR);
  ioctl(fd, TIOCCDTR);
  ioctl(fd, TIOCMSET, &handshake);

  return fd;
}


void resetbaseband()
{
  kern_return_t   result;
  mach_port_t     masterPort;
  result = IOMasterPort(MACH_PORT_NULL, &masterPort);
  CFMutableDictionaryRef matchingDict = IOServiceMatching("AppleBaseband");  
  io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault, matchingDict);
  io_connect_t conn;
  result = IOServiceOpen(service, mach_task_self(), 0, &conn);
  result = IOConnectCallScalarMethod(conn, 0, 0, 0, 0, 0);
  IOServiceClose(conn);
}

void getheader(int timeout)
{
	fd_set nfp;
	FD_ZERO(&nfp);
	FD_SET(fp, &nfp);
	struct timeval tv;
	tv.tv_sec=timeout;
	tv.tv_usec=10000;
	hlen=0;
	while(select(fp+1,&nfp,0,0,&tv)>0)
	{
		hlen+=read(fp,data+hlen, 0x10064-hlen);
		//printf("Attempting to read[%d]...%x %x\n",hlen,data[0],data[1]);
	}
}


struct termios options;

void openbaseband()
{
	int t1=0;
	int t2=0x126;
	fp=open("/dev/tty.baseband",0x20002);
	ioctl(fp,0x2000740D);
	fcntl(fp,4,0);
	tcgetattr(fp,&options);
	ioctl(fp,0x8004540A,&t1);
	cfsetspeed(&options,115200);
	cfmakeraw(&options);
	options.c_cc[16]=0;
	options.c_cc[17]=5;
	options.c_iflag=(options.c_iflag | 0x5) & 0xFFFFF0CD;
	options.c_oflag=options.c_oflag & 0xFFFFFFFE;
	options.c_cflag=(options.c_cflag | 0x3CB00) & 0xFFFFEFFF;
	options.c_lflag=options.c_lflag & 0xFFFFFA77;
	tcsetattr(fp,0,&options);
	ioctl(fp,0x20007479);
	ioctl(fp,0x20007478);
	ioctl(fp,0x8004746D,&t2);
	printf("Opened: /dev/tty.baseband\n");
}



void printbuffer()
{
	for(t=0;t<hlen;t++)
	{
		if(t!=0&&t%16==0) printf("\n");
		printf("%2.2X ", data[t]);
	}
	printf("\n");
}


struct cmd_pkt{
	unsigned short int w02;
	unsigned short int cmd;
	unsigned short int data_size;
};

struct cmd_pkt_end{
	unsigned short int checksum;
	unsigned short int w03;
};

struct cmd_pkt mycmdpkt;
struct cmd_pkt_end mycmdpktend;

void cmd_write()
{
	mycmdpkt.w02=2;
	mycmdpktend.w03=3;
	
	mycmdpktend.checksum=0;
	for(t=0;t<mycmdpkt.data_size;t++)
	{
		mycmdpktend.checksum+=data[t];
	}
	mycmdpktend.checksum+=mycmdpkt.cmd+mycmdpkt.data_size;
	write(fp,&mycmdpkt,6);
	write(fp,data,mycmdpkt.data_size);
	write(fp,&mycmdpktend,4);
}

void usage()
{
	printf("Hypher's nor dumper v0.1\n");
	printf("Based on norz by Geohot.\n");
	printf("usage: hnor	<dump_file_name>"
						"\n\t\t[--verbose|-v]"
						"\n\t\t[--force|-f]"
						"\n\t\t[--reset|-r]"
						"\n\t\t[--start|-s <start>]"
						"\n\t\t[--skip|-k <skip>]"
						"\n\t\t[--length|-l <len>]\n"
						"\n\t\t[--seczone]\n"
		   "Note: All address parameters are treated as hex values.\n\n");
}

void help()
{
	usage();

	printf("All options must be passed separately, that is, no POSIX combining of arguments.\n");
	printf("\n");
	printf("--help (-? or -h):\n");
	printf("	Show this information.\n");
	printf("\n");
	printf("--seczone:\n");
	printf("	Shortcut for dumping your seczone, equilivent to -s 03fa000 -l 2000\n");
	printf("\n");
	printf("--force (-f):\n");
	printf("	Use whatever values are given, and don't assume anything. Also auto-reformats your hard disk.\n");
	printf("\n");
	printf("--verbose (-f):\n");
	printf("	Be really wordy about whats going on, to the point that you have no idea whats going on at all, because of all the useless information.\n");
	printf("\n");
	printf("--start (-s) <address>:\n");
	printf("	Start dumping the NOR at <address>.\n");
	printf("\n");
	printf("--skip (-k) <offset> <count>:\n");
	printf("	After dumping <length> bytes, jump <offset> bytes and repeat, <count> times. Note that <count> should be a decimal value, and cannot be omitted.\n");
	printf("\n");
	printf("--length (-l) <length>:\n");
	printf("	Dump <length> bytes of the nor at each address dumped.\n");
	printf("\n");
	printf("\n");	
	printf("Credits:\n");
	printf("	Written by Hypher.\n");
	printf("	Based on norz by geohot. Thanks to everyone in the iphone-elite team, and everyone in the iphone-dev team who has been putting so much effort into reversing the iphone. None of this could be done without all of you. I respect everyone on team. Everyone. Lets all be a team, shall we =)\n");
	printf("\n");
}

char* dump_fname = NULL;
unsigned int start = 0xA0000000;
unsigned int len = 0x400000;
unsigned int skip = 0;
unsigned int count = 0;
bool reset = false;
bool force = false;
bool verbose = false;

unsigned int cur_start = 0;
unsigned int cur_offset = 0;
unsigned int current_block = 0;
unsigned int block_bytes = 0;
FILE *block_file = NULL;

bool read_args(int argc, char *argv[])
{
	int i;
	int remain;
	char* arg;
	
	for (i=1; i<argc; i++)
	{
		arg = argv[i];
		remain = argc - i - 1;
		
		if (strcmp(arg, "--start") == 0 || strcmp(arg, "-s") == 0)
		{
			if (remain < 1)
			{
				printf("Please specify a start address.\n");
				return false;
			}
			sscanf(argv[++i], "%X", &start);
		}
			
		else if (strcmp(arg, "--skip") == 0 || strcmp(arg, "-k") == 0)
		{
			if (remain < 2)
			{
				printf("Please specify a skip value AND a count value.\n");
				return false;
			}
			
			sscanf(argv[++i], "%X", &skip);
			sscanf(argv[++i], "%d", &count);
		}
			
		else if (strcmp(arg, "--reset") == 0 || strcmp(arg, "-r") == 0)
			reset = true;

		else if (strcmp(arg, "--verbose") == 0 || strcmp(arg, "-v") == 0)
			verbose = true;
			
		else if (strcmp(arg, "--force") == 0 || strcmp(arg, "-f") == 0)
			force = true;
			
		else if (strcmp(arg, "--seczone") == 0)
		{
			start = 0x3fa000;
			len = 0x2000;
		}
			
		else if (strcmp(arg, "--length") == 0 || strcmp(arg, "-l") == 0)
		{
			if (remain < 1)
			{
				printf("Please specify a length.\n");
				return false;
			}
			sscanf(argv[++i], "%X", &len);
		}
		else if (strcmp(arg, "--help") == 0 || strcmp(arg, "-?") == 0 || strcmp(arg, "-h") == 0)
		{
			help();
			exit(0);
		}
		else
		{
			if (!dump_fname && arg[0] != '-')
			{
				dump_fname = arg;
			}
			else
			{
				printf("Unkown option: %s\n", arg);
				return false;
			}
		}
	}
	
	if (dump_fname == NULL)
	{
		printf("Please specify a dump filename\n");
		return false;
	}
	
	return true;
}

void open_dump_file()
{
	if (block_file)
	{
		if (verbose)
			printf("Closing current block file.\n");
			
		fclose(block_file);
	}
	
	current_block ++;
	char fname[1024];
	
	if (skip)
		sprintf(fname, "%s_%04d", dump_fname, current_block);
	else
		strcpy(fname, dump_fname);
		
	if (verbose)
		printf("Opening block file: \"%s\"\n", fname);
		
	block_file = fopen(fname, "wb");
	block_bytes = 0;
	
	if (!block_file)
	{
		printf("ERROR: Unable to open file: %s\n", fname);
		exit(-1);
	}
}

void write_dump_bytes(unsigned char * data, int size)
{
	if (size + block_bytes > len)
		size = len - block_bytes;
		
	fwrite(data, size, 1, block_file);
}

int main(int argc, char *argv[])
{
	if(! read_args(argc, argv))
	{
		printf("\n");
		usage();
		return -1;
	}
	
	if(start < (unsigned int)0xA0000000 && !force)
	{
		start += (unsigned int)0xA0000000;
		printf("Start address (0x%X) was below 0xA0000000, adding 0xA0000000. Use --force to disable.\n", start);
	}
	
	cur_offset = cur_start = start;
	
	printf("\n");
	
	printf("Base dumpfile name: %s\n", dump_fname);
	printf("Starting Address: 0x%X\n", start);
	printf("Dump Length: 0x%X\n", len);
	
	if (skip)
		printf("Skip: 0x%X\n", skip);
	else
		printf("Skipping Disabled\n");
		
	if (reset)
	{
		printf("Sorry -- reset not implemented yet =(\n\n");
		return -1;
	}
	
	// printf("Reset After Each? %s\n", reset ? "Yes" : "Nope");
	
	
	printf ("\n");
	//return 0;
	
	if (!skip)
		printf("Dumping: %X-%X\n", start, len);
	else
		printf("Dumping: %X-%X at %d intervals of %X%s\n", start, len, count, skip, reset ? ", resetting after each block." :"");
		
	resetbaseband();
	fp = InitConn(115200);
	
	open_dump_file();
	
	FILE *fd;
	fd=fopen(argv[1],"wb");
	
	data=(unsigned char *)malloc(70000);
	data[0]=0x60; data[1]=0x0D;
	printf("Waiting for data...\n");
	do
	{
		if(write(fp,data,2)==-1)
		{
			printf("Can't write\n");
			return -1;
		}
		getheader(1);
	} while(hlen==0||data[0]!=0xb);
	printf("Got Header: %d %2.2x %2.2x\n",hlen, data[0], data[1]);
	fclose(f); 
	
	for(u=0;u<6;u++)
	{
		if(u==0)
		{
			printf("Increasing baud rate...\n");
			mycmdpkt.cmd=0x82;
			mycmdpkt.data_size=4;
			data[0]=0x00; data[1]=0x10; data[2]=0x0E; data[3]=0x00;		//115200 bps
		}
		if(u==1)
		{
			tcgetattr(fp,&options);			//baud rate upped
			cfsetspeed(&options,921600);
			tcsetattr(fp,0,&options);
			mycmdpkt.cmd=0x801;
			mycmdpkt.data_size=0;
		}

		if(u==2)
		{
			mycmdpkt.cmd=0x84;
			mycmdpkt.data_size=2;
			data[0]=0; data[1]=0;
		}
		if(u==3)
		{
			mycmdpkt.cmd=0x85;
			mycmdpkt.data_size=0;
		}
		if(u==4)
		{
			mycmdpkt.cmd=0x802;
			mycmdpkt.data_size=4;
			memcpy(data, &cur_offset, 4);
			//data[0]=0x00; data[1]=0x00; data[2]=0x00; data[3]=0xA0;		//0x3FA000
			//data[0]=0x0; data[1]=0x0; data[2]=0x40; data[3]=0xf4;
		}
		if(u==5)
		{
			mycmdpkt.cmd=0x803;
			mycmdpkt.data_size=2;
			data[0]=0x0; data[1]=0x08;
			
		}
		cmd_write();
		
		getheader(0);
		if(u!=5 && verbose)
		{
			printbuffer();
		}
		
		if(u==5)
		{
			//TODO: Why +6?
			write_dump_bytes(data + 6, 0x800);
			
			if(cur_offset%0x1000==0 && verbose)
				printf("Wrote: 0x%x\n",start);
			
			cur_offset += 0x800;
			
			if(cur_offset < len+start)
				u=4;
			
			else if (skip && current_block < count)
			{
				cur_offset = cur_start = (cur_start + skip);
				
				open_dump_file();
				printf("Skipped 0x02%X to 0x08%X, now on part %d of %d\n", skip, cur_start, current_block, count);
				u=3;
			}
		}
	}
	
	fclose(block_file);
	printf("Dumped\n");
	return 0;
}


