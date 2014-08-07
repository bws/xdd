
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
from xdd.core import XDDError, TransferPreconditionError
from xdd.factory import EndpointFactory

class TransferFailError(XDDError):

    def __init__(source, dest, reason):
        self.source = source
        self.dest = dest
        self.reason = reason

#
# Interface classes to interact with local and remote XDD flows
#
class TransferManager:
    """
    Manager for creating, monitoring, and deallocating local and remote flows.
    """

    def __init__(self):
        """Constructor"""
        self.isCreated = False
        self.isStarted = False
        self.isComplete = False
        self.isSuccess = False
        self.factory = None

        # Temporary variables used for showing progress
        self.beginTime = None
        self.currentSourceFile = None
        self.currentSinkFile = None
        self.currentFlowSize = 0
        self.currentRestartOffset = 0
        self.verboseLog = None

        self.requestSize = 0
        self.transferSize = None
        self.restartFlag = False
        self.sinkDIOFlag = False
        self.sinkSerialFlag = False
        self.sinkTimestampFlag = False
        self.sinkVerboseFlag = False
        self.sinkTarget = None
        self.sinkXddPath = ''
        self.sinks = []
        self.sourceDIOFlag = False
        self.sourceSerialFlag = False
        self.sourceVerboseFlag = False
        self.sourceTimestampFlag = False
        self.sourceTarget = None
        self.sourceXddPath = ''
        self.sources = []


    def setRequestSize(self, reqSize):
        """Set the chunk size of data to move across the wire"""
        self.requestSize = reqSize
        
    def setTransferSize(self, transferSize):
        """Set the size of data to move across the wire"""
        self.transferSize = transferSize
        
    def setRestartFlag(self, restartFlag):
        """Set the amount of data previously sent across the wire"""
        self.restartFlag = restartFlag
        
    def setSinkTarget(self, target, dioFlag=False, serialFlag=False):
        """ Set the name of of the sink target file"""
        assert not self.isCreated
        self.sinkTarget = target
        self.sinkDIOFlag = dioFlag
        self.sinkSerialFlag = serialFlag

    def setSourceTarget(self, target, dioFlag=False, serialFlag=False):
        """Set the name of the source target file"""
        assert not self.isCreated
        self.sourceTarget = target
        self.sourceDIOFlag = dioFlag
        self.sourceSerialFlag = serialFlag

    def setVerbosity(self, verboseLevel, filename):
        """Set the level of verbosity"""
        if 1 <= verboseLevel:
            self.sinkVerboseFlag = True
            self.sourceVerboseFlag = True
        if 2 <= verboseLevel:
            self.sinkTimestampFlag = True
            self.sourceTimestampFlag = True
        self.verboseLog = filename

    def addSink(self, user, hostIP, hostname, threads, ifs = [], port = 40010):
        """Add a sink to the list of sinks"""
        assert not self.isCreated
        assert hostIP
        assert hostname
        assert 0 < threads
        sink = {'ip': hostIP, 'hostname': hostname, 'threads': threads, 'port': port, 'ifs': []}
        if 0 == len(ifs):
            sink['ifs'].append(hostIP)
        else:
            sink['ifs'].extend(ifs)

        self.sinks.append(sink)

    def addSource(self, user, hostIP, hostname, threads, ifs = [], port = 40010):
        """Add a source to the list of sources"""
        assert not self.isCreated
        assert hostIP
        assert hostname
        assert 0 < threads
        source = {'ip': hostIP, 'hostname': hostname, 'threads': threads, 'port': port, 'ifs': []}
        if 0 == len(ifs):
            source['ifs'].append(hostIP)
        else:
            source['ifs'].extend(ifs)

        self.sources.append(source)

    def setSinkXddPath(self, path):
        """Set the path sinks use to find xdd"""
        self.sinkXddPath = path

    def setSourceXddPath(self, path):
        """Set the path sources use to find xdd"""
        self.sourceXddPath = path

    def setXddPath(self, path):
        """Set the path used to find xdd"""
        self.sinkXddPath = path
        self.sourceXddPath = path

    def performPostCreateChecks(self):
        """@return 0 if all checks pass, otherwise non-zero"""
        rc = 0
        # Check that all of the XDD's are the same version
        base = None
        current = None
        for f in self.factory.getEndpoints():
            current = f.protocolVersion()
            if base is None:
                base = current
            if current is None or not current or current != base:
                print("ERROR: XDD Protocols do not match", file=sys.stderr)
                rc = 1
                break
            

        # Print a generic warning if the sinks do not have pre-allocation
        # support
        for s in self.factory.getSinkEndpoints():
            if not s.hasPreallocateAvailable():
                print("WARNING: XDD preallocation support not available", 
                      file=sys.stderr)
        return rc

    def createEndpoints(self):
        """Create local and remote transfer host endpoints"""
        assert not self.isCreated
        assert 0 < self.requestSize
        assert 0 < len(self.sources)
        assert 0 < len(self.sinks)
        assert self.sinkTarget
        assert self.sourceTarget
        self.factory = EndpointFactory(self.requestSize,
                                       self.sourceDIOFlag, 
                                       self.sourceSerialFlag, 
                                       self.sourceVerboseFlag, 
                                       self.sourceTimestampFlag,
                                       self.sourceXddPath,
                                       self.sources,
                                       self.sinkDIOFlag, 
                                       self.sinkSerialFlag, 
                                       self.sinkVerboseFlag, 
                                       self.sinkTimestampFlag,
                                       self.sinkXddPath,
                                       self.sinks)
        rc = self.factory.createEndpoints()
        if 0 == rc:
            self.isCreated = True
            rc = self.performPostCreateChecks()
        return rc

    def createDir(self, sourceDir, targetDir):
        """Create the directory targetDir on the sink"""
        assert self.isCreated
        self.beginTime = time.time()
        self.currentFlowSize = 0
        sink = self.factory.getSinkEndpoints()[0]
        rc = sink.createDirectory(targetDir)
        if 0 == rc:
            self.showProgress(sourceDir, 0)
            print()
        return rc

    def createSymlink(self, sourceLink, targetLink, targetTarget):
        """Create the directory targetDir on the sink"""
        assert self.isCreated
        self.beginTime = time.time()
        self.currentFlowSize = 0
        sink = self.factory.getSinkEndpoints()[0]
        rc = sink.createSymlink(targetTarget, targetLink)
        if 0 == rc:
            self.showProgress(sourceLink, 0)
            print()
        return rc

    def startTransfer(self, sourceFile, sinkFile):
        """Start all flows in correct order"""
        assert self.isCreated
        rc = 0
        self.currentSourceFile = sourceFile
        self.currentSinkFile = sinkFile
        self.currentRestartOffset = 0

        # If needed get a transfer size
        if not self.transferSize:
            s = self.factory.getSourceEndpoints()[0]
            transferSize = s.getFileSize(sourceFile)
        else:
            transferSize = self.transferSize

        # Set a restart offset
        restartOffset = 0
        if self.restartFlag:
            s = self.factory.getSinkEndpoints()[0]
            restartOffset = s.getRestartOffset(sinkFile)
            self.currentRestartOffset = restartOffset

        # Start sinks first
        sinks = self.factory.getSinkEndpoints()
        for s in sinks:
            r = s.startFlow(sinkFile, transferSize, 
                            self.restartFlag, restartOffset)
            if 0 != r:
                print('Sink flow failed during startup', s.errorString())
                rc += 1

        # Pause between sink and source startups
        time.sleep(XDD_SINK_TO_SOURCE_DELAY)

        # Then start sources
        sources = self.factory.getSourceEndpoints()
        for s in sources:
            r = s.startFlow(sourceFile, transferSize, 
                            self.restartFlag, restartOffset)
            if 0 != r:
                print('Source flow failed during startup', s.errorString())
                rc += 1

        # Update the log with the execute commands
        self.writeLog(isWriteCommand=True)

        # If all went well, set state and start timer
        if 0 == rc:
            self.isStarted = True
            self.beginTime = time.time()
            self.currentFlowSize = transferSize
        return rc

    def monitorTransfer(self, monitorInterval):
        """
        Monitor the current flows.  Print a progress report to the screen
        and update the logfile if necessary.
        """
        assert self.isStarted
        endpoints = self.factory.getEndpoints()
        completedBytes = 0
        completeCount = 0
        failureCount = 0
        while completeCount < len(endpoints) and 0 == failureCount:
            monBegin = time.time()
            completedBytes = 0
            completeCount = 0
            for e in endpoints:
                # Check for completions and failures
                if e.completionStatus() is not None:
                    completeCount += 1
                    if 0 != e.completionStatus():
                        failureCount += 1
                # Get the progress
                currentBytes = e.currentByte()
                if 0 < currentBytes:
                    completedBytes += currentBytes
            
            # The flows don't really report exactly correct bytes yet, so
            # modify the completed bytes to be correct, this is due to a couple
            # things, sinks report the restart offset and multi-source transfers
            # use -startoffset rather than -restart offset
            correctedBytes = completedBytes - self.currentRestartOffset
            if len(self.sources) > 1:
                # Progress temporarily reports as len(sources) * restartOffset
                # But then shrinks as the actual heartbeats get reported
                initialVal = len(self.sources) * self.currentRestartOffset
                if correctedBytes == initialVal:
                    correctedBytes = self.currentRestartOffset
                else:
                    correctedBytes += self.currentRestartOffset

            # Print a status message
            self.showProgress(self.currentSourceFile, correctedBytes)
            self.writeLog()
            monEnd = time.time()

            # Sleep until its time to monitor again
            interval = monEnd - monBegin
            if interval < monitorInterval:
                time.sleep(monitorInterval - interval)

        # Get all of the remaining log output
        self.writeLog(flushAll=True)

        # Determine the outcome of transfer
        failures = 0
        for e in endpoints:
            if e.completionStatus() is None:
                e.cancelFlow()
            elif 0 != e.completionStatus():
                failures += 1
                print('RC:', e.completionStatus())
                print('ERROR:', e.errorString())
            
        # If the completion status for all endpoints is success, print out
        # a final progress message and remove the restart cookie
        if 0 == failures:
            self.currentRestartOffset = 0
            self.showProgress(self.currentSourceFile, self.currentFlowSize)
            print('')
            if self.restartFlag:
                for s in self.factory.getSinkEndpoints():
                    # Ignore the return code on purpose
                    rc = s.removeRestartCookie(self.currentSinkFile)

        return failures

    def startAndMonitorTransfers(self, monitorInterval):
        """
        Used to transfer and monitor a directory or multiple files
        as part of a single transfer
        """
        # First determine if the sink path exists
        sink = self.factory.getSinkEndpoints()[0]
        sinkIsDir = sink.pathIsDir(self.sinkTarget)
        sinkExists = sink.pathExists(self.sinkTarget)
        # This is tricky, if the sink path exists from a previous
        # transfer attempt and this is a retry, then we need to lie about it
        if self.restartFlag and sinkExists and \
           sink.transferIsComplete(self.sinkTarget, self.sinkTarget):
            sinkExists = False

        # Use one of the sources to build the transfer pairs
        # Note you have to do this on the source in case the requested
        # path is a directory or symlink
        source = self.factory.getSourceEndpoints()[0]
        (rc, dirs, files, links) = source.buildWalk(self.sourceTarget,
                                                    self.sinkTarget,
                                                    targetExists=sinkExists,
                                                    targetIsDir=sinkIsDir)
        if 0 != rc:
            print('ERROR:  Unable to access', self.sourceTarget)

        #print(rc)
        #print(dirs)
        #print(files)
        #print(links)
        # Build all of the remote directories with the first sink
        if 0 == rc:
            for (sdir, tdir) in dirs:
                rc = self.createDir(sdir, tdir)
                if 0 != rc:
                    print('ERROR transferring', sdir)
                    break
                sink.markTransferCompleted(self.sinkTarget, tdir)

        # Move all files
        if 0 == rc:
            for (sfile, tfile) in files:
                if self.restartFlag and \
                   sink.transferIsComplete(self.sinkTarget, tfile):
                    print(sfile, 'previously completed')
                    continue
                rc = self.startTransfer(sfile, tfile)
                if 0 != rc:
                    print('ERROR starting transfer', sfile)
                    break
                rc = self.monitorTransfer(monitorInterval)
                if 0 != rc:
                    print('ERROR transferring', sfile)
                    break
                if self.restartFlag and 0 == rc:
                    sink.markTransferCompleted(self.sinkTarget, tfile)
                

        # Create all symlinks
        if 0 == rc:
            for (ssym, tsym, link) in links:
                rc = self.createSymlink(ssym, tsym, link)
                if 0 != rc:
                    print('ERROR transferring', ssym)
                    break
                sink.markTransferCompleted(self.sinkTarget, tsym)

        # Remove the progress file
        if 0 == rc:
            sink.removeTransferProgressFile(self.sinkTarget)

        return rc

    def cleanupEndpoints(self):
        """
        Determine exit status for stopped flows, and kill any remaining flows
        """
        rc = 0
        if self.factory:
            endpoints = self.factory.getEndpoints()
            #for e in endpoints:
            #    if True == e.pollFlow():
            #        f.cancelFlow()
            #        print('Cancelling Flow')
            #        rc = 1
            #    elif 0 != f.completionStatus():
            #        print('Flow exited with code:', e.completionStatus())
            #        print('Flow cmd:', e.executeString())
            #        rc = 2

            self.factory.shutdown()
            del self.factory

        self.isCreate = False
        self.isStarted = False
        self.isCompleted = True
        return rc

    def completedSuccessfully(self):
        """@return true if the flows completed successfully"""
        return self.isComplete and self.isSuccess

    def showProgress(self, filename, completedBytes):
        """Print a status line to stderr without a linefeed"""
        # Calculate the status values
        try:
            currentBytes = completedBytes - self.currentRestartOffset
            pctProgress = (float(completedBytes) / self.currentFlowSize) * 100.0
            transferTime = time.time() - self.beginTime
            bandwidth = float(currentBytes) / transferTime
            eta = '--:--:--'
        except ZeroDivisionError:
            # Zero size flows are completed by definition
            pctProgress = 100.0
            transferTime = time.time() - self.beginTime
            bandwidth = 0.0
            eta = '00:00:00'

        if (bandwidth > 0):
            eta_secs = round((self.currentFlowSize - completedBytes)/bandwidth)
            td = datetime.timedelta(seconds=eta_secs)
            eta = str(td)

        # Get the width of the terminal
        cols = 80
        if sys.stdin.isatty():
            stty_size = os.popen('stty size').read()
            if 0 != len(stty_size):
                (r, c) = stty_size.split()
                cols = int(c)

        # Generate the status string
        status = ''
        if 100.0 <= pctProgress:
            status += str(round(pctProgress)) + '% '
        elif 10.0 <= pctProgress:
            status += str(pctProgress)[0:4] + '% '
        else:
            status += str(pctProgress)[0:3] + '% '
        if (1024*1024*1024) < bandwidth:
            status += str(completedBytes / (1024*1024*1024)) + 'GiB '
        elif (1024*1024) < bandwidth:
            status += str(completedBytes / (1024*1024)) + 'MiB '
        elif 1024 < bandwidth:
            status += str(completedBytes / 1024) + 'KiB '
        else:
            status += str(completedBytes) + 'B '
        status += str(round(bandwidth / 1024.0 / 1024.0)) + 'MiB/s '
        status += eta + ' ETA'
           
        # Determine the padding length between the file name and status
        padding = ' '
        if cols > (len(status) + len(filename)):
            padding *= cols - len(status) - len(filename)
        
        # Clear the existing status line with whitespace
        print('\r', ' ' * cols, sep='', end='', file=sys.stderr)
         
        # Write the new status line
        print('\r', filename, padding, status, 
              sep='', end='', file=sys.stderr)

    def writeLog(self, isWriteCommand=False, flushAll=False):
        """
        Log current status to the logfile.  This isn't free.  It sends alot 
        of data over the RPC channels if this transfer is remote.
        """
        if self.verboseLog is not None:
            f = open(self.verboseLog, 'a')
            # Write sink data
            if self.sinkVerboseFlag:
                sinks = self.factory.getSinkEndpoints()
                for s in sinks:
                    prefix = s.hostname() + ': '
                    if isWriteCommand:
                        f.write(prefix + s.executeString() + os.linesep)
                    else:
                        lines = s.output(flushAll).split(os.linesep)
                        for l in lines:
                            if l:
                                f.write(prefix + l + os.linesep)
            # Write source data
            if self.sourceVerboseFlag:
                sources = self.factory.getSourceEndpoints()
                for s in sources:
                    prefix = s.hostname() + ': '
                    if isWriteCommand:
                        f.write(prefix + s.executeString() + os.linesep)
                    else:
                        lines = s.output(flushAll).split(os.linesep)
                        for l in lines:
                            if l:
                                f.write(prefix + l + os.linesep)
            f.close()
