KSDK    := ../ps4-ksdk/

CC      := gcc
AS      := gcc
OBJCOPY := objcopy
ODIR    := build
SDIR    := source
IDIRS   := -I$(KSDK)/include -I. -Iinclude
LDIRS   := -L$(KSDK) -L. -Llib
MAPFILE := $(shell basename $(CURDIR)).map
CFLAGS  := $(IDIRS) -Os -std=gnu11 -ffunction-sections -fdata-sections -fno-builtin -nostartfiles -nostdlib -Wall -masm=intel -march=btver2 -mtune=btver2 -m64 -mabi=sysv -mcmodel=small -fpie
SFLAGS  := -nostartfiles -nostdlib -march=btver2 -mtune=btver2
LFLAGS  := $(LDIRS) -Xlinker -T linker.x -Wl,--build-id=none -Wl,--gc-sections
CFILES  := $(wildcard $(SDIR)/*.c)
SFILES  := $(wildcard $(SDIR)/*.s)
OBJS    := $(patsubst $(SDIR)/%.c, $(ODIR)/%.o, $(CFILES)) $(patsubst $(SDIR)/%.s, $(ODIR)/%.o, $(SFILES))

LIBS:= -lKSDK

TARGET := ./installer.bin

# Build target
$(TARGET): $(ODIR) $(OBJS)
	@echo "Linking $@"
	@$(CC) crt0.s $(ODIR)/*.o -o temp.t $(CFLAGS) $(LFLAGS) $(LIBS)
	@echo "Converting to binary"
	@$(OBJCOPY) -O binary temp.t $(TARGET)
	@echo "Cleaning up temporary files"
	@rm -f temp.t

# Compile C source files
$(ODIR)/%.o: $(SDIR)/%.c
	@echo "Compiling C: $<"
	@$(CC) -c -o $@ $< $(CFLAGS)

# Compile assembly source files
$(ODIR)/%.o: $(SDIR)/%.s
	@echo "Compiling Assembly: $<"
	@$(AS) -c -o $@ $< $(SFLAGS)

# Create build directory if it doesn't exist
$(ODIR):
	@echo "Creating build directory"
	@mkdir $@

# Clean rule
.PHONY: clean

clean:
	@echo "Cleaning..."
	@rm -f $(TARGET) $(ODIR)/*.o
