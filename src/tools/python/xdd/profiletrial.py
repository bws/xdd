
#
# Python stuff to improve portability
#
from __future__ import print_function

#
# Generic python packages used
#
import os
import random
import subprocess
import time

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
                 order, pattern, alloc, tlimit, nbytes=None):
        """Constructor"""
        self._target = target
        self._reqsize = reqsize
        self._qdepth = qdepth
        self._dio = dio
        self._ordering = order
        self._pattern = pattern
        self._alloc = alloc
        self._tlimit = tlimit
        self._nbytes = nbytes
        
        # If nbytes is not provided, guess
        if not self._nbytes:
            limit = 16 * 1024 * 1024 * 1024
            hueristic = reqsize * 1024 * 1024
            self._nbytes = min (limit, hueristic)

    def _logCommand(self, log, cmd):
        lh = open(log, 'w')
        for c in cmd:
            lh.write(c + ' ')
        lh.close()
        
    def run(self, logfile, isWrite, removeData):

        # Set the operation
        if isWrite:
            op = 'write'
        else:
            op = 'read'

        # NOTE: Because the write may have short-circuited due to the time
        # limit it is necessary to adjust the bytes to the actual file size
        # on reads
        nbytes = self._nbytes
        if 'read' == op:
            nbytes = os.path.getsize(self._target)
            
        # Enable dio if requested
        dio = ''
        if dio:
            dio = '-dio'
            
        # Convert numbers to strings
        rs = str(self._reqsize)
        qd = str(self._qdepth)
        tl = str(self._tlimit)
        nb = str(nbytes)

        # Enable access pattern
        pattern = ''
        if 'random' == self._pattern:
            srange = nbytes / self._reqsize
            slist = range(0, srange)
            random.shuffle(slist)
            pattern = ['-seek', 'random', '-seek',
                       'range', str(srange)]

        # Configure ordering
        if 'loose' == self._ordering:
            order = '-looseordering'
        elif 'serial' == self._ordering:
            order = '-serialordering'
        else:
            order = '-noordering'
        
        # Enable allocation
        alloc = ''
        if 'pre' == self._alloc:
            alloc = ['-preallocate', nb]
        elif 'trunc' == self._alloc:
            alloc = ['-pretruncate', nb]
            
        # Build the command
        cmd = ['xdd']
        cmd.extend(['-target', self._target])
        cmd.extend(['-op', op])
        cmd.extend(['-reqsize', '1'])
        cmd.extend(['-blocksize', rs])
        cmd.extend(['-qd', qd])
        cmd.extend([dio])
        cmd.extend(pattern)
        cmd.extend(alloc)
        cmd.extend(['-bytes', nb])
        cmd.extend(['-timelimit', tl])
        cmd.extend(['-output', logfile])
        cmd.extend(['-stoponerror', '-minall'])

        # Write the command to the log for posterity
        self._logCommand(logfile, cmd)
        
        # Execute the command
        p = subprocess.Popen(cmd,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        (out, err) = p.communicate()

        if 0 != p.returncode:
            print('Xdd return code:', p.returncode)
            print('Xdd stdout:', out)
            print('Xdd stderr:', err)

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
