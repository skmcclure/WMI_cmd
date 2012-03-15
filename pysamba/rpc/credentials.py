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
__doc__ = "Define constants needed for the cli_credentials structure"

( CRED_UNINITIALISED,
  CRED_GUESS_ENV,
  CRED_CALLBACK, 
  CRED_GUESS_FILE,
  CRED_CALLBACK_RESULT, 
  CRED_SPECIFIED ) = range(6)
