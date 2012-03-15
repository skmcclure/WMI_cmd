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

__doc__="""
Define a simple API for performing a sequence of RPC calls to a
single host in an asynchronous fashion
"""

from pysamba.twisted.reactor import eventContext
from pysamba.library import library
from pysamba.twisted.callback import Callback
from pysamba.composite_context import *
from pysamba.talloc import *
from pysamba.rpc.rpc_request import rpc_request

class dcerpc_pipe(Structure): pass
class dcerpc_interface_table(Structure): pass

# struct composite_context* dcerpc_pipe_connect_send(TALLOC_CTX *parent_ctx,
#                                                    const char *binding,
#                                                    const struct dcerpc_interface_table *table,
#                                                    struct cli_credentials *credentials,
#                                                    struct event_context *ev);
library.dcerpc_pipe_connect_send.restype = POINTER(composite_context)
library.dcerpc_pipe_connect_send.argtypes = [c_void_p, c_char_p, c_void_p, c_void_p, c_void_p]

# NTSTATUS dcerpc_pipe_connect_recv(struct composite_context *c,
#                                   TALLOC_CTX *mem_ctx,
#                                   struct dcerpc_pipe **pp);
library.dcerpc_pipe_connect_recv.restype = NTSTATUS
library.dcerpc_pipe_connect_recv.argtypes = [POINTER(composite_context), c_void_p, c_void_p]

# _PUBLIC_ NTSTATUS dcerpc_ndr_request_recv(struct rpc_request *req);
library.dcerpc_ndr_request_recv.restype = NTSTATUS
library.dcerpc_ndr_request_recv.argtypes = [c_void_p]

#
# Continue the RPC connect after a successful socket open to the server by
# receiving the results.
#
def rpc_connect_continue(ctx):
    c = talloc_get_type(ctx.contents.async.private_data, composite_context)
    if not library.composite_is_ok(c):
        return

    # complete the pipe connect and receive a pointer to the
    # dcerpc_pipe structure
    pipe = POINTER(dcerpc_pipe)()
    import time
    c.contents.status = library.dcerpc_pipe_connect_recv(ctx, c, byref(pipe));
    c.contents.async.private_data = cast(pipe, c_void_p)
    if not library.composite_is_ok(c):
        return
    library.composite_done(c);
continue_callback = CFUNCTYPE(None, POINTER(composite_context))
rpc_connect_continue = continue_callback(rpc_connect_continue)

empty_string = ''

#
# Open an RPC connection to the specified server
#
def async_rpc_open(event_ctx, server, cred, binding, arg, callback):
    # create a composite context for this sequence of asynchronous calls
    c = composite_create(None, event_ctx)
    c.contents.async.fn = callback
    c.contents.async.private_data = None

    # build a binding string using the only protocol we care about...
    binding = talloc_asprintf(c, "%s:%s", binding, server)

    # create a set of credentials
    creds = library.cli_credentials_init(c)
    if library.composite_nomem(creds, c):
        return c
    library.cli_credentials_set_conf(creds);
    if cred:
        library.cli_credentials_parse_string(creds, cred, CRED_SPECIFIED);

    # issue the asynchronous rpc pipe connect request
    rpc_ctx = library.dcerpc_pipe_connect_send(c,
                                              binding,
                                              arg,
                                              creds,
                                              event_ctx)
    if library.composite_nomem(rpc_ctx, c):
        return c

    # setup the next stage of the connect process
    library.composite_continue(c, rpc_ctx, rpc_connect_continue, c)
    return c

# Fetch the return result after an RPC call completes
def continue_rpc(rpc_ctx):
    c = talloc_get_type(rpc_ctx.contents.async.private, composite_context)
    c.contents.async.private_data = rpc_ctx.contents.ndr.struct_ptr;
    c.contents.status = library.dcerpc_ndr_request_recv(rpc_ctx)
    if not library.composite_is_ok(c):
        return
    library.composite_done(c)
continue_rpc_callback = CFUNCTYPE(None, POINTER(rpc_request))
continue_rpc = continue_rpc_callback(continue_rpc)

class Rpc(object):

    def __init__(self):
        self.ctx = self.rpc_pipe = None

    def __del__(self):
        self.close()

    def close(self):
        if self.ctx:
            talloc_free(self.ctx)
        self.ctx = self.rpc_pipe = None

    def connect(self, host, credentials, object, binding='ncacn_np'):
        table = dcerpc_interface_table.in_dll(library, 'dcerpc_table_' + object)
        cb = Callback()
        ctx = async_rpc_open(eventContext,
                             host,
                             credentials,
                             binding,
                             byref(table),
                             cb.callback)
        cb.deferred.addCallback(self._store_rpc_pipe, ctx)
        cb.deferred.addErrback(self._errback_cleanup, ctx)
        return cb.deferred

    def _errback_cleanup(self, result, ctx):
        talloc_free(ctx)
        return result

    def _store_rpc_pipe(self, rpc_pipe, ctx):
        self.ctx = ctx
        self.rpc_pipe = rpc_pipe
        return rpc_pipe

    # ctx is the memory context to use to make the call, if not the one that
    # started the original open call
    def call(self, send_func, arg, ctx = None):
        cb = Callback()
        if ctx is None:
            ctx = self.ctx
        ctx.contents.async.fn = cb.callback
        ctx.contents.async.private_data = None
        rpc_ctx = send_func(self.rpc_pipe, ctx, arg)
        if library.composite_nomem(rpc_ctx, ctx):
            return
        library.composite_continue_rpc(ctx, rpc_ctx, continue_rpc, ctx);
        return cb.deferred

