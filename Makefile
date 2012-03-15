# This is a simple Makefile that replaces the original GNUMakefile that 
# came with the original distribution

SRCDIR=Samba/source
PROGS=$(SRCDIR)/bin/wmic \
	$(SRCDIR)/bin/wmi_method \
	$(SRCDIR)/bin/wmi_rexec \
	$(SRCDIR)/bin/wmi_pushscript

SOURCE=$(SRCDIR)/wmi/wmic.c wmi_method.c wmi_rexec.c wmi_pushscript.c

all: $(PROGS)

$(SRCDIR)/wmi/proto.h:
	cd $(SRCDIR); ./autogen.sh; ./configure 
	$(MAKE) -C $(SRCDIR) proto

$(PROGS): $(SRCDIR)/wmi/proto.h $(SOURCE)
	cp config.mk $(SRCDIR)/wmi
	cp *.c $(SRCDIR)/wmi
	$(MAKE) -C $(SRCDIR) bin/wmic bin/wmi_method bin/wmi_rexec bin/wmi_pushscript

distclean:
	$(MAKE) -C $(SRCDIR) distclean

clean:
	rm -f $(SRCDIR)/wmi/*.o
	rm -f $(PROGS)

