SRC_PATH=/home/czx/qemu-workspace/qemu-spy/plugin_spy
TOP_SRC_PATH = $(SRC_PATH)/..
PKG_CONFIG=pkg-config
CC=cc -m64 -mcx16
CFLAGS=-O0 -g

CFLAGS += $(shell $(PKG_CONFIG) --cflags glib-2.0)
CFLAGS += -fPIC -Wall
CFLAGS += -I$(TOP_SRC_PATH)/include
CFLAGS += -I$(TOP_SRC_PATH)/include/qemu
CFLAGS += -I$(TOP_SRC_PATH)/build
CFLAGS += -I$(TOP_SRC_PATH)/host/include/generic

all: $(SONAMES)
.SUFFIXES:

test_targets := hello-syscall hello-pipe hello-seg hello-server
$(test_targets): %: test/%.c
	@arm-linux-gnueabi-gcc -static -o build/$@ $<
	@echo "${<}-->build/${@}"

test_targets_cpp := hello-crow hello-agent
$(test_targets_cpp): %: test/%.cpp
	@arm-linux-gnueabi-g++ -static -o build/$@ $<
	@echo "${<}-->build/${@}"
test: $(test_targets)

%.o: %.c
	@$(CC) $(CFLAGS) $(PLUGIN_CFLAGS) -c -o build/$@ $<
	@echo "${<}-->${@}"

%: %.o
	@$(CC) $(CFLAGS) -shared -o build/lib$@.so build/$^
	@echo "${<}-->lib${@}.so"

run: aflspy
	cd test/ && bash start-qemu-user.sh && cd ..

runsys: aflspy
	cd test/ && bash start-qemu.sh && cd ..

clean:
	rm -f build/*

.PHONY: all clean test $(test_targets)
.SECONDARY:
