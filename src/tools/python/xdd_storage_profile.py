#!/bin/env python

#
# Stupid python-isms to make code slightly more portable
#
from __future__ import print_function

#
# Import modules
#
import datetime
import optparse
import os
import socket
import sys
import time
import xdd
from xdd.profileparameters import ProfileParameters

#
# Time in seconds to wait between profiling runs
#
XDDPROF_OUTDIR_DEFAULT = 'xddprof-' + datetime.datetime.now().strftime('%Y%m%d%H%M%S')
XDDPROF_DIRECTIO_DEFAULT = None
XDDPROF_FILE_MODE_DEFAULT = False
XDDPROF_GRAPHICAL_DEFAULT = False
XDDPROF_RESUME_DEFAULT = False
XDDPROF_NBYTES_DEFAULT = None
XDDPROF_NTRIALS_DEFAULT = 1
XDDPROF_NSAMPLES_DEFAULT = 1
XDDPROF_OFFSET_DEFAULT = None
XDDPROF_ORDER_DEFAULT = 'serial'
XDDPROF_PERSONALITY_DEFAULT = 'default'
XDDPROF_TLIMIT_DEFAULT = 90
XDDPROF_THREADS_DEFAULT = None
XDDPROF_VERBOSE_DEFAULT = False

def createParser():
    """Create an option parser for xddmcp"""
    usage='usage: xddprof [Options] <path>'
    parser = optparse.OptionParser(usage=usage)
    parser.add_option('-a', '--resume', dest='resume',
                      action='store_true', default=XDDPROF_RESUME_DEFAULT,
                      help='enable resume for partially completed profiles')
    parser.add_option('-b', '--bytes', dest='nbytes', 
                      action='store', type='int',
                      default=XDDPROF_NBYTES_DEFAULT, metavar='N',
                      help='number of bytes to read or write')
    parser.add_option('-d', '--directio', dest='directio', 
                      action='store_true', default=XDDPROF_DIRECTIO_DEFAULT,
                      metavar='N',
                      help='profile only direct I/O')
    parser.add_option('-e', '--order', dest='order', 
                      action='store', type='string',
                      default=XDDPROF_DIRECTIO_DEFAULT, metavar='N',
                      help='profile only the ordering provided (i.e. serial, loose, or none)')
    parser.add_option('-f', '--file-mode', dest='filemode',
                      action='store_true', default=XDDPROF_FILE_MODE_DEFAULT,
                      help='profile the specified file rather than a directory')
    parser.add_option('-g', '--graphical', dest='graphical',
                      action='store_true', default=XDDPROF_GRAPHICAL_DEFAULT,
                      help='produce graphical data')
    parser.add_option('-k', '--keep', dest='keep', 
                      action='store_true',
                      default=False, metavar='N',
                      help='one of r, w, or rw [Default: rw]')
    parser.add_option('-m', '--mode', dest='mode', 
                      action='store', type='string',
                      default=None, metavar='N',
                      help='one of r, w, or rw [Default: rw]')
    parser.add_option('-n', '--trials', dest='trials', 
                      action='store', type='int',
                      default=XDDPROF_NTRIALS_DEFAULT, metavar='N',
                      help='number of trials to run per configuration [Default: 1]')
    parser.add_option('-o', '--output', dest='outdir',
                      action='store', type='string',
                      default=XDDPROF_OUTDIR_DEFAULT, metavar='outdir',
                      help='Write output to file')
    parser.add_option('-p', '--personality', dest='personality',
                      action='store', type='string',
                      default=XDDPROF_PERSONALITY_DEFAULT, metavar='personality',
                      help='Select storage personality [Default: ' \
                      + XDDPROF_PERSONALITY_DEFAULT + ']')
    parser.add_option('-r', '--reqsize', dest='reqsize', 
                      action='store', type='int',
                      default=None, metavar='N',
                      help='request size in bytes')
    parser.add_option('-s', '--samples', dest='samples', 
                      action='store', type='int',
                      default=XDDPROF_NSAMPLES_DEFAULT, metavar='N',
                      help='number of samples to measure per trial [Default: 1]')
    parser.add_option('-l', '--tlimit', dest='tlimit', 
                      action='store', type='float',
                      default=XDDPROF_TLIMIT_DEFAULT, metavar='N',
                      help='time limit in seconds for each trial [Default: ' \
                      + str(XDDPROF_TLIMIT_DEFAULT) + ']')
    parser.add_option('-t', '--threads', dest='threads', 
                      action='store', type='int',
                      default=XDDPROF_THREADS_DEFAULT, metavar='N',
                      help='number of threads to use [Default: ' \
                      + str(XDDPROF_THREADS_DEFAULT) + ']')
    parser.add_option('-T', '--offset', dest='offset', 
                      action='store', type='int',
                      default=XDDPROF_OFFSET_DEFAULT, metavar='N',
                      help='offset to use [Default: 0]')
    parser.add_option('-v', dest='verbose',
                      action='store_true',
                      default=XDDPROF_VERBOSE_DEFAULT,
                      help='create a log file')
    return parser

