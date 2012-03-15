#################################
# Start BINARY winexe
[BINARY::winexe]
INSTALLDIR = BINDIR
OBJ_FILES = \
                winexe.o \
		service.o \
		async.o \
		winexesvc/winexesvc_exe.o
PRIVATE_DEPENDENCIES = \
                POPT_SAMBA \
                POPT_CREDENTIALS \
                LIBPOPT \
		RPC_NDR_SVCCTL
# End BINARY winexe
#################################

winexe/winexesvc/winexesvc_exe.c: winexe/winexesvc/winexesvc.c
	@$(MAKE) -C winexe/winexesvc
