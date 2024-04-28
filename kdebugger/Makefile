# Define paths and variables
KSDK := ../ps4-ksdk/

LFILE = ./kdebugger.x

# Compiler and linker settings
CC      := gcc
AS      := gcc
OBJCOPY := objcopy
ODIR    := build
SDIR    := source
IDIRS   := -I$(KSDK)/include -I. -Iinclude
LDIRS   := -L$(KSDK) -L. -Llib
CFLAGS  := $(IDIRS) -O3 -std=gnu11 -fno-builtin -fno-exceptions -fno-asynchronous-unwind-tables -nostartfiles -nostdlib -Wall -masm=intel -march=btver2 -mtune=btver2 -m64 -mabi=sysv -mcmodel=small -fPIE
SFLAGS  := -nostartfiles -nostdlib -masm=intel -march=btver2 -mtune=btver2 -m64 -mabi=sysv -mcmodel=small
LFLAGS  := $(LDIRS) -Xlinker -T $(LFILE) -Wl,--build-id=none -pie
CFILES  := $(wildcard $(SDIR)/*.c)
SFILES  := $(wildcard $(SDIR)/*.s)
OBJS    := $(patsubst $(SDIR)/%.c, $(ODIR)/%.o, $(CFILES)) $(patsubst $(SDIR)/%.s, $(ODIR)/%.o, $(SFILES))

LIBS    := -lKSDK

TARGET  := ./kdebugger.elf

# Build target
$(TARGET): $(ODIR) $(OBJS)
	@echo "Linking $@"
	@$(CC) crt0.s $(ODIR)/*.o -o $(TARGET) $(CFLAGS) $(LFLAGS) $(LIBS)
	@echo "Converting to DYN format"
	@elfedit --output-type=DYN $(TARGET)
	@echo "Stripping symbols"
	@strip $(TARGET)

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