from ctypes import *

from pysamba.twisted.reactor import reactor, eventContext
from pysamba.library import *
from pysamba.wbem.wbem import *
from pysamba.wbem.Query import Query
from pysamba.twisted.callback import WMIFailure
import sys

import logging
log = logging.getLogger('p.t.watcher')

import Globals
from Products.ZenUtils.Driver import drive

from twisted.internet import defer

wql = """SELECT * FROM __InstanceCreationEvent where """\
      """TargetInstance ISA 'Win32_NTLogEvent' """\
      """and TargetInstance.EventType <= 5"""

def async_sleep(secs):
    d = defer.Deferred()
    reactor.callLater(secs, d.callback, None)
    return d

def printSize():
    "Monitor this process' memory usage"
    import gc
    gc.collect()
    sz = open('/proc/%d/statm' % os.getpid()).read().split()[0]
    log.info('*'*40)
    log.info("Current size: %s" % (sz,) )

def doOneDevice(creds, hostname):
    def inner(driver):
        try:
            q = Query()
            yield q.connect(eventContext, hostname, creds)
            driver.next()
            yield q.notificationQuery(wql)
            result = driver.next()
            log.info("Query sent")
            while 1:
                printSize()
                try:
                    class_name = ''
                    while 1:
                        yield result.fetchSome(500)
                        if not driver.next(): break
                        log.info("Got %d items", len(driver.next()))
                        for obj in driver.next():
                            obj = obj.targetinstance
                            props = [p for p in obj.__dict__.keys()
                                     if not p.startswith('_')]
                            if obj._class_name != class_name:
                                class_name = obj._class_name
                                print obj._class_name
                                print repr(props)
                            print repr([getattr(obj, p) for p in props])
                except WError, ex:
                    log.exception(ex)
                    yield async_sleep(1)
                    driver.next()
            q.close()
        except Exception, ex:
            log.exception(ex)
    return drive(inner)

def main():
    logging.basicConfig()
    log = logging.getLogger()
    log.setLevel(20)
    # DEBUGLEVEL.value = 99

    creds = sys.argv[1]
    hosts = sys.argv[2:]

    def stop(result):
        print result
        reactor.stop()
    def later():
        d = defer.DeferredList(
            [doOneDevice(creds, h) for h in hosts]
            )
        d.addBoth(stop)
    reactor.callLater(1, later)
    reactor.run()

sys.exit(main())
