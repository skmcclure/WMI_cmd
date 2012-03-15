/*
   WMI Sample client
   Copyright (C) 2006 Andrzej Hajda <andrzej.hajda@wp.pl>

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

#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define STRING_SIZE 1024
#define TIMEOUT 30              // seconds
#define CHECK_INTERVAL 500      // milliseconds
#define PROC_STOP_QUERY "SELECT * FROM Win32_ProcessStopTrace"

#define WERR_CHECK(msg) if (!W_ERROR_IS_OK(result)) { \
			    DEBUG(1, ("ERROR: %s\n", msg)); \
			    goto error; \
			} else { \
			    DEBUG(1, ("OK   : %s\n", msg)); \
			}

struct program_args {
    char *hostname;
    char *scriptfile;
    int timeout;
};

static void parse_args(int argc, char *argv[], struct program_args *pmyargs)
{
    poptContext pc;
    int opt, i, argc_new;
    char **argv_new;
    
    struct poptOption long_options[] = {
        POPT_AUTOHELP
        POPT_COMMON_SAMBA
        POPT_COMMON_CONNECTION
        POPT_COMMON_CREDENTIALS
        {"timeout", 't', POPT_ARG_INT, &pmyargs->timeout, 0,
         "Timeout (sec) for each individual command, defaults to 60 sec", 0},
        POPT_TABLEEND
    };
    
    pc = poptGetContext("wmi", argc, (const char **) argv,
                        long_options, POPT_CONTEXT_KEEP_FIRST);
    
    poptSetOtherOptionHelp(pc, "//host\n\nExample: wmi_rexec -U [domain/]adminuser%password -t 600 //host <script-file>");
    
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

    if (argc_new < 1 || argv_new[1][0] != '/' || argv_new[1][1] != '/')
    {
        poptPrintUsage(pc, stdout, 0);
        poptFreeContext(pc);
        exit(1);
    }
    
    pmyargs->hostname   = argv_new[1] + 2;
    pmyargs->scriptfile = argv_new[2];
    poptFreeContext(pc);
}

/*
 * Blocks until the process with the specified PID has completed. It
 * does this by checking the Win32_ProcessStopTrace table for the
 * given PID every CHECK_INTERVAL seconds for a maximum of TIMEOUT
 * seconds.  (These values can be changed in wmi_common.h).
 *
 * The exit code of the process is placed in the 'exitstatus' parameter.
 * */
WERROR WBEM_WaitForProcessExit(struct IWbemServices *pWS,
                               struct IEnumWbemClassObject *pEnum,
                               uint32_t pid, int timeout, uint32_t *exitstatus)
{
    TALLOC_CTX *ctx;
    union CIMVAR v;
    WERROR result;
    time_t starttime;

    ctx = talloc_new(0);
    starttime = time(NULL);

    while (1)
    {
        struct WbemClassObject *co[1];
        uint32_t count;

        // See if timeout has expired
        if (time(NULL) - starttime >= timeout)
        {
            fprintf(stderr, "ERROR: Timed out waiting for process exit\n");
            exit(1);
        }

        DEBUG(1, ("Waiting for pid %d to exit...\n", pid));
        result = IEnumWbemClassObject_SmartNext(
            pEnum, ctx, CHECK_INTERVAL, 1, co, &count);
        WERR_CHECK("IEnumWbemClassObject_Next.");

        // Didn't get any results, try again
        if (count < 1)
        {
            DEBUG(1, ("No results\n"));
            continue;
        }
            
        // We will be notified anytime a process exits, so check to
        // see if the PID matches.
        result = WbemClassObject_Get(co[0], ctx, "ProcessID", 0, &v, 0, 0);
        WERR_CHECK("IWbemClassObject_Get(ProcessId).");
        if (v.v_uint32 != pid)
        {
            DEBUG(1, ("Got entry for pid %d instead of %d\n", v.v_uint32, pid));
            continue;
        }
        
        // PID matches. Extract process ExitStatus
        result = WbemClassObject_Get(co[0], ctx, "ExitStatus", 0, &v, 0, 0);
        WERR_CHECK("IWbemClassObject_Get(ExitStatus).");
        *exitstatus = v.v_uint32;
        break;
    }
    
error:
    talloc_free(ctx);
    return result;
}


/*
 * Starts a process with the given command on the remote server.  If
 * wait_for_complete is zero, return right away, otherwise block until
 * the process exits.  If the process exits with a non-zero exit code,
 * an error is raises.
 * */
