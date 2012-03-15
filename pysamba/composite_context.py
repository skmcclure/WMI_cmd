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

from pysamba.library import *
from pysamba.rpc.credentials import CRED_SPECIFIED

import logging
log = logging.getLogger('p.composite_context')

( COMPOSITE_STATE_INIT, COMPOSITE_STATE_IN_PROGRESS,
  COMPOSITE_STATE_DONE, COMPOSITE_STATE_ERROR ) = range(4)
    
class composite_context(Structure): pass
composite_context_callback = CFUNCTYPE(None, POINTER(composite_context));
class async(Structure):
    _fields_ = [
        ('fn', composite_context_callback),
        ('private_data', c_void_p),
        ]

composite_context._fields_ = [
    ('state', enum),
    ('private_data', c_void_p),
    ('status', NTSTATUS),
    ('event_ctx', c_void_p), # struct event_context *
    ('async', async),
    ('used_wait', BOOL),
    ]

# _PUBLIC_ struct composite_context *composite_create(TALLOC_CTX *mem_ctx,
#                                                     struct event_context *ev);
library.composite_create.restype = POINTER(composite_context)
library.composite_create.argtypes = [c_void_p, c_void_p]

def composite_create(memctx, eventContext):
    result = library.composite_create(memctx, eventContext)
    if not result:
        raise RuntimeError("Unable to allocate a composite_context")
    return result

# _PUBLIC_ BOOL composite_nomem(const void *p, struct composite_context *ctx);
library.composite_nomem.restype = BOOL
library.composite_nomem.argtypes = [c_void_p, POINTER(composite_context)]

