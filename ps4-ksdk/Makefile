# Compiler and linker settings
CC	:= gcc
AR	:= ar
ODIR := build
SDIR := source
IDIRS := -I. -Iinclude
LDIRS := -L. -Llib
CFLAGS := $(IDIRS) -Os -std=gnu11 -ffunction-sections -fdata-sections -fno-builtin -nostartfiles -nostdlib -Wall -masm=intel -march=btver2 -mtune=btver2 -m64 -mabi=sysv -mcmodel=small -fpie
LFLAGS := $(LDIRS)
CFILES := $(wildcard $(SDIR)/*.c)
SFILES := $(wildcard $(SDIR)/*.s)
OBJS := $(patsubst $(SDIR)/%.c, build/%.o, $(CFILES)) $(patsubst $(SDIR)/%.s, build/%.o, $(SFILES))

TARGET = libKSDK.a

# Build target
$(TARGET): $(ODIR) $(OBJS)
	@echo "Creating library: $@"
	$(AR) rcs $@ $(OBJS)

# Compile C source files
$(ODIR)/%.o: $(SDIR)/%.c
	@echo "Compiling C: $<"
	$(CC) -c -o $@ $< $(CFLAGS) $(LFLAGS)

# Compile assembly source files
$(ODIR)/%.o: $(SDIR)/%.s
	@echo "Compiling Assembly: $<"
	$(CC) -c -o $@ $< $(CFLAGS) $(LFLAGS)

# Create build directory if it doesn't exist
$(ODIR):
	@echo "Creating build directory: $@"
	@mkdir $@

# Clean rule
.PHONY: clean
clean:
	@echo "Cleaning..."
	@rm -rf $(TARGET) $(ODIR)
