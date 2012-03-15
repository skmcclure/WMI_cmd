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
__doc__="python classes to integrate the twisted and Samba event loops"

from pysamba.library import *

import os
import sys
import errno
import sys
import logging
logging.basicConfig()
log = logging.getLogger("zen.pysamba")

EVENT_FD_READ = 1
EVENT_FD_WRITE = 2

class Timer(object):
    callLater = None
timer = Timer()

class timeval(Structure):
    _fields_ = [
        ('tv_sec', c_long),
        ('tv_usec', c_long),
        ]

from twisted.internet.selectreactor import SelectReactor
from select import select

class WmiReactorMixin:
    def __init__(self):
        self.readFdMap = {}
        self.writeFdMap = {}

    def callTimeouts(self, delay):
        """
        Perform a single iteration of the reactor. Here we make sure that
        all of our file descriptors that we need to watch are and then delegate
        the actual watching back to the twisted reactor.
        """
        
        #
        # determine the next timeout interval based upon any queued events
        #
        timeout = timeval()
        while True:
            library.zenoss_get_next_timeout(eventContext, byref(timeout))
            t = timeout.tv_sec + timeout.tv_usec / 1e6
            if t > 0.0: break

        if timer.callLater:
            timer.callLater.cancel()
            timer.callLater = None
        timer.callLater = self.callLater(t, self.__checkTimeouts)

    def removeFileDescriptor(self, fd):
        self.watchFileDescriptor(fd, 0)

    def watchFileDescriptor(self, fd, flags):
        if (flags & EVENT_FD_READ) != 0:
            if fd not in self.readFdMap:
                reader = ActivityHook('Reader', fd)
                self.readFdMap[fd] = reader
                self.addReader(self.readFdMap[fd])
        elif fd in self.readFdMap:
            self.removeReader(self.readFdMap[fd])
            del self.readFdMap[fd]

        if (flags & EVENT_FD_WRITE) != 0:
            if fd not in self.writeFdMap:
                writer = ActivityHook('Writer', fd)
                self.writeFdMap[fd] = writer
                self.addWriter(self.writeFdMap[fd])
        elif fd in self.writeFdMap:
            self.removeWriter(self.writeFdMap[fd])
            del self.writeFdMap[fd]
            
        return 0

    def __checkTimeouts(self):
        timer.callLater = None

    def doOnce(self):
        self.doIteration(0.1)
        return 0

class WmiReactor(WmiReactorMixin, SelectReactor):
    def __init__(self):
        SelectReactor.__init__(self)
        WmiReactorMixin.__init__(self)

    def _preenDescriptors(self):
        for fdMap, lst in (
            (self.readFdMap, self.readFdMap.keys()),
            (self.writeFdMap.keys(), self.writeFdMap.keys())
            ):
            for fd in lst:
                try:
                    select(fd + 1, [fd], [fd], [fd], 0)
                except:
                    try:
                        fdMap.pop(fd)
                    except IndexError:
                        pass
        SelectReactor._preenDescriptors(self)

    def doIteration(self, delay):
        """
        Perform a single iteration of the reactor. Here we make sure that
        all of our file descriptors that we need to watch are and then delegate
        the actual watching back to the twisted reactor.
        """

        self.callTimeouts(delay)
        return SelectReactor.doIteration(self, delay)

#
# Install the WmiReactor instead of the default twisted one. See the following
# ticket for the history of using an epoll vs. select reactor.
#   http://dev.zenoss.org/trac/ticket/4640
#
if os.uname()[0] == 'Linux':
    from twisted.internet.epollreactor import EPollReactor
    class WmiEPollReactor(WmiReactorMixin, EPollReactor):
        def __init__(self):
            EPollReactor.__init__(self)
            WmiReactorMixin.__init__(self)

        def _remove(self, xer, primary, other, selectables, event, antievent):
            try:
                return EPollReactor._remove(self,
                                            xer, primary, other, selectables,
                                            event, antievent)
            except IOError, err:
                # huh, wha?  oh, we weren't listening for that fileno anyhow
                if err.errno != errno.EBADF:
                    raise

        def doPoll(self, delay):
            self.callTimeouts(delay)
            return EPollReactor.doPoll(self, delay)

        doIteration = doPoll

    wmiReactor = WmiEPollReactor()

else:
    wmiReactor = WmiReactor()

watch_fd_callback = CFUNCTYPE(c_int, c_int, c_uint16)
loop_callback = CFUNCTYPE(c_int)
class Callbacks(Structure):
    _fields_ = [
        ('fd_callback', watch_fd_callback),
        ('loop_callback', loop_callback)
        ]
callback = Callbacks()
callback.fd_callback = watch_fd_callback(wmiReactor.watchFileDescriptor)
callback.loop_callback = loop_callback(wmiReactor.doOnce)
eventContext = library.async_create_context(byref(callback))

from twisted.internet.main import installReactor
try:
    installReactor(wmiReactor)
except Exception, e:
    log.critical("Unable to install the WMI reactor. %s" % e)
    sys.exit(1) # TODO: pick a better error code?

class ActivityHook:
    "Notify the Samba event loop of file descriptor activity"

    def __init__(self, prefix, fd):
        self.prefix = prefix
        self.fd = fd

    def logPrefix(self):
        return self.prefix

    def doRead(self):
        library.zenoss_read_ready(eventContext, self.fd)

    def doWrite(self):
        library.zenoss_write_ready(eventContext, self.fd)

    def fileno(self):
        return self.fd

    def connectionLost(self, why):
        wmiReactor.removeFileDescriptor(self.fd)

# allow users to write:
#    from pysamba.twisted import reactor
# instead of
#    from twisted.internet import reactor
from twisted.internet import reactor

def setNTLMv2Authentication(enabled = False):
    """
    Enables or disables the NTLMv2 authentication feature.
    @param enabled: True if NTLMv2 is supported
    @type enabled: boolean
    """
    flag = "no"
    if enabled:
        flag = "yes"
    library.lp_do_parameter(-1, "client ntlmv2 auth", flag)
    log.debug("client ntlmv2 auth is now %s", flag)