WERROR WBEM_RemoteExecute(struct IWbemServices *pWS, const char *command,
                          int wait_for_complete, int timeout)
{
    struct IWbemClassObject *wco = NULL;
    struct IWbemClassObject *inc, *outc, *in;
    struct IWbemClassObject *out = NULL;
    struct IEnumWbemClassObject *pEnum = NULL;
    WERROR result;
    union CIMVAR v;
    TALLOC_CTX *ctx;
    uint32_t pid, exitstatus;
    
    ctx = talloc_new(0);
    
    result = IWbemServices_GetObject(pWS, ctx, "Win32_Process",
                                     WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                     NULL, &wco, NULL);
    WERR_CHECK("GetObject(Win32_Process)");
    
    result = IWbemClassObject_GetMethod(wco, ctx, "Create", 0, &inc, &outc);
    WERR_CHECK("IWbemClassObject_GetMethod(Create)");
    
    result = IWbemClassObject_SpawnInstance(inc, ctx, 0, &in);
    WERR_CHECK("IWbemClassObject_SpawnInstance");
    
    v.v_string = command;
    result = IWbemClassObject_Put(in, ctx, "CommandLine", 0, &v, 0);
    WERR_CHECK("IWbemClassObject_Put(CommandLine).");
    
    if (wait_for_complete)
    {
        // We are going to look for the process to show up in the
        // Win32_ProcessStopTrace table, indicating that it has completed.
        // Issue the query before creating the process, since there's a
        // small chance that it could complete before we issue the
        // query. Later call SmartNext to block for the object to
        // show up. 
        result = IWbemServices_ExecNotificationQuery(
            pWS, ctx, "WQL", PROC_STOP_QUERY,
            WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
            NULL, &pEnum);
        WERR_CHECK("WMI notification query execute.");
    }

    result = IWbemServices_ExecMethod(pWS, ctx, "Win32_Process",
                                      "Create", 0, NULL, in, &out, NULL);
    WERR_CHECK("IWbemServices_ExecMethod(Win32_Process::Create).");
    
    if (wait_for_complete)
    {
        result = WbemClassObject_Get(out->object_data, ctx,
                                     "ProcessId", 0, &v, 0, 0);
        WERR_CHECK("IWbemClassObject_Get(ProcessId).");
        pid = v.v_uint32;

        // Wait for it to finish
        result = WBEM_WaitForProcessExit(pWS, pEnum, pid, timeout, &exitstatus);
        WERR_CHECK("WBEM_WaitForProcessExit.");
        
        // Check the returm code
        if (exitstatus != 0)
        {
            fprintf(stderr, "ERROR: Command '%s' exited with code %d\n",
                    command, exitstatus);
            exit(1);
        }
    }
        
error:
    talloc_free(ctx);
    return result;
}

WERROR process_file(char *scriptName, struct IWbemServices *pWS,
                    int timeout)
{
    FILE* scriptFile = NULL;
    WERROR result;
    int len;
    
    DEBUG(1, ("attempting to read script: %s\n", scriptName));
    if ((scriptFile = fopen(scriptName,"r")) == NULL)
    {
        fprintf(stderr, "ERROR: Could not open script file %s\n", scriptName);
        exit(1);
    }

    while(1)
    { 
        char command[STRING_SIZE];
        bzero(command, STRING_SIZE);

        if (fgets(command, STRING_SIZE, scriptFile) == NULL)
        {
            if (feof(scriptFile))
                break;
            
            fprintf(stderr, "Unable to read line from %s\n", scriptFile);
            exit(1);
        }

        len = strlen(command);

        // Strip off newlines
        if (command[len - 1] == '\n')
        {
            command[len - 1] = 0;
            len--;
        }

        // Skip blank lines
        if (len == 0)
            continue;

        // Create the process
        DEBUG(0, ("Run: '%s'\n", command));
        result = WBEM_RemoteExecute(pWS, command, 1, timeout);
        WERR_CHECK("WBEM_RemoteExecute.");
    }
    
error:
    return result;
}


int main(int argc, char **argv)
{
    struct   program_args args = {};
    struct   com_context *ctx = NULL;
    WERROR   result;
    NTSTATUS status;
    struct   IWbemServices *pWS = NULL;

    // Default timeout is 60 sec
    args.timeout = 60;
    parse_args(argc, argv, &args);

    DEBUG(1, ("Timeout: %d\n", args.timeout));
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

    result = WBEM_ConnectServer(ctx, args.hostname,
                    "root\\cimv2", 0, 0, 0, 0, 0, 0, &pWS);
    WERR_CHECK("WBEM_ConnectServer.");

    result = process_file(args.scriptfile, pWS, args.timeout);
    WERR_CHECK("process_script_file.");
    talloc_free(ctx);
    return 0;

error:
    status = werror_to_ntstatus(result);
    fprintf(stderr, "NTSTATUS: %s - %s\n",
            nt_errstr(status), get_friendly_nt_error_msg(status));
    talloc_free(ctx);
    return 1;
}
