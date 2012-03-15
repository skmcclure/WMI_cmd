/*
   WMI method call client

   Greg Jednaszewski
   Copyright (C) 2010 Racemi, Inc.

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
   
   TODO:
   - Add support for array input args
*/

#include "includes.h"
#include "lib/cmdline/popt_common.h"
#include "auth/credentials/credentials.h"
#include "librpc/rpc/dcerpc.h"
#include "librpc/gen_ndr/ndr_oxidresolver.h"
#include "librpc/gen_ndr/ndr_oxidresolver_c.h"
#include "librpc/gen_ndr/dcom.h"
#include "librpc/gen_ndr/ndr_dcom.h"
#include "librpc/gen_ndr/ndr_dcom_c.h"
#include "librpc/gen_ndr/ndr_remact_c.h"
#include "librpc/gen_ndr/ndr_epmapper_c.h"
#include "librpc/gen_ndr/com_dcom.h"
#include "librpc/rpc/dcerpc_table.h"

#include "lib/com/dcom/dcom.h"
#include "lib/com/proto.h"
#include "lib/com/dcom/proto.h"

#include "wmi/wmi.h"

#include "wmi/proto.h"

struct program_args
{
    char *hostname;
    char *namespace;
    char *obj_path;
    char *method;
    char *method_args;
};


static void parse_args(int argc, char *argv[], struct program_args *pmyargs)
{
    poptContext pc;
    int opt, i;

    int argc_new;
    char **argv_new;

    const char *help =
        "wmi_method [options] //host object-path method\n\n"
        "STATIC METHODS:\n"
        "To execute a static method, set object-path to the name of the class.\n"
        "For example to create a process on the server: \n\n"
        "wmi_method -U [domain/]adminuser%password -a 'CommandLine,string,notepad.exe' //host Win32_Process Create\n\n"
        "INSTANCE METHODS:\n"
        "To execute an instance method, set object-path to the path to the\n"
        "object, including the key properties that identify the object.\n"
        "For example to shut down a HyperV VM: \n\n"
        "wmi_method --namespace='root\\virtualization' -U [domain/]adminuser%password  -a 'RequestedState,uint16,3' //host 'Msvm_ComputerSystem.CreationClassName=\"Msvm_ComputerSystem\",Name=\"24C0F23E-90C0-47D3-B074-5A2B33181C46\"' RequestStateChange\n";
    
    struct poptOption long_options[] = {
    POPT_AUTOHELP
    POPT_COMMON_SAMBA
    POPT_COMMON_CONNECTION
    POPT_COMMON_CREDENTIALS
    {"namespace", 0, POPT_ARG_STRING, &pmyargs->namespace, 0,
     "WMI namespace, default to root\\cimv2", 0},
    {"method-args", 'a', POPT_ARG_STRING, &pmyargs->method_args, 0,
     "Pipe-separated list of name,type,value tuples representing "
     "the arguments to pass into method. Valid types are: "
     "int8, uint8, unt16, uint16, int32, uint32, dlong, "
     "udlong, string", 0},
    POPT_TABLEEND
    };

    pc = poptGetContext("wmi", argc, (const char **) argv,
                        long_options, POPT_CONTEXT_KEEP_FIRST);

    poptSetOtherOptionHelp(pc, help);

    while ((opt = poptGetNextOpt(pc)) != -1)
    {
        poptPrintUsage(pc, stdout, 0);
        poptFreeContext(pc);
        exit(1);
    }

    argv_new = discard_const_p(char *, poptGetArgs(pc));

    argc_new = argc;
    for (i = 0; i < argc; i++)
    {
        if (argv_new[i] == NULL)
        {
            argc_new = i;
            break;
        }
    }

    if (argc_new < 4 || argv_new[1][0] != '/' || argv_new[1][1] != '/')
    {
        poptPrintUsage(pc, stdout, 0);
        poptFreeContext(pc);
        exit(1);
    }

    pmyargs->hostname = argv_new[1] + 2;
    pmyargs->obj_path = argv_new[2];
    pmyargs->method   = argv_new[3];
    poptFreeContext(pc);
}

