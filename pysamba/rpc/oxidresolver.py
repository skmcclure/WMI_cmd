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
__doc__="Define structure for the ServerAlive2 RPC call"

from pysamba.library import *

class COMVERSION(Structure):
    _fields_ = [
        ('MajorVersion', uint16_t),
        ('MinorVersion', uint16_t),
        ]

class COMINFO(Structure):
    _fields_ = [
        ('version', COMVERSION),
        ('unknown1', uint32_t),
        ]

class DUALSTRINGARRAY(Structure):
    _fields_ = [
        ('stringbindings', c_void_p), # POINTER(POINTER(STRINGBINDING))),
        ('securitybindings', c_void_p), # POINTER(PIONTER(SECURITYBINDING))),
        ]

uint_t = c_uint
class ServerAlive2_out(Structure):
    _fields_ = [
        ('info', POINTER(COMINFO)),
        ('dualstring', POINTER(DUALSTRINGARRAY)),
        ('unknown2', uint8_t*3),
        ('result', WERROR),
        ]
    
class ServerAlive2(Structure):
    _fields_ = [('out', ServerAlive2_out)]
