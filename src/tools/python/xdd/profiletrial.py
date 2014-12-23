
#
# Python stuff to improve portability
#
from __future__ import print_function

#
# Generic python packages used
#
import datetime
import fcntl
import getpass
import os
import select
import SocketServer
import socket
import subprocess
import sys
import threading
import time
from stat import *

#
# XDD packages
#
from xdd.constants import XDD_SINK_TO_SOURCE_DELAY
from xdd.core import XDDError

class TrialFailError(XDDError):
    """Report trial failures"""
    def __init__(reason):
        self.reason = reason

#
#
#
class ProfileTrial:
    """
    Profiler for profiling a storage volume
    """

    def __init__(self, target, reqsize, qdepth, dio,
                 order, access, preallocate, tlimit):
        """Constructor"""
        self._target = target
        self._reqsize = reqsize
        self._qdepth = qdepth
        self._dio = dio
        self._order = order
        self._access = access
        self._prealloc = preallocate
        self._tlimit = tlimit

    def run(self, logfile, isWrite, removeData):

        # Set the operation
        if isWrite:
            op = 'write'
        else:
            op = 'read'
            
        # Enable random access pattern
        access = ''
        if 'random' == access:
            access = '-seek random -seek range'
            
        # Enable dio if requested
        dio = ''
        if dio:
            dio = '-dio'
            
        # Convert numbers to strings
        rs = str(self._reqsize)
        qd = str(self._qdepth)
        tl = str(self._tlimit)
        nr = str(1024*1024)
        
        # Build the command
        cmd = ['xdd']
        cmd.extend(['-target', self._target])
        cmd.extend(['-op', op])
        cmd.extend(['-reqsize', '1'])
        cmd.extend(['-blocksize', rs])
        cmd.extend(['-qd', qd])
        cmd.extend([access, dio])
        cmd.extend(['-numreqs', nr])
        cmd.extend(['-timelimit', tl])
        cmd.extend(['-output', logfile])
        cmd.extend(['-stoponerror', '-minall'])
        
        # Execute the command
        p = subprocess.Popen(cmd,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        (out, err) = p.communicate()
        #if 0 != p.returncode:
        #    print('Xdd stdout:', out)
        #    print('Xdd stderr:', err)

        # Cleanup if requested
        if removeData:
            os.remove(self._target)
        
    def result(self, logfile):
        """@return (total bytes, total ops, total seconds)"""
        # Parse log using grep
        p = subprocess.Popen(['grep', 'COMBINED', logfile],
                             stdout=subprocess.PIPE)
        (out, err) = p.communicate()
        if 0 != p.returncode:
            print('grep results failed:', err)
            return None
        
        res = out.split()
        tbytes = int(res[4])
        tops = int(res[5])
        secs = float(res[6])
        return (tbytes, tops, secs)