#define WERR_CHECK(msg) if (!W_ERROR_IS_OK(result)) { \
                DEBUG(0, ("ERROR: %s\n", msg)); \
                fflush(stdout);                 \
                goto error; \
            } else { \
                DEBUG(1, ("OK   : %s\n", msg)); \
                fflush(stdout);                 \
            }

#define RETURN_CVAR_ARRAY_STR(fmt, arr) {\
        uint32_t i;\
	char *r;\
\
        if (!arr) {\
                return talloc_strdup(mem_ctx, "NULL");\
        }\
	r = talloc_strdup(mem_ctx, "(");\
        for (i = 0; i < arr->count; ++i) {\
		r = talloc_asprintf_append(r, fmt "%s", arr->item[i], (i+1 == arr->count)?"":",");\
        }\
        return talloc_asprintf_append(r, ")");\
}

WERROR add_arg_by_type(union CIMVAR *v, char *type, char *val, int len)
{
    if (! strncmp(type, "int8", len))
        v->v_sint8 = atoi(val);
    else if (! strncmp(type, "uint8", len))
        v->v_uint8 = atoi(val);
    else if (! strncmp(type, "int16", len))
        v->v_sint16 = atoi(val);
    else if (! strncmp(type, "uint16", len))
        v->v_uint16 = atoi(val);
    else if (! strncmp(type, "int32", len))
        v->v_sint32 = atoi(val);
    else if (! strncmp(type, "uint32", len))
        v->v_uint32 = atoi(val);
    else if (! strncmp(type, "dlong", len))
        v->v_sint64 = atol(val);
    else if (! strncmp(type, "udlong", len))
        v->v_uint64 = atol(val);
    else if (! strncmp(type, "string", len))
        v->v_string = val;
    else
        return WERR_INVALID_PARAM;

    return WERR_OK;
}

WERROR set_method_args(struct IWbemClassObject *in, TALLOC_CTX *ctx, char *args)
{
    WERROR result = WERR_OK;
    union CIMVAR v;
    size_t args_len;
    char *token;

    if (!args)
        return result;
    
    args_len = strlen(args);
    token    = strtok(args, "|");
    while (token != NULL)
    {
        char *key, *value, *type, *pos;
        /* Each token is a name,type,value tuple.  Find all the commas and
         * replace them with string terminators.  Then set the key, value,
         * and type accordingly
         * */
        key = token;
        if ( (pos = strchr(token, ',')) == NULL )
        {
            fprintf(stderr,
                    "Method Args must be specified as a pipe (|) separated "
                    "list of key,type,value tuples.");
            result = WERR_INVALID_PARAM;
            goto error;
        }
        *pos = 0;

        type = pos + sizeof(char);
        if ( (pos = strchr(type, ',')) == NULL )
        {
            fprintf(stderr,
                    "Method Args must be specified as a pipe (|) separated "
                    "list of key,type,value tuples.");
            result = WERR_INVALID_PARAM;
            goto error;
        }
        *pos = 0;

        value = pos + sizeof(char);

        DEBUG(1, ("     : Setting arg %s (%s) = %s\n", key, type, value));

        result = add_arg_by_type(&v, type, value, strlen(token));
        WERR_CHECK("_insertArgByType");

        result = IWbemClassObject_Put(in, ctx, key, 0, &v, 0);
        WERR_CHECK("IWbemClassObject_Put.");
        
        token = strtok(NULL, " ");
    }

    error:
    return result;
}

