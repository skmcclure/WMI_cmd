#######################
# Start LIBRARY async_wmi_lib
[LIBRARY::async_wmi_lib]
VERSION=0.0.1
SO_VERSION=0
PUBLIC_DEPENDENCIES = LIBCLI_SMB NDR_MISC LIBSAMBA-UTIL LIBSAMBA-CONFIG RPC_NDR_SAMR RPC_NDR_LSA DYNCONFIG \
		NDR_DCOM \
		dcerpc \
		dcom \
		wmi \
		RPC_NDR_REMACT \
		NDR_TABLE 
OBJ_FILES = async_wmi_lib.o zenoss_events.o
# End LIBRARY async_wmi_lib
#######################

#################################
# Start BINARY wmic
[BINARY::wmic]
PRIVATE_PROTO_HEADER = proto.h
INSTALLDIR = BINDIR
OBJ_FILES = wmic.o
PRIVATE_DEPENDENCIES = \
                POPT_SAMBA \
                POPT_CREDENTIALS \
                LIBPOPT \
		RPC_NDR_OXIDRESOLVER \
		NDR_DCOM \
		RPC_NDR_REMACT \
		NDR_TABLE \
		DCOM_PROXY_DCOM \
		dcom \
		wmi
# End BINARY wmic
#################################

#################################
# Start BINARY wmi_method
[BINARY::wmi_method]
INSTALLDIR = BINDIR
OBJ_FILES = wmi_method.o
PRIVATE_DEPENDENCIES = \
                POPT_SAMBA \
                POPT_CREDENTIALS \
                LIBPOPT \
		RPC_NDR_OXIDRESOLVER \
		NDR_DCOM \
		RPC_NDR_REMACT \
		NDR_TABLE \
		DCOM_PROXY_DCOM \
		dcom \
		wmi
# End BINARY wmic
#################################

#################################
# Start BINARY wmi_rexec
[BINARY::wmi_rexec]
INSTALLDIR = BINDIR
OBJ_FILES = wmi_rexec.o
PRIVATE_DEPENDENCIES = \
                POPT_SAMBA \
                POPT_CREDENTIALS \
                LIBPOPT \
		RPC_NDR_OXIDRESOLVER \
		NDR_DCOM \
		RPC_NDR_REMACT \
		NDR_TABLE \
		DCOM_PROXY_DCOM \
		dcom \
		wmi
# End BINARY wmi_rexec
#################################

#################################
# Start BINARY wmi_pushscript
[BINARY::wmi_pushscript]
INSTALLDIR = BINDIR
OBJ_FILES = wmi_pushscript.o
PRIVATE_DEPENDENCIES = \
                POPT_SAMBA \
                POPT_CREDENTIALS \
                LIBPOPT \
		RPC_NDR_OXIDRESOLVER \
		NDR_DCOM \
		RPC_NDR_REMACT \
		NDR_TABLE \
		DCOM_PROXY_DCOM \
		dcom \
		wmi
# End BINARY wmi_pushscript
#################################

#################################
# Start BINARY wmis
[BINARY::wmis]
INSTALLDIR = BINDIR
OBJ_FILES = wmis.o
PRIVATE_DEPENDENCIES = \
                POPT_SAMBA \
                POPT_CREDENTIALS \
                LIBPOPT \
		RPC_NDR_OXIDRESOLVER \
		NDR_DCOM \
		RPC_NDR_REMACT \
		NDR_TABLE \
		DCOM_PROXY_DCOM \
		dcom \
		wmi
# End BINARY wmis
#################################

librpc/gen_ndr/dcom_p.c: idl

#################################
# Start BINARY pdhc
#[BINARY::pdhc]
#INSTALLDIR = BINDIR
#OBJ_FILES = \
#                pdhc.o
#PRIVATE_DEPENDENCIES = \
#                POPT_SAMBA \
#                POPT_CREDENTIALS \
#                LIBPOPT \
#		NDR_TABLE \
#		RPC_NDR_WINREG
# End BINARY pdhc
#################################

#################################
# Start LIBRARY wmi
[LIBRARY::wmi]
VERSION=0.0.1
SO_VERSION=0
OBJ_FILES = wbemdata.o wmicore.o
PUBLIC_DEPENDENCIES = LIBCLI_SMB NDR_MISC LIBSAMBA-UTIL LIBSAMBA-CONFIG \
		RPC_NDR_SAMR RPC_NDR_LSA DYNCONFIG \
		RPC_NDR_OXIDRESOLVER \
		NDR_DCOM \
		RPC_NDR_REMACT \
		NDR_TABLE \
		DCOM_PROXY_DCOM \
		RPC_NDR_WINREG \
		dcom
# End LIBRARY wmi
#################################
