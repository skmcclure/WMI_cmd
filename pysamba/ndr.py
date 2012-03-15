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
__doc__ = "Provide access to generic NDR routines"

from pysamba.library import library

NDR_OUT = 2
def debugPrint(text, printFunct, obj):
    "Print an NDR structure to debug output"
    library.ndr_print_function_debug(printFunct, text, NDR_OUT, obj)