char *string_CIMVAR(TALLOC_CTX *mem_ctx, union CIMVAR *v,
                    enum CIMTYPE_ENUMERATION cimtype)
{
	switch (cimtype)
    {
        case CIM_SINT8: return talloc_asprintf(mem_ctx, "%d", v->v_sint8);
        case CIM_UINT8: return talloc_asprintf(mem_ctx, "%u", v->v_uint8);
        case CIM_SINT16: return talloc_asprintf(mem_ctx, "%d", v->v_sint16);
        case CIM_UINT16: return talloc_asprintf(mem_ctx, "%u", v->v_uint16);
        case CIM_SINT32: return talloc_asprintf(mem_ctx, "%d", v->v_sint32);
        case CIM_UINT32: return talloc_asprintf(mem_ctx, "%u", v->v_uint32);
        case CIM_SINT64: return talloc_asprintf(mem_ctx, "%lld", v->v_sint64);
        case CIM_UINT64: return talloc_asprintf(mem_ctx, "%llu", v->v_sint64);
        case CIM_REAL32: return talloc_asprintf(mem_ctx, "%f", (double)v->v_uint32);
        case CIM_REAL64: return talloc_asprintf(mem_ctx, "%f", (double)v->v_uint64);
        case CIM_BOOLEAN: return talloc_asprintf(mem_ctx, "%s", v->v_boolean?"True":"False");
        case CIM_STRING:
        case CIM_DATETIME:
        case CIM_REFERENCE: return talloc_asprintf(mem_ctx, "%s", v->v_string);
        case CIM_CHAR16: return talloc_asprintf(mem_ctx, "Unsupported");
        case CIM_OBJECT: return talloc_asprintf(mem_ctx, "Unsupported");
        case CIM_ARR_SINT8: RETURN_CVAR_ARRAY_STR("%d", v->a_sint8);
        case CIM_ARR_UINT8: RETURN_CVAR_ARRAY_STR("%u", v->a_uint8);
        case CIM_ARR_SINT16: RETURN_CVAR_ARRAY_STR("%d", v->a_sint16);
        case CIM_ARR_UINT16: RETURN_CVAR_ARRAY_STR("%u", v->a_uint16);
        case CIM_ARR_SINT32: RETURN_CVAR_ARRAY_STR("%d", v->a_sint32);
        case CIM_ARR_UINT32: RETURN_CVAR_ARRAY_STR("%u", v->a_uint32);
        case CIM_ARR_SINT64: RETURN_CVAR_ARRAY_STR("%lld", v->a_sint64);
        case CIM_ARR_UINT64: RETURN_CVAR_ARRAY_STR("%llu", v->a_uint64);
        case CIM_ARR_REAL32: RETURN_CVAR_ARRAY_STR("%f", v->a_real32);
        case CIM_ARR_REAL64: RETURN_CVAR_ARRAY_STR("%f", v->a_real64);
        case CIM_ARR_BOOLEAN: RETURN_CVAR_ARRAY_STR("%d", v->a_boolean);
        case CIM_ARR_STRING: RETURN_CVAR_ARRAY_STR("%s", v->a_string);
        case CIM_ARR_DATETIME: RETURN_CVAR_ARRAY_STR("%s", v->a_datetime);
        case CIM_ARR_REFERENCE: RETURN_CVAR_ARRAY_STR("%s", v->a_reference);
        default: return talloc_asprintf(mem_ctx, "Unsupported");
	}
}

void print_properties(TALLOC_CTX *ctx, struct IWbemClassObject *obj)
{
    struct WbemClassObject *data;
    int i, num_properties;
    
    data = obj->object_data;
    num_properties = data->obj_class->__PROPERTY_COUNT; 

    for (i = 0; i < num_properties; i++)
    {
        const char *key, *val;
        key = data->obj_class->properties[i].name;
        val = string_CIMVAR(ctx, &data->instance->data[i],
                            data->obj_class->properties[i].desc->cimtype
                            & CIM_TYPEMASK);
        printf("%s: %s\n", key, val);
	}
}

