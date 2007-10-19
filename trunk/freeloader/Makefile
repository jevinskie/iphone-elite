CC = arm-elf-gcc
LD = arm-elf-ld
OBJCOPY = arm-elf-objcopy
AS = arm-elf-as

INCLUDE = -I.
CFLAGS = -O0

ENTRY = 0x0

OBJS =	arm9_utils.o \
	cfi_flash.o

shitload.bin:	shitload.elf
		$(OBJCOPY) -O binary $< $@

shitload.elf:	$(OBJS)
		$(LD) -e $(ENTRY) -o $@ $(OBJS)

all:	shitload.bin

.S.o:
	$(AS) -o $@ $<
.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<

clean:
	rm -f $(OBJS) *~ shitload.bin shitload.elf
