#! /usr/bin/env python
import sys
import os
version = '.'.join(map(str, sys.version_info[:2]))
print os.path.join(sys.prefix, 'include', 'python' + version)

