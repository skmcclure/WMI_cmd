/*
   DCOM torture tester
   Copyright (C) Zenoss, Inc. 2008

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "includes.h"
#include "auth/credentials/credentials.h"
#include "lib/cmdline/popt_common.h"
#include "librpc/rpc/dcerpc.h"
#include "torture/rpc/rpc.h"
#include "torture/torture.h"
#include "librpc/rpc/dcerpc_table.h"
#include "lib/util/dlinklist.h"
#include "libcli/composite/composite.h"
#include "lib/com/com.h"
#include "librpc/gen_ndr/com_dcom.h"
#include "lib/com/dcom/dcom.h"
#include "wmi/wmi.h"

/*
 * Test activating the IWbemLevel1Login interface synchronously.
 */
static BOOL torture_wbem_login(struct torture_context *torture)
{
    BOOL ret = True;
    TALLOC_CTX *mem_ctx = talloc_init("torture_wbem_login");
    struct com_context *com_ctx = NULL;
    const char *binding = NULL;
    struct IUnknown **mqi = NULL;
    struct GUID clsid;
    struct GUID iid;
    NTSTATUS status;

    /*
     * Initialize our COM and DCOM contexts.
     */
    com_init_ctx(&com_ctx, NULL);
    dcom_client_init(com_ctx, cmdline_credentials);

    /*
     * Pull our needed test arguments from the torture parameters subsystem.
     */
    binding = torture_setting_string(torture, "binding", NULL);

    /*
     * Create the parameters needed for the activation call: we need the CLSID
     * and IID for the specific interface we're after.
     */
    GUID_from_string(CLSID_WBEMLEVEL1LOGIN, &clsid);
    GUID_from_string(COM_IWBEMLEVEL1LOGIN_UUID, &iid);

    /*
     * Activate the interface using the DCOM synchronous method call.
     */
    status = dcom_activate(com_ctx, mem_ctx, binding, &clsid, &iid, 1, &mqi);
    ret = NT_STATUS_IS_OK(status);
    if (ret)
    {
        /*
         * Clean up by releasing the IUnknown interface on the remote server
         * and also by releasing our allocated interface pointer.
         */
        IUnknown_Release(mqi[0], mem_ctx);
        talloc_free(mqi);
    }

    talloc_report_full(mem_ctx, stdout);

    return ret;
}

/*
 * Test activating the IWbemLevel1Login interface asynchronously.
 */
static void torture_wbem_login_async_cont(struct composite_context *ctx)
{
    struct composite_context *c = NULL;
    BOOL *pRet = NULL;
    struct IUnknown **mqi = NULL;
    NTSTATUS status;

    /* retrieve the parent composite context */
    c = talloc_get_type(ctx->async.private_data, struct composite_context);
    if (!composite_is_ok(c)) return;

    pRet = (BOOL *)c->private_data;

    status = dcom_activate_recv(ctx, c, &mqi);
    *pRet = NT_STATUS_IS_OK(status);

    talloc_report_full(c, stdout);

    if (*pRet)
    {
        /*
         * Clean up by releasing the IUnknown interface on the remote server
         * and also by releasing our allocated interface pointer.
         */
        IUnknown_Release(mqi[0], c); /* really synchronous but eh */
        talloc_free(mqi);
    }

    composite_done(c);
}

static BOOL torture_wbem_login_async(struct torture_context *torture)
{
    BOOL ret = True;
    TALLOC_CTX *mem_ctx = talloc_init("torture_wbem_login_async");
    struct com_context *com_ctx = NULL;
    const char *binding = NULL;
    struct composite_context *c = NULL;
    struct composite_context *new_ctx = NULL;
    struct GUID clsid;
    struct GUID iid;

    /*
     * Initialize our COM and DCOM contexts.
     */
    com_init_ctx(&com_ctx, NULL);
    dcom_client_init(com_ctx, cmdline_credentials);

    /*
     * Pull our needed test arguments from the torture parameters subsystem.
     */
    binding = torture_setting_string(torture, "binding", NULL);

    /*
     * Create a new composite for our call sequence, with the private data being
     * our return flag.
     */
    c = composite_create(mem_ctx, com_ctx->event_ctx);
    c->private_data = &ret;

    /*
     * Create the parameters needed for the activation call: we need the CLSID
     * and IID for the specific interface we're after.
     */
    GUID_from_string(CLSID_WBEMLEVEL1LOGIN, &clsid);
    GUID_from_string(COM_IWBEMLEVEL1LOGIN_UUID, &iid);

    /*
     * Fire off the asynchronous activation request with all the needed
     * input parameters. Then wait for the composite to be done within the
     * context of this function, which allows all the asynchronous magic to
     * still happen.
     */
    new_ctx = dcom_activate_send(c, &clsid, binding, 1, &iid, com_ctx);
    composite_continue(c, new_ctx, torture_wbem_login_async_cont, c);
    composite_wait(new_ctx);
    talloc_free(c);
    talloc_report_full(mem_ctx, stdout);

    return ret;
}

