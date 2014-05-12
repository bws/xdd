#!/bin/env python

#
# Stupid python-isms to make code slightly more portable
#
from __future__ import print_function

#
# Import modules
#
import optparse
import os
import socket
import sys
import time
import xdd

#
# Time in seconds to wait between profiling runs
#
XDD_FILE_RETRY_DELAY = 5.0

class InvalidSpecError(Exception):
    """Spec was not of the format [[user@]host[,[user@]host]:]file"""
    pass

class CreateTransferManagerError(Exception):
    """File transfer failed during flow creation"""
    def __init__(self, reason):
        """Constructor"""
        self.reason = reason    

class CreateFlowTransferError(Exception):
    """File transfer failed during flow creation"""
    pass

class StartFlowTransferError(Exception):
    """File transfer failed during flow start"""
    pass

def createParser():
    """Create an option parser for xddmcp"""
    usage='usage: xddprof [Options] <path>'
    parser = optparse.OptionParser(usage=usage)
    parser.add_option('-a', '--resume', dest='resume',
                      action='store_true', default=False,
                      help='enable resume for partially completed profiles')
    parser.add_option('-o', '--output', dest='outfile',
                      action='store', type='string', metavar='outfile',
                      help='Write output to file')
    parser.add_option('-n', '--trials', dest='trials', 
                      action='store', type='int', default=1, metavar='N',
                      help='number of trials to run per configuration [Default: 1]')
    parser.add_option('-s', '--samples', dest='samples', 
                      action='store', type='int', default=1, metavar='N',
                      help='number of samples to measure per trial [Default: 1]')
    parser.add_option('-v', dest='verbose',
                      action='store_true', default=False,
                      help='create a log file')
    return parser

def createProfiler():
    p = Profiler()

    # If this is resuming an existing profile, just figure out what remains,
    # Otherwise, begin the profiling from the beginning
    
    # Add request sizes
    
def profileVolume():
    parser = createParser()
    (opts, args) = parser.parse_args()
    if 1 > len(args):
        print('ERROR: Directory to profile is required')
        parser.print_usage()
        return 1

    # Create and the profiler
    profiler = createProfiler()
    profiler.start()
    profiler.wait()
    return profiler.completionCode()

def main():
    """Main's only responsibility is to catch exceptions thrown by the VM"""
    try:
        rc = profileVolume()
    except KeyboardInterrupt:
        print('Transfer cancelled by user')
        rc = 1
        # TODO: remove before release
        raise
    return rc

# Execute main
if __name__ == "__main__":
    rc = main()
    sys.exit(rc)
