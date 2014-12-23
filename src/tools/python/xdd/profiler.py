
#
# Python stuff to improve portability
#
from __future__ import print_function

#
# Generic python packages used
#
import time
import os
from math import log10, floor

#
# XDD packages
#
from xdd.constants import XDD_SINK_TO_SOURCE_DELAY
from xdd.core import XDDError
from xdd.profiletrial import ProfileTrial
from xdd.profileparameters import ProfileParameters

class ProfileTrialError(XDDError):
    """Report trial failures"""
    def __init__(reason):
        self.reason = reason

#
#
#
class Profiler:
    """
    Profiler for profiling a storage volume
    """

    def __init__(self, logdir, volume, profParams, trials, samples, tlimit):
        """Constructor"""
        self.verbose = 0

        self._logdir = logdir
        self._volume = volume
        self._pp = profParams
        self._trials = trials
        self._samples = samples
        self._tlimit = tlimit
        self._results = []
        self._readId = 0
        self._writeId = 0

        # Output file names
        self._wlog = self._logdir + os.sep + 'writeperf.dat'
        self._rlog = self._logdir + os.sep + 'readperf.dat'

    def writeHeader(self, fname):
        """Write out the XDD profiling header"""
        header = '# XDD Profiler 0.0.1' + os.linesep
        header += '#' + os.linesep
        header += '# \t\t\t\tWorkload\t\t\t\t\t| Performance' + os.linesep
        header += '#' + os.linesep
        header += '# ID\t'
        header += 'Volume\t'
        header += 'Oper\t'
        header += 'Trial\t'
        header += 'Target\t'
        header += 'TLimit\t'
        header += 'Reqsz\t'
        header += 'Thrds\t'
        header += 'Direct\t'
        header += 'Order\t'
        header += 'Access\t'
        header += 'Bytes\t\t'
        header += 'TOPs\t'
        header += 'Time\t'
        header += 'IOPs\t'
        header += 'MB/s\t'
        header += 'GB/s\t'
        header += 'MiB/s\t'
        header += 'GiB/s\t'
        header += os.linesep
        header += '#' + os.linesep

        # Write out the header
        ofile = open(fname, 'w')
        ofile.write(header)
        ofile.close()
        
    def writeResults(self):
        """Write results to output files"""

        # Open file for write results
        wfile = open(self._wlog, 'a')
        rfile = open(self._rlog, 'a')

        # Write results
        for res in self._results:
            s = ''
            for r in res:
                s += r + '\t'
                
            # if res is a write result
            if 'write' == res[2]:
                wfile.write(s + os.linesep)
            else:
                rfile.write(s + os.linesep)

        wfile.close()
        rfile.close()

    def addResult(self, vol, oper, trial, target, tlimit, rsz, tcount,
                  dio, order, access, tbytes, tops, secs):
        """Add the result to the result list"""
        # Get the result id
        if 'write' == oper:
            id = self._writeId
            self._writeId += 1
        else:
            id = self._readId
            self._readId += 1
            
        # Add the descriptive fields
        res = [str(id), vol, oper, str(trial), target,
               str(tlimit), str(rsz), str(tcount), str(dio), order, access]

        # Add the performance fields
        res.extend([str(tbytes), str(tops), str(secs)])

        # Calculate the derived fields
        ftops = float(tops)
        fbytes = float(tbytes)
        iops = ftops/secs
        mbs = (fbytes/1000/1000)/secs
        gbs = (fbytes/1000/1000/1000)/secs
        mibs = (fbytes/1024/1024)/secs
        gibs = (fbytes/1024/1024/1024)/secs

        # Round to 4 significant digits
        riops = round(iops, -int(floor(log10(iops))) + 3)
        rmbs = round(mbs, -int(floor(log10(mbs))) + 3)
        rgbs = round(gbs, -int(floor(log10(gbs))) + 3)
        rmibs = round(mibs, -int(floor(log10(mibs))) + 3)
        rgibs = round(gibs, -int(floor(log10(gibs))) + 3)
        
        # Add the derived fields
        res.append(str(riops))
        res.append(str(rmbs))
        res.append(str(rgbs))
        res.append(str(rmibs))
        res.append(str(rgibs))

        # Update the results and flush to log
        self._results.append(res)
        self.writeResults()
        self._results = []
        
    def start(self):
        """Start the profiling trials"""
        # Create the logs
        self.writeHeader(self._wlog)
        self.writeHeader(self._rlog)

        # Profile storage
        for r in self._pp.reqsizes:
            for t in self._pp.qdepths:
                for d in self._pp.dios:
                    for o in self._pp.orders:
                        for a in self._pp.patterns:
                            for n in range(0, self._trials):
                                targetBase = 'ptarget-'
                                targetBase += str(r) + '-'
                                targetBase += str(t) + '-'
                                targetBase += str(d) + '-'
                                targetBase += str(o) + '-'
                                targetBase += str(a) + '-' + str(n)
                                target = str(self._volume) + os.sep \
                                         + targetBase + '.dat'
                                pt = ProfileTrial(target=target, reqsize=r,
                                                  qdepth=t, dio=d,
                                                  order=o, access=a,
                                                  preallocate=False,
                                                  tlimit=self._tlimit)

                                time.sleep(5)
                                wlog = self._logdir + os.sep \
                                       + targetBase + '-write.log'
                                pt.run(wlog, isWrite=True, removeData=False)
                                (tbytes, tops, secs) = pt.result(wlog)
                                self.addResult(vol=self._volume,
                                               oper='write',
                                               trial=n,
                                               target='ptarget',
                                               tlimit=self._tlimit,
                                               rsz=r,
                                               tcount=t,
                                               dio=d,
                                               order=o,
                                               access=a,
                                               tbytes=tbytes,
                                               tops=tops,
                                               secs=secs)

                                time.sleep(5)
                                rlog = self._logdir + os.sep + \
                                       targetBase + '-read.log'
                                pt.run(rlog, isWrite=False, removeData=True)
                                (tbytes, tops, secs) = pt.result(rlog)
                                self.addResult(vol=self._volume,
                                               oper='read',
                                               trial=n,
                                               target='ptarget',
                                               tlimit=self._tlimit,
                                               rsz=r,
                                               tcount=t,
                                               dio=d,
                                               order=o,
                                               access=a,
                                               tbytes=tbytes,
                                               tops=tops,
                                               secs=secs)
                            
    def wait(self):
        """Wait for profiling trial completion"""
        print('Write performance results logged:', self._wlog)
        print('Read performance results logged:', self._rlog)

    def completionCode(self):
        """@return the completion code for the profiling runs"""
        self.writeResults()
        return 1
        

