CC = arm-elf-gcc
CFLAGS = -O0 -Wall
LD = arm-elf-ld
OBJCOPY = arm-elf-objcopy
AS = arm-elf-as

INCLUDE = -I.

NAME = freeloader

OBJS =	init.o \
	arm9_utils.o \
	cfi_flash.o \
	pmb8876_misc.o \
	pmb8876_uart.o

$(NAME).bin:	$(NAME).elf
		$(OBJCOPY) -O binary $< $@

$(NAME).elf:	$(OBJS) freeloader.o
		$(LD) -Tfreeloader.x -o $@ $(OBJS)

all:	freeload.bin

.S.o:
	$(CC) $(CFLAGS) -c -o $@ $<
.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<

clean:
	rm -f $(OBJS) *~ freeload.bin freeload.elf freeloader.o