struct wbem_exec_query_state
{
    NTSTATUS status;
    struct IWbemServices *pServices;
    struct IEnumWbemClassObject *pEnum;
};

static void wbem_enum_release_continue(struct composite_context *ctx)
{
    struct composite_context *c = NULL;

    /* retrieve the parent composite context */
    c = talloc_get_type(ctx->async.private_data, struct composite_context);

    (void)IUnknown_Release_recv(ctx);

    composite_done(c);
}

static void wbem_enum_next_continue(struct composite_context *ctx)
{
    struct composite_context *c = NULL;
    struct composite_context *new_ctx = NULL;
    struct wbem_exec_query_state *s = NULL;
    struct WbemClassObject **apObjects = NULL;
    uint32_t uReturned = 0;
    WERROR result;

    /* retrieve the parent composite context */
    c = talloc_get_type(ctx->async.private_data, struct composite_context);

    s = talloc_get_type(c->private_data, struct wbem_exec_query_state);

    apObjects = talloc_array(c, struct WbemClassObject *, 7);
    if (composite_nomem(apObjects, c)) return;

    result = IEnumWbemClassObject_SmartNext_recv(ctx, c, apObjects, &uReturned);
    if (W_ERROR_IS_OK(result))
    {
        DEBUG(1, ("wbem_enum_next_continue: %u objects returned\n", uReturned));
        if (uReturned == 0)
        {
            new_ctx = IUnknown_Release_send((struct IUnknown *)s->pEnum, c);
            if (composite_nomem(new_ctx, c)) return;
            composite_continue(c, new_ctx, wbem_enum_release_continue, c);
        }
        else
        {
            new_ctx = IEnumWbemClassObject_SmartNext_send(s->pEnum,
                    c, 0xFFFFFFFF, 7); /* 0xffffffff = lTimeout, 7 = uCount */
            if (composite_nomem(new_ctx, c)) return;

            composite_continue(c, new_ctx, wbem_enum_next_continue, c);
        }
    }
    else
    {
        DEBUG(1, ("wbem_enum_next_continue: %08x %s\n", W_ERROR_V(result),
                wmi_errstr(result)));
        composite_error(c, werror_to_ntstatus(result));
    }

}

static void wbem_exec_query_continue3(struct composite_context *ctx)
{
    struct composite_context *c = NULL;
    struct composite_context *new_ctx = NULL;
    struct wbem_exec_query_state *s = NULL;
    WERROR result;

    /* retrieve the parent composite context */
    c = talloc_get_type(ctx->async.private_data, struct composite_context);

    s = talloc_get_type(c->private_data, struct wbem_exec_query_state);

    result = IEnumWbemClassObject_Reset_recv(ctx);
    if (!W_ERROR_IS_OK(result))
    {
        s->status = werror_to_ntstatus(result);
        composite_error(c, s->status);
        return;
    }

    new_ctx = IEnumWbemClassObject_SmartNext_send(s->pEnum,
            c, 0xFFFFFFFF, 7); /* 0xffffffff = lTimeout, 5 = uCount */
    if (composite_nomem(new_ctx, c)) return;

    composite_continue(c, new_ctx, wbem_enum_next_continue, c);
}

