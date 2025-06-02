# Customizable variables
O       ?= out
API     ?= 29

# Version and date
VERSION = $(shell git describe --tags --abbrev=0)
DATE    = $(shell date +"%Y-%m-%d")

SRCS    := main.c
CLFLAGS := -Wall -O2 -static -s
CLFLAGS += -DVERSION="\"$(VERSION)\"" -DDATE="\"$(DATE)\""

# Build target
all: aarch64 x86_64
aarch64: $(O)/kmi-arm64-v8a
x86_64: $(O)/kmi-x86_64

mkdir_out:
	@mkdir -p $(O)

$(O)/kmi-arm64-v8a: $(SRCS) | mkdir_out
	aarch64-linux-android$(API)-clang $(CLFLAGS) -DAARCH64 -o $@ $<

$(O)/kmi-x86_64: $(SRCS) | mkdir_out
	x86_64-linux-android$(API)-clang $(CLFLAGS) -DX86_64 -o $@ $<

# Clean target
clean:
	rm -rf $(O)/*

.PHONY: all clean aarch64 x86_64
