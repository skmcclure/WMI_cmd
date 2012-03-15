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
Support classes for integrating deferreds into the Samba asynchronous framework
"""

from pysamba.composite_context import *
from twisted.internet import defer

DEFERS = {}
COUNTER = 0L

class WMIFailure(Exception):
    "Exception that represents a composite_context failure"
    def __str__(self):
        return library.nt_errstr(self.args[1])

class Callback(object):
    "Turn a composite_context callback into a deferred callback"
    def __init__(self):
        # keep a reference to this object as long as it lives in the C code
        global COUNTER
        COUNTER += 1 
        self.which = COUNTER
        DEFERS[self.which] = self
        self.callback = composite_context_callback(self.callback)
        self.deferred = defer.Deferred()
        
    def callback(self, ctx):
        # remove the reference to the object now that we're out of C code
        try:
            DEFERS.pop(self.which)
        except KeyError, ex:
            import pdb; pdb.set_trace()
        d = self.deferred
        if ctx.contents.state == COMPOSITE_STATE_DONE:
            d.callback(ctx.contents.async.private_data)
        else:
            d.errback(WMIFailure(ctx.contents.state,
                                 ctx.contents.status))
