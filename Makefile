# ==========================================================================
# OS32 Blockfall — standalone Makefile
# ==========================================================================

OS32_SDK  ?= ../os32/build/sdk

_AUTO_CROSS := $(patsubst %/bin/i386-elf-gcc,%,$(shell command -v i386-elf-gcc 2>/dev/null))
CROSS_DIR  ?= $(if $(_AUTO_CROSS),$(_AUTO_CROSS),/usr/local/cross)

SDK_STAMP := $(wildcard $(OS32_SDK)/KAPI_VERSION)
ifeq ($(SDK_STAMP),)
  ifneq ($(filter-out clean,$(or $(MAKECMDGOALS),all)),)
    $(error OS32_SDK=$(OS32_SDK) に SDK が見つかりません。OS32 側で make sdk を実行してください)
  endif
  KAPI_VERSION = 0
else
  KAPI_VERSION := $(shell cat $(OS32_SDK)/KAPI_VERSION)
endif

CC      = i386-elf-gcc
LD      = i386-elf-ld
OBJCOPY = i386-elf-objcopy

GCC_LIBDIR := $(patsubst %/,%,$(dir $(shell $(CC) -print-libgcc-file-name 2>/dev/null)))

CFLAGS = -std=gnu89 -m32 -march=i386 -ffreestanding -fno-pie -fno-stack-protector \
         -nostdlib -mno-red-zone -fcommon -O2 -Wall -MMD -MP -D__OS32_USERLAND__ \
         -I$(OS32_SDK)/include -I$(OS32_SDK)/include/os32 \
         -I$(CROSS_DIR)/i386-elf/include

LDFLAGS = -m elf_i386 -T $(OS32_SDK)/link/app.ld -nostdlib --nmagic --gc-sections \
          -L$(OS32_SDK)/lib -L$(CROSS_DIR)/i386-elf/lib \
          -L$(GCC_LIBDIR)

SDK_CRT = $(OS32_SDK)/crt/crt0.o $(OS32_SDK)/crt/crt0_c.o \
          $(OS32_SDK)/crt/syscalls.o $(OS32_SDK)/crt/help.o

LIBS = -los32gfx -los32math

SRC := $(wildcard src/*.c)
OBJ := $(patsubst src/%.c,build/%.o,$(SRC))
DEP := $(OBJ:.o=.d)

all: build/blockfall.bin
	@echo "=== OS32 Blockfall (SDK KAPI v$(KAPI_VERSION)) ==="

build:
	mkdir -p build

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

build/blockfall.elf: $(OBJ) | build
	$(LD) $(LDFLAGS) -o $@ $(SDK_CRT) $(OBJ) \
	       --start-group $(LIBS) --end-group -lc -lgcc

build/blockfall.raw: build/blockfall.elf
	$(OBJCOPY) -O binary $< $@

build/blockfall.bin: build/blockfall.raw build/blockfall.elf app.conf
	@_api=$$(awk '$$1 == "build/blockfall" { print $$2 }' app.conf); \
	_heap=$$(awk '$$1 == "build/blockfall" { print $$3 }' app.conf); \
	_api=$${_api:-$(KAPI_VERSION)}; _heap=$${_heap:-0}; \
	if [ "$$_heap" != "0" ]; then \
		python3 $(OS32_SDK)/bin/mkos32x.py $< $@ --elf build/blockfall.elf --api $$_api --heap $$_heap; \
	else \
		python3 $(OS32_SDK)/bin/mkos32x.py $< $@ --elf build/blockfall.elf --api $$_api; \
	fi

clean:
	rm -rf build

.PHONY: all clean
-include $(DEP)
