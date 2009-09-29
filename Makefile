
CC = gcc
# These where extacted from vzctl flags, you may need to run ./configure && make on vzctl's source and extract the flags for your environment
DEFS = -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_DLFCN_H=1 -DSBINDIR=\"/usr/sbin\" -DPKGCONFDIR=\"/etc/vz\" -DPKGLIBDIR=\"/usr/lib/vzctl\" -DDEBUG=1

# Assume vzctl's source is extracted in to the same parent directory as libvzctl-remote-template was extracted
INCLUDES = -I ./include -I ../vzctl/include

CFLAGS = -Wall -Wextra -fPIC

all: remote_template.so

install: remote_template.so
	install remote_template.so /usr/lib/vzctl/modules/remote_template.so

# Debian 4.0 installation locations
uninstall:
	rm /usr/lib/vzctl/modules/remote_template.so

clean:
	rm remote_template.so remote_template.o

remote_template.so: remote_template.o
	$(CC) -shared remote_template.o -Wl,-soname -ldl -o remote_template.so

remote_template.o: src/remote_template.c
	$(CC) $(DEFS) $(INCLUDES) $(CFLAGS) -c src/remote_template.c

