
CC=gcc
COL=/usr/bin/col
MAKEDEPEND=/usr/openwin/bin/makedepend
NEQN=/usr/bin/neqn
NROFF=/usr/bin/nroff
RM=/usr/bin/rm
TBL=/usr/bin/tbl

CFLAGS=-Wall -pedantic

EXE=ckwtmpx
SRC=ckwtmpx.c

.PHONY: all
all: $(EXE) README

$(EXE): $(SRC) Makefile
	$(CC) -o $(EXE) $(CFLAGS) $(SRC)

README: ckwtmpx.1
	$(TBL) ckwtmpx.1 | $(NEQN) /usr/share/lib/pub/eqnchar - | $(NROFF) -u0 -Tlp -man - | $(COL) -b -x > README

.PHONY: clean
clean:
	$(RM) -f core $(EXE) README

.PHONY: depend
depend:
	$(MAKEDEPEND) $(SRC)
# DO NOT DELETE

ckwtmpx.o: /usr/include/stdio.h /usr/include/sys/feature_tests.h
ckwtmpx.o: /usr/include/sys/ccompile.h /usr/include/sys/isa_defs.h
ckwtmpx.o: /usr/include/iso/stdio_iso.h /usr/include/sys/va_list.h
ckwtmpx.o: /usr/include/stdio_tag.h /usr/include/stdio_impl.h
ckwtmpx.o: /usr/include/iso/stdio_c99.h /usr/include/stdlib.h
ckwtmpx.o: /usr/include/iso/stdlib_iso.h /usr/include/iso/stdlib_c99.h
ckwtmpx.o: /usr/include/unistd.h /usr/include/sys/types.h
ckwtmpx.o: /usr/include/sys/machtypes.h /usr/include/sys/int_types.h
ckwtmpx.o: /usr/include/sys/select.h /usr/include/sys/time_impl.h
ckwtmpx.o: /usr/include/sys/time.h /usr/include/sys/time.h
ckwtmpx.o: /usr/include/sys/unistd.h /usr/include/errno.h
ckwtmpx.o: /usr/include/sys/errno.h /usr/include/utmpx.h /usr/include/utmp.h
ckwtmpx.o: /usr/include/sys/types32.h /usr/include/inttypes.h
ckwtmpx.o: /usr/include/sys/inttypes.h /usr/include/sys/int_limits.h
ckwtmpx.o: /usr/include/sys/int_const.h /usr/include/sys/int_fmtio.h
ckwtmpx.o: /usr/include/sys/stdint.h /usr/include/string.h
ckwtmpx.o: /usr/include/iso/string_iso.h /usr/include/sys/ddi.h
ckwtmpx.o: /usr/include/sys/map.h /usr/include/sys/t_lock.h
ckwtmpx.o: /usr/include/sys/machlock.h /usr/include/sys/param.h
ckwtmpx.o: /usr/include/sys/mutex.h /usr/include/sys/rwlock.h
ckwtmpx.o: /usr/include/sys/semaphore.h /usr/include/sys/condvar.h
ckwtmpx.o: /usr/include/sys/buf.h /usr/include/sys/kstat.h
ckwtmpx.o: /usr/include/sys/uio.h /usr/include/sys/stream.h
ckwtmpx.o: /usr/include/sys/vnode.h /usr/include/sys/rwstlock.h
ckwtmpx.o: /usr/include/sys/ksynch.h /usr/include/sys/cred.h
ckwtmpx.o: /usr/include/sys/resource.h /usr/include/vm/seg_enum.h
ckwtmpx.o: /usr/include/sys/kmem.h /usr/include/sys/vmem.h
ckwtmpx.o: /usr/include/sys/poll.h /usr/include/sys/strmdep.h
ckwtmpx.o: /usr/include/sys/model.h /usr/include/sys/strft.h
ckwtmpx.o: /usr/include/stdarg.h /usr/include/iso/stdarg_iso.h
ckwtmpx.o: /usr/include/sys/va_impl.h /usr/include/iso/stdarg_c99.h
ckwtmpx.o: /usr/include/fcntl.h /usr/include/sys/fcntl.h
ckwtmpx.o: /usr/include/sys/stat.h /usr/include/sys/stat_impl.h ckwtmpx.h
