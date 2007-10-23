CC = arm-elf-gcc
CFLAGS = -O0 -Wall
LD = arm-elf-ld
OBJCOPY = arm-elf-objcopy
AS = arm-elf-as

INCLUDE = -I.

NAME = virgintool

OBJS =	arm9_utils.o \
	misc.o \
	sha1.o \
	tea.o \

$(NAME).bin:	$(NAME).elf
		$(OBJCOPY) -O binary $< $@
		dd if=header.bin bs=4128 count=1 of=eliteloader.bin 2>/dev/null
		cat virgintool.bin >> eliteloader.bin

$(NAME).elf:	$(OBJS) $(NAME).o
		$(LD) -T$(NAME).x -o $@ $(OBJS)

all:	$(NAME).bin

.S.o:
	$(CC) $(CFLAGS) -c -o $@ $<
.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<

clean:
	rm -f $(OBJS) *~ $(NAME).bin $(NAME).elf $(NAME).o eliteloader.bin
