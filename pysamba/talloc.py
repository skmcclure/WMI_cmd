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
__doc__= "Re-implement the talloc macros in a python-compatible way"

from pysamba.library import library
from ctypes import *

class TallocError(Exception): pass

# function wrapper to check for out-of-memory and turn it into an exception
def check(f):
    def inner(*args, **kw):
        res = f(*args, **kw)
        if not res:
            raise TallocError("Out of memory")
        return res
    return inner

# talloc remembers the strings we pass in... so we need to make sure
# that the names we provide live forever.  This lets us hold onto a
# reference to the string without keeping a string for every allocation.
live_forever = {}

library._talloc_zero.restype = c_void_p

@check
def talloc_zero(ctx, type):
    typename = 'struct ' + type.__name__
    typename = live_forever.setdefault(typename, typename)
    return cast(library._talloc_zero(ctx,
                                     sizeof(type),
                                     typename),
                POINTER(type))

library.talloc_asprintf.restype = c_char_p

@check
def talloc_asprintf(*args):
    return library.talloc_asprintf(*args)

def talloc_get_type(obj, type):
    result = library.talloc_check_name(obj, 'struct ' + type.__name__)
    if not result:
        raise TallocError("Probable mis-interpretation of memory block: "
                          "Have %s, wanted %s" % (talloc_get_name(obj),
                                                  'struct ' + type.__name__))
    return cast(result, POINTER(type))

@check
def talloc_array(ctx, type, count):
    obj = library._talloc_array(ctx,
                                sizeof(type),
                                count,
                                'struct ' + type.__name__)
    return cast(obj, POINTER(type))

talloc_free = library.talloc_free
talloc_increase_ref_count = library.talloc_increase_ref_count

library.talloc_get_name.restype = c_char_p
talloc_get_name = library.talloc_get_name
