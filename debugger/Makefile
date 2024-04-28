LIBPS4  := ../ps4-payload-sdk

TEXT    := 0x926200000
DATA    := 0x926300000

CC      := gcc
AS      := gcc
OBJCOPY := objcopy
ODIR    := build
SDIR    := source
IDIRS   := -I$(LIBPS4)/include -I. -Iinclude
LDIRS   := -L$(LIBPS4) -L. -Llib
CFLAGS  := $(IDIRS) -O2 -std=c11 -fno-builtin -nostartfiles -nostdlib -Wall -masm=intel -march=btver2 -mtune=btver2 -m64 -mabi=sysv -mcmodel=large -DTEXT_ADDRESS=$(TEXT) -DDATA_ADDRESS=$(DATA)
SFLAGS  := -nostartfiles -nostdlib -march=btver2 -mtune=btver2
LFLAGS  := $(LDIRS) -Xlinker -T $(LIBPS4)/linker.x -Wl,--build-id=none -Ttext=$(TEXT) -Tdata=$(DATA)
CFILES  := $(wildcard $(SDIR)/*.c)
SFILES  := $(wildcard $(SDIR)/*.s)
OBJS    := $(patsubst $(SDIR)/%.c, $(ODIR)/%.o, $(CFILES)) $(patsubst $(SDIR)/%.s, $(ODIR)/%.o, $(SFILES))
LIBS    := -lPS4

TARGET  := ./debugger.bin

# Build target
$(TARGET): $(ODIR) $(OBJS)
	@echo "Linking $@"
	@$(CC) $(LIBPS4)/crt0.s $(ODIR)/*.o -o temp.t $(CFLAGS) $(LFLAGS) $(LIBS)
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
