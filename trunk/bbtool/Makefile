SOURCES=\
	baseband.c \
	main.c
# set this to upload the built bbtool to /usr/local/bin on your iPhone for testing
IPHONE_IP=

CC=/usr/local/iphonedev/bin/arm-apple-darwin-gcc
CFLAGS=-isysroot /Developer/SDKs/iPhone.sdk
ifeq ($(CONFIGURATION),Debug)
	CFLAGS+=-g -O0 -Wall -DDEBUG
else
	CFLAGS+=-O7 -Wall -Werror
endif
LD=$(CC)
LDFLAGS=-isysroot /Developer/SDKs/iPhone.sdk -mmacosx-version-min=10.1
LIBS=-framework IOKit

EXECUTABLE_NAME=$(PRODUCT_NAME)
SOURCES_ABS=$(addprefix $(SRCROOT)/,$(SOURCES))
OBJECTS=\
	$(patsubst %.c,%.o,$(filter %.c,$(SOURCES)))
OBJECTS_ABS=$(addprefix $(CONFIGURATION_TEMP_DIR)/,$(OBJECTS))
PRODUCT_ABS=$(CONFIGURATION_TEMP_DIR)/bbtool

ifdef IPHONE_IP
all: install
else
all: $(PRODUCT_ABS)
endif

install: $(PRODUCT_ABS)
	scp -r $(PRODUCT_ABS) root@$(IPHONE_IP):/usr/local/bin/

$(PRODUCT_ABS): $(APP_ABS) $(OBJECTS_ABS)
	$(LD) $(LDFLAGS) $(LIBS) -o $(PRODUCT_ABS) $(OBJECTS_ABS)

$(CONFIGURATION_TEMP_DIR):
	mkdir -p $(CONFIGURATION_TEMP_DIR)

$(CONFIGURATION_TEMP_DIR)/%.o: $(SRCROOT)/%.m $(CONFIGURATION_TEMP_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(CONFIGURATION_TEMP_DIR)/%.o: $(SRCROOT)/%.c $(CONFIGURATION_TEMP_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS_ABS)
	rm -f $(PRODUCT_ABS)