static void wbem_exec_query_continue2(struct composite_context *ctx)
{
    struct composite_context *c = NULL;
    struct wbem_exec_query_state *s = NULL;
    struct composite_context *new_ctx = NULL;
    WERROR result;

    /* retrieve the parent composite context */
    c = talloc_get_type(ctx->async.private_data, struct composite_context);

    s = talloc_get_type(c->private_data, struct wbem_exec_query_state);

    result = IWbemServices_ExecQuery_recv(ctx, &s->pEnum);
    if (!W_ERROR_IS_OK(result))
    {
        s->status = werror_to_ntstatus(result);
        composite_error(c, s->status);
        return;
    }

    new_ctx = IEnumWbemClassObject_Reset_send(s->pEnum, c);
    composite_continue(c, new_ctx, wbem_exec_query_continue3, c);
}

static void wbem_exec_query_continue(struct composite_context *ctx)
{
    struct composite_context *c = NULL;
    struct wbem_exec_query_state *s = NULL;
    struct composite_context *new_ctx = NULL;
    WERROR result;

    /* retrieve the parent composite context */
    c = talloc_get_type(ctx->async.private_data, struct composite_context);

    s = talloc_get_type(c->private_data, struct wbem_exec_query_state);

    result = WBEM_ConnectServer_recv(ctx, c, &s->pServices);
    if (!W_ERROR_IS_OK(result))
    {
        s->status = werror_to_ntstatus(result);
        composite_error(c, s->status);
        return;
    }

    new_ctx = IWbemServices_ExecQuery_send(s->pServices, c, "WQL",
            "SELECT * FROM Win32_NetworkAdapterConfiguration", 0, NULL);
    composite_continue(c, new_ctx, wbem_exec_query_continue2, c);
}

static BOOL torture_wbem_exec_query_async(struct torture_context *torture)
{
    BOOL ret = True;
    TALLOC_CTX *mem_ctx = talloc_init("torture_wbem_login_async");
    struct com_context *com_ctx = NULL;
    struct composite_context *c = NULL;
    struct composite_context *new_ctx = NULL;
    const char *binding = NULL;

    /*
     * Initialize our COM and DCOM contexts.
     */
    com_init_ctx(&com_ctx, NULL);
    dcom_client_init(com_ctx, cmdline_credentials);

    /*
     * Pull our needed test arguments from the torture parameters subsystem.
     */
    binding = torture_setting_string(torture, "binding", NULL);

    /*
     * Create a new composite for our call sequence, with the private data being
     * our return flag.
     */
    c = composite_create(mem_ctx, com_ctx->event_ctx);
    c->private_data = talloc_zero(c, struct wbem_exec_query_state);

    new_ctx = WBEM_ConnectServer_send(com_ctx,
            mem_ctx,        // parent_ctx
            binding,        // server
            "root\\cimv2",  // namespace
            NULL,           // user
            NULL,           // password
            NULL,           // locale
            0,              // flags
            NULL,           // authority
            NULL);          // wbem_ctx
    composite_continue(c, new_ctx, wbem_exec_query_continue, c);
    composite_wait(c);
    ret = NT_STATUS_IS_OK(c->status);
    talloc_free(c);
    talloc_report_full(mem_ctx, stdout);

    return ret;
}

NTSTATUS torture_dcom_init(void)
{
    struct torture_suite *suite = torture_suite_create(
                                        talloc_autofree_context(),
                                        "DCOM");

    /*
     * Initialize all of the DCEPRC tables.
     */
    dcerpc_init();
    dcerpc_table_init();

    /*
     * Initialize all of the DCOM proxies.
     */
    dcom_proxy_IUnknown_init();
    dcom_proxy_IRemUnknown_init();
    dcom_proxy_IWbemLevel1Login_init();
    dcom_proxy_IWbemServices_init();
    dcom_proxy_IEnumWbemClassObject_init();
    dcom_proxy_IWbemFetchSmartEnum_init();
    dcom_proxy_IWbemWCOSmartEnum_init();

    /*
     * Add all of the test cases in this suite.
     */
    torture_suite_add_simple_test(suite, "WBEM-LOGIN", torture_wbem_login);
    torture_suite_add_simple_test(suite, "WBEM-LOGIN-ASYNC",
            torture_wbem_login_async);
    torture_suite_add_simple_test(suite, "WBEM-EXEC-QUERY-ASYNC",
            torture_wbem_exec_query_async);

    /*
     * Finish configuring our test suite and pass it back to the test subsystem.
     */
    suite->description = talloc_strdup(suite,
                        "DCOM protocol and interface tests");

    torture_register_suite(suite);

    return NT_STATUS_OK;
}

