
CC = gcc
# These where extacted from vzctl flags, you may need to run ./configure && make on vzctl's source and extract the flags for your environment
DEFS = -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_DLFCN_H=1 -DSBINDIR=\"/usr/sbin\" -DPKGCONFDIR=\"/etc/vz\" -DPKGLIBDIR=\"/usr/lib/vzctl\"
PKGLIBDIR = /usr/lib/vzctl
VERSION = 0.02

INCLUDES = -I ./include -I ./vzctl/include

CFLAGS = -Wall -fPIC

CFLAGS += -Wextra

all: src/remote_template.so scripts/vps-remote-create

install: src/remote_template.so scripts/vps-remote-create
	install src/remote_template.so $(PKGLIBDIR)/modules/remote_template.so
	install scripts/vps-remote-create $(PKGLIBDIR)/scripts/vps-remote-create

# Debian 4.0 installation locations
uninstall:
	rm $(PKGLIBDIR)/modules/remote_template.so

# Build a source tarball
dist: libvzctl-remote-template-$(VERSION).tar.gz

clean:
	rm src/remote_template.so src/remote_template.o scripts/vps-remote-create libvzctl-remote-template-$(VERSION).tar.gz

src/remote_template.so: src/remote_template.o
	$(CC) -shared $+ -Wl,-soname -ldl -o $@

src/remote_template.o: src/remote_template.c
	$(CC) $(DEFS) $(INCLUDES) $(CFLAGS) -c $+ -o $@

scripts/vps-remote-create: scripts/vps-remote-create.in
	sed -e 's!@'PKGLIBDIR'@!$(PKGLIBDIR)!g;' $< > $@

libvzctl-remote-template-$(VERSION).tar.gz: README COPYING Makefile include/remote_template.h src/remote_template.c scripts/vps-remote-create.in
	tar c -z --xform 's|^|libvzctl-remote-template-$(VERSION)/|' -f libvzctl-remote-template-$(VERSION).tar.gz README COPYING Makefile include/remote_template.h src/remote_template.c scripts/vps-remote-create.in
