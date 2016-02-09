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
# Time in seconds to wait between failed transfer retries
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
    usage='usage: xddmcp [Options] [host[,host]:]sfile [host:]dfile'
    parser = optparse.OptionParser(usage=usage)
    parser.add_option('-a', '--resume', dest='resume',
                      action='store_true', default=False,
                      help='enable resume for partially completed transfers')
    parser.add_option('-b', dest='bs',
                      action='store', type='int', default=8192,
                      help='the size of a disk/network block in KiB')
    parser.add_option('-d', dest='dio', 
                      action='store', type='choice', choices=['s', 'd', 'b'],
                      metavar='s|d|b',
                      help='use direct I/O for the specified side (s for source, d for dest, b for both sides)')
    parser.add_option('-o', dest='ordered',
                      action='store', type='choice', choices=['s', 'd', 'b'],
                      metavar='s|d|b',
                      help='use serial ordering for the specified side (s for source, d for dest, b for both sides)')
    parser.add_option('-n', '--retry', dest='retries', 
                      action='store', type='int', default=0, metavar='N',
                      help='on failure, retry up to N times [Default: 0]')
    parser.add_option("-p", "--port", dest='port',
                      action='store', type='int', default=40010,
                      help='set the first listen port [Default: 40010]')
    parser.add_option('-r', dest='recursive', 
                      action='store_true', default=True,
                      help='copy files recursively [Default: On]')
    parser.add_option('-t', dest='threads',
                      action='store', type='int', default=8,
                      help='set the total number of parallel streams [Default: 8]')
    parser.add_option('-s', '--size', dest='size',
                      action='store', type='int', default=None,
                      help='number of source device bytes to transfer')
    parser.add_option('-v', dest='verbose',
                      action='store_true', default=False,
                      help='create a log file')
    parser.add_option('-V', dest='verbose2',
                      action='store_true', default=False,
                      help='add timestamp output to log')
    return parser

def parseSpec(spec):
    """
    Parse spec.  The current spec format is:
    [[user@]host[,[user@]host]:]file

    @return a tuple with a list of hosts and the filename
    """
    # Return entry defaults
    userHostTuples = []
    filename = ''

    # First split at the colon to get the hosts and files
    parts = spec.split(':')
    if 1 == len(parts):
        filename = spec
    elif 2 == len(parts):
        hspec = parts[0]
        filename = parts[1]
        # Second, split hosts at comma
        hlist = hspec.split(',')
        for h in hlist:
            # Finally, split at @ to get user and host
            uh = h.split('@')
            if 1 == len(uh) and 0 != len(h):
                userHostTuples.append( (None, h) )
            elif 2 == len(uh) and 0 != len(uh[0]) and 0 != len(uh[1]):
                userHostTuples.append( (uh[0], uh[1]) )
            else:
                raise InvalidSpecError
    else:
        raise InvalidSpecError

    # Finally, make sure what we have makes sense
    if 0 == len(filename):
        raise InvalidSpecError

    return (userHostTuples, filename)

def partitionThreads(total, buckets):
    """
    @return an array of the number of threads for each bucket
    """
    parts = []
    while len(parts) < buckets:
        parts.append(total / buckets)
        rem = total % buckets
        if len(parts) <= rem:
            parts[-1] += 1
    return parts

def createLogFile(base):
    """
    @return the name of the logfile, or None if creation failed
    """
    name = base + '-' + str(int(time.time())) + '.log'
    try:
        fd = os.open(name, os.O_CREAT|os.O_EXCL|os.O_WRONLY, 0644)
        os.write(fd, base)
        for a in sys.argv:
            os.write(fd, ' ' + a)
        os.write(fd, os.linesep)
        os.close(fd)
    except OSError:
        print('WARNING:  Log file creation failed for: ' + name)
        name = None
    return name

