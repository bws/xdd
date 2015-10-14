
#
# Python stuff to improve portability
#
from __future__ import print_function

#
# Generic python packages used
#
from math import log10, floor
import time
import os

#
# XDD packages
#
from xdd.constants import XDD_SINK_TO_SOURCE_DELAY
from xdd.core import XDDError
from xdd.profiletrial import ProfileTrial
from xdd.profileparameters import ProfileParameters
from xdd.profilevisualizer import ProfileVisualizer

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

    def __init__(self, logdir, volumes, profParams, trials, samples, tlimit,
                 nbytes):
        """Constructor"""
        self.verbose = 0

        self._logdir = logdir
        self._volumes = volumes
        self._pp = profParams
        self._trials = trials
        self._samples = samples
        self._tlimit = tlimit
        self._nbytes = nbytes
        self._results = []
        self._id = 0
        self._pause = min(5, tlimit)
        self._xddTargetPrefix = 'xddptrg'
        self._flushedResults = []
        
        # Output file names
        self._wlog = self._logdir + os.sep + 'writeprof.tsv'
        self._rlog = self._logdir + os.sep + 'readprof.tsv'

    def writeHeader(self, fname):
        """Write out the XDD profiling header"""
        header = '# XDD Profiler 0.0.1' + os.linesep
        header += '#' + os.linesep
        header += '# Volumes:'
        for v in self._volumes:
            header += ' ' + v
        header += os.linesep
        header += '#' + os.linesep
        header += '# \t\t\t\tWorkload\t\t\t\t\t\t\t| Performance' + os.linesep
        header += '#' + os.linesep
        header += '# ID\t'
        header += 'Numvols\t'
        header += 'Oper\t'
        header += 'Trial\t'
        header += 'Target\t'
        header += 'TLimit\t'
        header += 'Reqsz\t'
        header += 'Thrds\t'
        header += 'Direct\t'
        header += 'Issue\t'
        header += 'Pattern\t'
        header += 'Alloc\t'
        header += 'Bytes\t\t'
        header += 'TOPs\t'
        header += 'Secs\t'
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

    def addResult(self, vols, oper, trial, target, tlimit, rsz, tcount, dio,
                  order, pattern, alloc, tbytes, tops, secs):
        """Add the result to the result list"""
        # Add the descriptive fields
        res = [str(self._id/2), str(len(vols)), oper, str(trial), target,
               str(tlimit), str(rsz), str(tcount), str(dio),
               order, pattern, alloc]

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
        self._results.append(res)
        self._id += 1

        # Flust the results to logs
        self.writeResults()
        self._flushedResults.append(res)
        self._results = []
        
    def start(self):
        """Start the profiling trials"""
        # Create the logs
        self.writeHeader(self._wlog)
        self.writeHeader(self._rlog)

        # Profile storage
        for rs in self._pp.getReqSizes():
            for qd in self._pp.getQueueDepths():
                for dio in self._pp.getDios():
                    for order in self._pp.getOrders():
                        for pattern in self._pp.getPatterns():
                            for alloc in self._pp.getAllocs():
                                for off in self._pp.getOffsets():
                                    # If the personality, specifies a target, use it
                                    # otherwise create a target here
                                    target = self._pp.getTarget()
                                    if target is None:
                                        targetBase = self._xddTargetPrefix + '-'
                                        targetBase += str(rs) + '-'
                                        targetBase += str(qd) + '-'
                                        targetBase += str(dio) + '-'
                                        targetBase += str(order) + '-'
                                        targetBase += str(pattern) + '-'
                                        targetBase += str(alloc) + '-'
                                        targetBase += str(off) + '-'
                                        target = targetBase + '.dat'
                                    else:
                                        # Also need to use an empty volume
                                        self._volumes = ['']
                                        
                                    # Run the trials for the construct target
                                    for op in self._pp.getOpTypes():
                                        for n in range(0, self._trials):
                                            self.runTrial(target=target,
                                                          optype=op,
                                                          reqsize=rs,
                                                          qdepth=qd,
                                                          dio=dio,
                                                          order=order,
                                                          pattern=pattern,
                                                          alloc=alloc,
                                                          trial=n,
                                                          offset=off)
                                    # Cleanup only after all operations are complete
                                    if not self._pp.getKeepFiles():
                                        os.remove(target)
                                    self.postOperation()

    def runTrial(self, target, optype, reqsize, qdepth, dio,
                 order, pattern, alloc, trial, offset):
        """"""
        
        pt = ProfileTrial(volumes=self._volumes,
                          target=target, reqsize=reqsize,
                          qdepth=qdepth, dio=dio, offset=offset,
                          order=order, pattern=pattern,
                          alloc=alloc,
                          tlimit=self._tlimit,
                          nbytes=self._nbytes)

        if 'write' == optype:
            wlog = self._logdir + os.sep
            wlog += os.path.basename(target) + '-' + optype + '.log'
            pt.run(wlog, isWrite=True, removeData=False)
            result = pt.result(wlog)
        else:
            rlog = self._logdir + os.sep
            rlog += os.path.basename(target) + '-' + optype + '.log'
            pt.run(rlog, isWrite=False, removeData=False)
            result = pt.result(rlog)
            
        if result:
            (tbytes, tops, secs) = result
            self.addResult(vols=self._volumes,
                           oper=optype,
                           trial=trial,
                           target=self._xddTargetPrefix,
                           tlimit=self._tlimit,
                           rsz=reqsize,
                           tcount=qdepth,
                           dio=dio,
                           order=order,
                           pattern=pattern,
                           alloc=alloc,
                           tbytes=tbytes,
                           tops=tops,
                           secs=secs)
        else:
            print('ERROR: Trial run failed')

    def postOperation(self):
        self.writeResults()
        
    def wait(self):
        """Wait for profiling trial completion"""
        print('Write performance results logged:', self._wlog)
        print('Read performance results logged:', self._rlog)

    def completionCode(self):
        """@return the completion code for the profiling runs"""
        self.writeResults()
        return 1
        
    def produceGraphs(self):
        """Generate files that show profile data"""
        pv = ProfileVisualizer(self._flushedResults, self._logdir)
        pv.plotAll()

        
