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
__doc__ = "Define common structures used to perform DCE-RPC calls"

from pysamba.library import *

class GUID(Structure):
    _fields_ = [
        ('time_low', uint32_t),
        ('time_mid', uint16_t),
        ('time_hi_and_version', uint16_t),
        ('clock_seq', uint8_t*2),
        ('node', uint8_t*6),
        ]

class policy_handle(Structure):
    _fields_ = [
        ('handle_type', uint32_t),
        ('uuid', GUID),
        ]


class dcerpc_syntax_id(Structure):
    _fields_ = [
        ('uuid', GUID),
        ('if_version', uint32_t),
        ]

class dcerpc_pipe(Structure):
    _fields_ = [
        ('context_id', uint32_t),
        ('syntax', dcerpc_syntax_id),
        ('transfer_syntax', dcerpc_syntax_id),
        ('conn', c_void_p),    # lie: struct dcerpc_connection *
        ('binding', c_void_p), # lie: struct dcerpc_binding *
        ('last_fault_code', uint32_t),
        ('request_timeout', uint32_t),
        ]

