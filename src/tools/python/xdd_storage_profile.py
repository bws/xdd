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
XDDPROF_GRAPHICAL_DEFAULT = True
XDDPROF_RESUME_DEFAULT = False
XDDPROF_NBYTES_DEFAULT = None
XDDPROF_NTRIALS_DEFAULT = 1
XDDPROF_NSAMPLES_DEFAULT = 1
XDDPROF_PERSONALITY_DEFAULT = 'default'
XDDPROF_TLIMIT_DEFAULT = 90
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
    parser.add_option('-g', '--graphical', dest='graphical',
                      action='store_true', default=XDDPROF_GRAPHICAL_DEFAULT,
                      help='produce graphical data')
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
    parser.add_option('-s', '--samples', dest='samples', 
                      action='store', type='int',
                      default=XDDPROF_NSAMPLES_DEFAULT, metavar='N',
                      help='number of samples to measure per trial [Default: 1]')
    parser.add_option('-t', '--tlimit', dest='tlimit', 
                      action='store', type='int',
                      default=XDDPROF_TLIMIT_DEFAULT, metavar='N',
                      help='time limit in seconds for each trial [Default: ' \
                      + str(XDDPROF_TLIMIT_DEFAULT) + ']')
    parser.add_option('-v', dest='verbose',
                      action='store_true',
                      default=XDDPROF_VERBOSE_DEFAULT,
                      help='create a log file')
    return parser

def createProfiler(outdir, paths, nbytes, tlimit, numTrials, numSubsamples,
                   personality, resume, verbose):
    """ """
    # If this is resuming an existing profile, just figure out what remains,
    # Otherwise, begin the profiling from the beginning
    if resume:
        print('Warning: Resume not yet supported')

    # Create a profiling parameter set
    pp = ProfileParameters.create(personality)
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
                              nbytes=opts.nbytes,
                              tlimit=opts.tlimit,
                              numTrials=opts.trials,
                              numSubsamples=opts.samples,
                              personality=opts.personality,
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
