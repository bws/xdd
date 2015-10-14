
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

    def __init__(self, volumes, target, reqsize, qdepth, dio,
                 offset, order, pattern, alloc, tlimit, nbytes=None):
        """Constructor"""
        self._volumes = volumes
        self._target = target
        self._reqsize = reqsize
        self._qdepth = qdepth
        self._dio = dio
        self._offset = offset
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
        """Add the XDD command line to the log file"""
        lh = open(log, 'w')
        for c in cmd:
            lh.write(c + ' ')
        lh.close()

    def _createRandomWriteSeekListFile(self, fname, srange):
        """Create an XDD seek list file"""
        # XDD seek lists are in the following format:
        # ID ReqNum ReqSize Op Start Stop

        # First generate and shuffle the request numbers
        reqnums = range(0, srange)
        random.shuffle(reqnums)

        # Init the id counter
        rid = 0
        
        # Write the file (which may be quite large)
        fh = open(fname, 'w')
        for req in reqnums:
            line = str(rid) + ' '  + str(req) + ' '
            line += 'w 0 0' + os.linesep
            fh.write(line)
            rid += 1
        fh.close()

    def _createRandomReadSeekListFile(self, fname, wseekList, nreqs):
        """
        Convert a seek list full of writes to reads, and truncate it
        to just the number of requests desired
        """
        # Grab the first nreqs lines from the write seek list
        wfh = open(wseekList, 'r')
        lines = []
        entries = range(0, nreqs - 1)
        for e in entries:
            lines.append(wfh.readline())
        wfh.close()

        # Generate the read seek list
        rid = 0
        rfh = open(fname, 'w')
        for l in lines:
            reqnum = l.split()[1]
            out = str(rid) + ' ' + reqnum + ' r 0 0' + os.linesep
            rfh.write(out)
            rid += 1
        rfh.close()
        
    def run(self, logfile, isWrite, removeData):
        """Run the trial"""
        # Construct the targets
        assert(0 < len(self._volumes))
        targetargs = ['-targets', str(len(self._volumes))]
        for v in self._volumes:
            if 0 != len(v):
                target = v + os.sep + self._target
            else:
                target = self._target
            targetargs.append(target)
            
        # Set the operation
        if isWrite:
            op = 'write'
        else:
            op = 'read'

        # NOTE: Because the write may have finished due to the time
        # limit it is necessary to adjust the bytes to the smallest
        # actual file size on reads
        nbytes = self._nbytes
        if 'read' == op:
            nbytes = 0
            for target in targetargs[2:]:
                tsize = os.path.getsize(target)
                if nbytes == 0 or tsize < nbytes:
                    nbytes = tsize
                # If the offset is beyond the tsize, set it to 0
                if tsize < self._offset:
                    self._offset = 0
            
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
            ldir = os.path.dirname(logfile)
            wseekFile = ldir + os.sep + 'wseek'
            if isWrite:
                self._createRandomWriteSeekListFile(wseekFile, srange)
            else:
                rseekFile = ldir + os.sep + 'rseek'
                self._createRandomReadSeekListFile(rseekFile, wseekFile, srange)
            pattern = ['-seek', 'random', '-seek',
                       'range', str(srange)]

        # Configure the offset prior to size calcuations
        restartOffset = []
        if self._offset > 0:
            restartOffset = ['-restart', 'offset', str(self._offset)]
            
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
        cmd.extend(targetargs)
        cmd.extend(['-op', op])
        cmd.extend(['-reqsize', '1'])
        cmd.extend(['-blocksize', rs])
        cmd.extend(['-qd', qd])
        cmd.extend([dio])
        cmd.extend(pattern)
        cmd.extend(alloc)
        cmd.extend(['-bytes', nb])
        cmd.extend(restartOffset)
        cmd.extend(['-timelimit', tl])
        cmd.extend(['-output', logfile])
        cmd.extend(['-stoponerror'])

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