WERROR execute_wmi_method(struct IWbemServices *pWS, char *obj_path,
                          char *class, char *method, char *args)
{
    struct IWbemClassObject *in_params  = NULL;
    struct IWbemClassObject *out_params = NULL;
    WERROR result = WERR_OK;
    TALLOC_CTX *ctx;
    
    ctx = talloc_new(0);

    if (args)
    {
        /* Get an instance of the method object and populate it with
         * any 'in' parameters
         * */
        struct IWbemClassObject *class_obj = NULL;
        struct IWbemClassObject *method_class;
        
        result = IWbemServices_GetObject(pWS, ctx, class,
                                         WBEM_FLAG_RETURN_WBEM_COMPLETE, NULL,
                                         &class_obj, NULL);
        WERR_CHECK("IWbemClassObject_GetObject.");

        result = IWbemClassObject_GetMethod(class_obj, ctx, method, 0,
                                            &method_class, NULL);
        WERR_CHECK("IWbemClassObject_GetMethod.");

        result = IWbemClassObject_SpawnInstance(method_class, ctx, 0,
                                                &in_params);
        WERR_CHECK("IWbemClassObject_SpawnInstance.");

        result = set_method_args(in_params, ctx, args);
        WERR_CHECK("setMethodArgs.");
    }

    /* Execute the method & print results */
    result = IWbemServices_ExecMethod(pWS, ctx, obj_path, method, 0, NULL,
                                      in_params, &out_params, NULL);
    WERR_CHECK("IWbemServices_ExecMethod.");

    /* Print the results of the call */
    print_properties(ctx, out_params);

    error:
    talloc_free(ctx);
    return result;
}

char *get_class_from_obj_path(struct com_context *ctx, char *obj_path)
{
    char *pos, *class;

    class = talloc_strdup(ctx, obj_path);
    
    /* If there is no dot in the object path, the user wants to invoke a
     * static method.  e.g. Create on Win32_Process. In that case the
     * object path is the same as the class name.     
     * */
    if ( (pos = strchr(class, '.')) == NULL )
        return class;

    /* If there is a dot in the object path, the user wants to invoke an
     * instance method. e.g. Terminate on Win32_Process.Handle="4425".
     * Just chop off everything after the dot.            
     * */
    *pos = '\0';
    return class;
}

int main(int argc, char **argv)
{
    struct program_args args = {};
    struct com_context *ctx = NULL;
    WERROR result = WERR_OK;
    NTSTATUS status;
    struct IWbemServices *pWS = NULL;
    char *class;
    
    parse_args(argc, argv, &args);

    dcerpc_init();
    dcerpc_table_init();

    dcom_proxy_IUnknown_init();
    dcom_proxy_IWbemLevel1Login_init();
    dcom_proxy_IWbemServices_init();
    dcom_proxy_IEnumWbemClassObject_init();
    dcom_proxy_IRemUnknown_init();
    dcom_proxy_IWbemFetchSmartEnum_init();
    dcom_proxy_IWbemWCOSmartEnum_init();
    dcom_proxy_IWbemClassObject_init();

    com_init_ctx(&ctx, NULL);
    dcom_client_init(ctx, cmdline_credentials);

    if (!args.namespace)
        args.namespace = "root\\cimv2";

    class = get_class_from_obj_path(ctx, args.obj_path);
    DEBUG(1, ("Hostname:    %s\n", args.hostname));
    DEBUG(1, ("Namespace:   %s\n", args.namespace));
    DEBUG(1, ("Class:       %s\n", class));
    DEBUG(1, ("Obj Path:    %s\n", args.obj_path));
    DEBUG(1, ("Method:      %s\n", args.method));

    if (args.method_args)
        DEBUG(1, ("Method Args: %s\n", args.method_args));
    else
        DEBUG(1, ("Method Args: None\n\n"));
    
    result = WBEM_ConnectServer(ctx, args.hostname, args.namespace,
                                0, 0, 0, 0, 0, 0, &pWS);
    WERR_CHECK("WBEM_ConnectServer.");

    /* Invoke method on the class */
    result = execute_wmi_method(pWS, args.obj_path, class, args.method,
                                args.method_args);
    WERR_CHECK("execute_wmi_method.");

    error:
    talloc_free(ctx);
    if (!W_ERROR_IS_OK(result))
    {
        status = werror_to_ntstatus(result);
        fprintf(stderr, "NTSTATUS: %s - %s\n",
                nt_errstr(status), get_friendly_nt_error_msg(status));
        return 1;
    }
    return 0;
}
