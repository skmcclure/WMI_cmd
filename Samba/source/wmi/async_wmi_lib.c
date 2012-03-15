/*
 * async_wmi_demo.c
 *
 ###########################################################################
 #
 # This program is part of Zenoss Core, an open source monitoring platform.
 # Copyright (C) 2008, Zenoss Inc.
 #
 # This program is free software; you can redistribute it and/or modify it
 # under the terms of the GNU General Public License version 2 as published by
 # the Free Software Foundation.
 #
 # For complete information please visit: http://www.zenoss.com/oss/
 #
 ###########################################################################
 *
 *  Created on: Aug 19, 2008
 *      Author: cgibbons
 */

#include "includes.h"

#include "libcli/composite/composite.h"
#include "librpc/rpc/dcerpc.h"
#include "librpc/gen_ndr/winreg.h"
#include "librpc/gen_ndr/ndr_winreg.h"
#include "librpc/gen_ndr/ndr_winreg_c.h"
#include "librpc/gen_ndr/oxidresolver.h"
#include "librpc/gen_ndr/ndr_oxidresolver.h"
#include "librpc/gen_ndr/ndr_oxidresolver_c.h"
#include "auth/credentials/credentials.h"

#include "zenoss_events.h"

#include "librpc/gen_ndr/ndr_dcom.h"
#include "librpc/gen_ndr/ndr_dcom_c.h"
#include "librpc/gen_ndr/ndr_remact_c.h"
#include "librpc/gen_ndr/ndr_epmapper_c.h"
#include "librpc/gen_ndr/com_dcom.h"
#include "librpc/rpc/dcerpc_table.h"

#include "lib/com/proto.h"
#include "lib/com/dcom/dcom.h"
#include "lib/com/dcom/proto.h"

#include "wmi/wmi.h"

struct WBEMCLASS;
struct WBEMOBJECT;

#include "wmi/proto.h"


/**
 * Initialize the Zenoss async event context. Will ensure that all
 * necessary Samba library initializtion takes place and that a root
 * event context for our local implementation is created.
 */
struct event_context* async_create_context(struct reactor_functions *funcs)
{
    // load all Samba parameters
    lp_load();

    // initialize the Samba DCERPC libraries
    dcerpc_init();
    dcerpc_table_init();
    dcom_proxy_IUnknown_init();
    dcom_proxy_IWbemLevel1Login_init();
    dcom_proxy_IWbemServices_init();
    dcom_proxy_IEnumWbemClassObject_init();
    dcom_proxy_IRemUnknown_init();
    dcom_proxy_IWbemFetchSmartEnum_init();
    dcom_proxy_IWbemWCOSmartEnum_init();

    // and finally create our top-level event context
    return zenoss_event_context_init(NULL, funcs);
}

#define WERR_CHECK(msg) if (!W_ERROR_IS_OK(result)) { \
			    DEBUG(0, ("ERROR: %s\n", msg)); \
			    goto error; \
			} else { \
			    DEBUG(1, ("OK   : %s\n", msg)); \
			}

WERROR ConnectAndQuery(struct com_context* ctx,
		       const char *hostname,
		       const char *query, 
		       struct IEnumWbemClassObject **pEnum)
{
	uint32_t cnt = 5, ret;
	WERROR result;
	NTSTATUS status;
	struct IWbemServices *pWS = NULL;

	result = WBEM_ConnectServer(ctx, 
				    hostname, 
				    "root\\cimv2", 
				    0, 0, 0, 0, 0, 0, 
				    &pWS);
	WERR_CHECK("Login to remote object.");
	    
	result = IWbemServices_ExecQuery(pWS, 
					 ctx, 
					 "WQL", 
					 query, 
					 WBEM_FLAG_RETURN_IMMEDIATELY | 
					 WBEM_FLAG_ENSURE_LOCATABLE, 
					 NULL, 
					 pEnum);
	WERR_CHECK("WMI query execute.");

	result = IEnumWbemClassObject_Reset(*pEnum, ctx);
	WERR_CHECK("Reset result of WMI query.");
error:
	return result;
}

/* This is a function */
struct composite_context * IWbemServices_ExecQuery_send_f(
    struct IWbemServices *interface, 
    TALLOC_CTX *mem_ctx, 
    BSTR strQueryLanguage, 
    BSTR strQuery, 
    int32_t lFlags, 
    struct IWbemContext *pCtx)
{
    /* This is a macro, and not directly callable from python */
    return IWbemServices_ExecQuery_send(interface, 
					mem_ctx, 
					strQueryLanguage,
					strQuery,
					lFlags,
					pCtx);
}


/* This is a function */
struct composite_context * IWbemServices_ExecNotificationQuery_send_f(
    struct IWbemServices *interface, 
    TALLOC_CTX *mem_ctx, 
    BSTR strQueryLanguage, 
    BSTR strQuery, 
    int32_t lFlags, 
    struct IWbemContext *pCtx)
{
    /* This is a macro, and not directly callable from python */
    return IWbemServices_ExecNotificationQuery_send(interface, 
						    mem_ctx, 
						    strQueryLanguage,
						    strQuery,
						    lFlags,
						    pCtx);
}


struct composite_context * IEnumWbemClassObject_Reset_send_f(
    struct IEnumWbemClassObject *interface, 
    TALLOC_CTX *mem_ctx)
{
    return IEnumWbemClassObject_Reset_send(interface, mem_ctx);
}

struct composite_context * IUnknown_Release_send_f(
    struct IUnknown *interface, 
    TALLOC_CTX *mem_ctx)
{
    return IUnknown_Release_send(interface, mem_ctx);
}

typedef void (*some_function)();
some_function hook_me_into_library[] = {
    (some_function)IEnumWbemClassObject_SmartNext,
    (some_function)dcerpc_winreg_OpenHKPD_send,
    (some_function)com_init_ctx,
};
