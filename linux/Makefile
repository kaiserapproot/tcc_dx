# Linux (WSL) build of tpp: x86_64 ELF target, built with gcc.
#
#   make -C linux          -> linux/out/tcc, linux/out/include, linux/out/libtcc1.a
#   make -C linux test     -> smoke tests (C / C++ / -run)
#   make -C linux clean
#
# The Windows build (tcc.vcxproj / build.bat) is unaffected; this Makefile
# passes TCC_TARGET_X86_64 explicitly so config.h does not select PE.

TOP     := $(abspath ..)
OUT     := $(abspath out)
CC      ?= gcc
CFLAGS  ?= -O1 -g
WARN    := -Wall -Wno-unused -Wno-unused-result -Wno-address \
           -Wno-pointer-sign -Wno-format
DEFS    := -DTCC_TARGET_X86_64 -DONE_SOURCE=0 \
           -DCONFIG_TRIPLET=\"x86_64-linux-gnu\" \
           -DCONFIG_TCCDIR=\"$(OUT)\" \
           -DCONFIG_TCC_BCHECK=0 \
           -DTCC_LIBTCC1=\"libtcc1.a\"
LIBS    := -lm -ldl -lpthread

# Sources saved as CP932 (see CLAUDE.md).  gcc would read a 0x5C trail
# byte as a backslash escape, and tcc.h pulls in the UTF-8 x86_64-gen.c,
# so -finput-charset cannot be used per file.  Instead compile a UTF-8
# copy made by iconv under out/src (the originals are never rewritten).
CP932_SRCS := tccgen.c tccpp.c tccasm.c libtcc.c

SRCS := tcc.c libtcc.c tccpp.c tccgen.c tccdbg.c tccelf.c tccasm.c \
        tccrun.c x86_64-gen.c x86_64-link.c i386-asm.c
OBJS := $(patsubst %.c,$(OUT)/obj/%.o,$(SRCS))

HDRS := $(wildcard $(TOP)/*.h)

TCC := $(OUT)/tcc
HDR_STAMP := $(OUT)/include/tccdefs.h

all: $(TCC) $(OUT)/libtcc1.a $(HDR_STAMP)

$(OUT)/obj/%.o: $(TOP)/%.c $(HDRS) Makefile | $(OUT)/obj
	$(CC) $(CFLAGS) $(WARN) $(DEFS) -I$(TOP) -c $< -o $@

$(OUT)/src/%.c: $(TOP)/%.c
	mkdir -p $(dir $@)
	iconv -f CP932 -t UTF-8 $< > $@.tmp && mv $@.tmp $@

$(patsubst %.c,$(OUT)/obj/%.o,$(CP932_SRCS)): $(OUT)/obj/%.o: $(OUT)/src/%.c $(HDRS) Makefile | $(OUT)/obj
	$(CC) $(CFLAGS) $(WARN) $(DEFS) -I$(TOP) -c $< -o $@

$(TCC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

$(OUT)/obj:
	mkdir -p $@

# Runtime headers: {B}/include is the first system include path.
# linux/include holds Linux-only wrapper headers (sys/cdefs.h).
$(HDR_STAMP): $(wildcard $(TOP)/include/*.h) $(wildcard include/*/*.h)
	mkdir -p $(OUT)/include
	cp -f $(TOP)/include/*.h $(OUT)/include/
	cp -rf include/. $(OUT)/include/

# libtcc1.a is built by the freshly built tcc itself (as upstream does).
LIB_C := libtcc1.c va_list.c
LIB_S := alloca86_64.S alloca86_64-bt.S
LIB_OBJS := $(patsubst %.c,$(OUT)/obj/lib/%.o,$(LIB_C)) \
            $(patsubst %.S,$(OUT)/obj/lib/%.o,$(LIB_S)) \
            $(OUT)/obj/lib/dsohandle.o

# Linux-only runtime pieces live in linux/lib.
$(OUT)/obj/lib/dsohandle.o: lib/dsohandle.c $(TCC) $(HDR_STAMP)
	mkdir -p $(dir $@)
	$(TCC) -B$(OUT) -c $< -o $@

$(OUT)/obj/lib/%.o: $(TOP)/lib/%.c $(TCC) $(HDR_STAMP)
	mkdir -p $(dir $@)
	$(TCC) -B$(OUT) -c $< -o $@

$(OUT)/obj/lib/%.o: $(TOP)/lib/%.S $(TCC) $(HDR_STAMP)
	mkdir -p $(dir $@)
	$(TCC) -B$(OUT) -c $< -o $@

$(OUT)/libtcc1.a: $(LIB_OBJS)
	$(TCC) -ar rcs $@ $(LIB_OBJS)

test: all
	sh ./run_tests.sh

clean:
	rm -rf $(OUT)

.PHONY: all test clean