def createTransferManager(src, dest, opts, logfilename):
    """
    Use the supplied command line arguments to create a TransferManager
    """
    # Create the TransferManager
    transferMgr = xdd.TransferManager()

    # Add options
    rs = opts.bs * 1024
    transferMgr.setRequestSize(rs)
    if opts.size is not None:
        transferMgr.setTransferSize(opts.size)
    transferMgr.setRestartFlag(opts.resume)
    if opts.verbose and logfilename is not None:
        transferMgr.setVerbosity(1, logfilename)

    # Unpack sources and destinations
    (destTuples, dfile) = dest
    (sourceTuples, sfile) = src

    # Add the targets
    dioFlag = False
    serialFlag = False
    if 's' == opts.dio or 'b' == opts.dio:
        dioFlag=True
    if 's' == opts.ordered or 'b' == opts.ordered:
        serialFlag=True
    transferMgr.setSourceTarget(sfile, dioFlag=dioFlag, serialFlag=serialFlag)
    dioFlag = False
    serialFlag = False
    if 'd' == opts.dio or 'b' == opts.dio:
        dioFlag=True
    if 'd' == opts.ordered or 'b' == opts.ordered:
        serialFlag=True
    transferMgr.setSinkTarget(dfile, dioFlag=dioFlag, serialFlag=serialFlag)

    # Add the destination host
    destUser = None
    destHost = None
    if not destTuples:
        destHost = 'localhost'
        hostname = 'localhost'
    else:
        (user, hostname) = destTuples[0]
        # Convert to IP to avoid DNS round-robin issues
        if 'localhost' == hostname:
            destHost = hostname
        else:
            try:
                destHost = socket.gethostbyname(hostname)
            except socket.gaierror:
                e = CreateTransferManagerError("Cannot resolve " + hostname)
                raise e

    transferMgr.addSink(destUser, destHost, hostname, opts.threads, [], opts.port)

    # Add the source hosts
    if not sourceTuples:
        transferMgr.addSource(None, 'localhost', 'localhost', opts.threads, [destHost], opts.port)
    else:
        # Partition the threads among the sources
        sourceThreads = partitionThreads(opts.threads, len(sourceTuples))

        # Add each source
        for s in sourceTuples:
            (user, host) = s
            # Convert to IP to avoid DNS round-robin issues
            if 'localhost' == host:
                srcHost = host
            else:
                try:
                    srcHost = socket.gethostbyname(host)
                except socket.gaierror:
                    e = CreateTransferManagerError("Cannot resolve " + host)
                    raise e
            threads = sourceThreads.pop()
            transferMgr.addSource(user, srcHost, host, threads, [destHost], opts.port)
            opts.port += threads

    return transferMgr


def performTransfer(transferMgr, retryLimit):
    """
    Perform the transfer and print out monitoring information.
    """
    # Handle retries here
    rc = 0
    for count in range(0, 1 + retryLimit):
        if count > 0:
            print("INFO: Retrying failed transfer")
            time.sleep(XDD_FILE_RETRY_DELAY)
        try:
            # Create all remote flow elements
            rc = transferMgr.createEndpoints()
            if 0 != rc:
                raise CreateFlowTransferError("Failed contacting hosts")
            try:
                # Start and monitor the transfers
                rc = transferMgr.startAndMonitorTransfers(1)
                if 0 != rc:
                    raise StartFlowTransferError('Failed starting XDD')
                else:
                    break
            finally:
                pass
        except:
            rc = 1
            print('Unexpected error:', sys.exc_info()[0])
            #raise
    transferMgr.cleanupEndpoints()
    return rc

def transferFiles():
    """
    Parse command line options and use the XDD package to move file data
    requested by user
    @return 0 on success, non-zero on failure
    """
    
    rc = 0
    parser = createParser()
    (opts, args) = parser.parse_args()
    if 2 > len(args):
        print('ERROR: Source and destination filenames are required')
        parser.print_usage()
        return 1

    # Convert arguments in sources and destination
    i = 0
    sources = []
    try:
        for spec in args:
            if len(sources) < len(args) - 1:
                sources.append( parseSpec(spec))
        else:
            dest = parseSpec(spec)
    except InvalidSpecError:
        print('ERROR: Invalid spec:', spec)
        parser.print_usage()
        return 1
                
    # Destination serial order is only allowed for single sources
    for src in sources:
        (shosts,_) = src
        if shosts and 1 < len(shosts) and ('b' == opts.ordered or 'd' == opts.ordered):
            print('ERROR: Destination ordering is not allowed with multiple sources') 
            return 1

    # For now, only a single destination host is allowed
    (dhosts, _) = dest
    if dhosts and 1 < len(dhosts):
        print('ERROR: Only one destination host currently allowed') 
        parser.print_usage()
        return 1

    # Require 1 thread per source
    for src in sources:
        (shosts, _) = src
        if len(shosts) > opts.threads:
            print('WARNING: Setting thread count to', len(shosts))
            opts.threads = len(shosts)

    # Maximum number of threads is 124
    if opts.threads > 124:
        print('ERROR: Thread count must be less than 124')
        return 1

    # Create the logfile if requested
    logfilename = None
    if opts.verbose:
        logfilename = createLogFile('xddmcp')

    # Perform a transfer for each source
    try:
        for src in sources:
            tmgr = createTransferManager(src, dest, opts, logfilename)
            rc = performTransfer(tmgr, opts.retries)
    except CreateTransferManagerError as e:
        print('ERROR: ' + e.reason)        
    except CreateFlowTransferError:
        print('ERROR: Remote process creation failed.')
    except StartFlowTransferError:
        (_, sf) = src
        (_, df) = dest
        print('ERROR: Failed transferring', sf, 'to', df)
        rc = 2

    return rc

def main():
    """Main's only responsibility is to catch exceptions thrown by the VM"""
    try:
        rc = transferFiles()
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