def createProfiler(outdir, paths, personality, filemode, keepfiles, mode, directio, order,
                   reqsize, offset, threads, nbytes, tlimit, numTrials, numSubsamples,
                   resume, verbose):
    """ """
    # First create a profiling parameter set based on the supplied
    # personality
    pp = ProfileParameters.create(personality)

    # Then override the values with those on the command line
    if 'r' == mode:
        pp.setReadOnly()
    elif 'w' == mode:
        pp.setWriteOnly()
    elif 'rw' == mode or 'wr' == mode:
        pp.setReadWrite()

    if reqsize:
        pp.setReqsize(reqsize)
    if offset:
        pp.setOffset(offset)    
    if threads:
        pp.setQueueDepth(threads)
    if filemode:
        pp.setTarget(paths[0])
    if directio:
        pp.setDIO()
    if keepfiles:
        pp.setKeepFiles(True)    

    if order == 'loose':
        pp.setOrderLoose()
    elif order == 'none':
        pp.setOrderNone()
    elif order == 'serial':
        pp.setOrderSerial()

    # Use the personality to create a profiler
    p = xdd.Profiler(outdir, paths, pp, numTrials, numSubsamples, tlimit,
                     nbytes)

    # Set the verbosity
    if verbose:
        p.verbosity = 1

    # Create the log dir if necessary
    if not os.path.exists(outdir):
        os.mkdir(outdir)
        
    return p

def profileVolume():
    """ """
    parser = createParser()
    (opts, args) = parser.parse_args()
    if 1 > len(args):
        print('ERROR: Directory to profile is required')
        parser.print_usage()
        return 1

    # Profile each supplied directory as a single entity
    rc = 0
    paths = args

    # Create the profiler and run it
    profiler = createProfiler(outdir=opts.outdir,
                              paths=paths,
                              personality=opts.personality,
                              filemode=opts.filemode,
                              mode=opts.mode,
                              keepfiles=opts.keep,
                              nbytes=opts.nbytes,
                              directio=opts.directio,
                              order=opts.order,
                              reqsize=opts.reqsize,
                              offset=opts.offset,
                              threads=opts.threads,
                              tlimit=opts.tlimit,
                              numTrials=opts.trials,
                              numSubsamples=opts.samples,
                              resume=opts.resume,
                              verbose=opts.verbose)
    profiler.start()
    profiler.wait()

    # Generate graphical data if requested
    if opts.graphical:
        profiler.produceGraphs()

    rc += profiler.completionCode()
        
    return rc 

def main():
    """Main's only responsibility is to catch exceptions thrown by the VM"""
    try:
        rc = profileVolume()
    except KeyboardInterrupt:
        print('Transfer cancelled by user')
        rc = 1
        # TODO: remove raise before release
        raise
    return rc

# Execute main
if __name__ == "__main__":
    rc = main()
    sys.exit(rc)
