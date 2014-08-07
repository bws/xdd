
#
# Python stuff to improve portability
#
from __future__ import print_function

#
# Python standard packages
#
import os

#
# 3rd party packages
#
import Pyro4

#
# XDD package imports
#
from xdd.constants import XDD_PROGRESS_FILENAME
from xdd.flow import Flow
from xdd.naming import PosixNamingStrategy

class FlowBuilder(object):
    """
    Helper class that can start flows, generate directory walks, create 
    symlinks, and retrieve the restart offset for partially completed 
    transfers.  This class is designed for use with an RPC-system, thus its
    grab-bag nature.
    """
    def __init__(self):
        """Constructor"""
        self.flow = None
        self.currentFlowStatus = None
        try:
            self.unameHost = os.uname()[1]
        except:
            self.unameHost = 'unknown'

    def shutdown(self):
        """Shutdown"""
        pass

    def hostname(self):
        """@return hostname"""
        return self.unameHost

    def pathExists(self, path):
        """@return true if the path exists"""
        return os.path.exists(path)

    def pathIsDir(self, path):
        """@return true if the path exists"""
        return os.path.isdir(path)

    def getFileSize(self, path):
        """@return file or block device size in bytes"""
        size = 0
        try:
            fd = os.open(path, os.O_RDONLY)
            try:
                size = os.lseek(fd, 0, os.SEEK_END)
            except:
                pass
            os.close(fd)
        except:
            pass
        return size

    def buildWalk(self, source, target, targetExists, targetIsDir):
        """
        @return a (rc, dirs, files, symlinks) tuple
        rc is 0 on success
        dirs is the list of source dirs to create
        files is a list of (sourcename, sinkname) tuples
        links is a list of (target, sinkname) tuples
        """
        namer = PosixNamingStrategy()
        return namer.buildDirsFilesLinks(source, target, 
                                         targetExists, targetIsDir)
    def createDirectory(self, target, mode=0755):
        """
        Create the directory target if it does not exists.  If it exists
        that is ok too
        @return 0 if the target directory exists on completion, 
          otherwise non-zero
        """
        rc = 0
        try:
            os.mkdir(target, mode)
        except OSError:
            # If the create failed, check and see if it exists
            if not os.path.isdir(target):
                rc = 1
        return rc

    def createSymlink(self, source, linkName):
        """Generate a symlink target that points at source"""
        rc = 0
        try:
            os.symlink(source, linkName)
        except OSError:
            # If the create failed, check and see if it exists
            if os.path.islink(linkName) and source == os.readlink(linkName):
                rc = 0
            else:
                rc = 1
        return rc
            
    def createEmptyFile(self, target):
        """Create an empty file"""
        rc = 0
        try:
            open(target, 'w').close()
        except OSError:
            # If the create failed, check and see if it exists
            if not os.path.isfile(source):
                rc = 1
        return rc
       
    def removeRestartCookie(self, filename):
        """Removes the restart cookie"""
        cookieName = self.flow.getRestartCookiename(filename)
        rc = 0
        try:
            os.unlink(cookieName)
        except OSError:
            rc = 1
        return rc

    def getRestartOffset(self, filename):
        """@return the restart offset from the existing cookie"""
        # Because the file format is tied to XDD, let the flow extract the
        # information
        return self.flow.getRestartOffset(filename)

    def markTransferCompleted(self, sinkRoot, filename):
        """Add filename to progress list"""
        rc = 0
        progressFilename = os.path.join(sinkRoot, XDD_PROGRESS_FILENAME)
        try:
            f = open(progressFilename, 'a')
            try:
                f.write(filename + os.linesep)
            except:
                rc = 2
            finally:
                f.close()
        except:
            rc = 1
        return rc

    def transferIsComplete(self, sinkRoot, filename):
        """@return True if filename is in the progress file, otherwise False"""
        isComplete = False
        progressFilename = os.path.join(sinkRoot, XDD_PROGRESS_FILENAME)
        try:
            f = open(progressFilename, 'r')
            try:
                cmpname = filename + os.linesep
                for l in f.readlines():
                    if cmpname == l:
                        isComplete = True
                        break
            except:
                pass
            finally:
                f.close()
        except:
            pass
        return isComplete

    def removeTransferProgressFile(self, sinkRoot):
        """Remove the file containing transfer progress"""
        rc = 0
        progressFilename = os.path.join(sinkRoot, XDD_PROGRESS_FILENAME)
        try:
            os.unlink(progressFilename)
        except:
            rc = 1
        return rc

    def protocolVersion(self):
        """@return The XDD protocol version, or empty string if no valid version is found"""
        pv = self.flow.protocolVersion()
        if pv is None:
            pv = ""
        return pv

    def buildFlow(self, isSink, reqSize, flowIdx, numFlows, ifaces,
                  dioFlag, serialFlag, verboseFlag, timestampFlag, 
                  xddPath):
        """Create the Flow"""
        self.flow = Flow(isSink=isSink, reqSize=reqSize, 
                         flowIdx=flowIdx, numFlows=numFlows, 
                         ifaces=ifaces, dioFlag=dioFlag, serialFlag=serialFlag,
                         verboseFlag=verboseFlag, timestampFlag=timestampFlag, 
                         xddPath=xddPath)

    def startFlow(self, filename, dataSize, restartFlag, restartOffset):
        """Start an XDD process"""
        self.currentFlowStatus = self.flow.start(filename, dataSize, 
                                                 restartFlag, restartOffset)
        if self.currentFlowStatus is not None:
            return 0
        else:
            return 1

    def errorString(self):
        """@return the error string from the flow during flow start"""
        return self.currentFlowStatus.errorString()

    def cancelFlow(self):
        """Stop a running XDD process"""
        return self.currentFlowStatus.cancel()

    def pollFlow(self):
        """Return the current state of a running XDD process"""
        return self.currentFlowStatus.poll()

    def completionStatus(self):
        """Return the return code from the XDD process"""
        return self.currentFlowStatus.completion()

    def currentByte(self):
        """@return the most recently processed byte"""
        return self.currentFlowStatus.currentByte()

    def output(self, flushAll=False):
        """@return the flow's standard output"""
        return self.currentFlowStatus.output(flush=flushAll)

    def executeString(self):
        args = self.flow.createCommandArgs()
        s = ''
        for a in args:
            s += a + ' '
        return s.rstrip()

    def hasPreallocateAvailable(self):
        """@return if preallocation is available for a flow"""
        # All we check here is that XDD supports pre-allocation, we
        # do not check file system support, because there is no way
        # the check could be correct
        return self.flow.hasPreallocate()

class RemoteFlowBuilder(FlowBuilder):
    """
    Pyro helper class that builds remote flows and dispatches messages to the 
    flow
    """

    def __init__(self, daemon):
        """Constructor"""
        super(RemoteFlowBuilder, self).__init__()
        self.daemon = daemon
        self.i = 0

    def shutdown(self):
        """
        Stop the pyro daemon.  This is guaranteed to raise a ConnectionClosed
        Exception, and is therefore not typically useful.  Shutdown from
        factory is exception safe.
        """
        print("Warning: Client initiated shutdown")
        self.daemon.shutdown()

    def isReady(self):
        """
        Returns True when the proxy is established.  Pyro throws an internal
        exception (usually Pyro4.errors.ConnectionClosedError) if the
        service or transport are not ready
        """
        return True

    def test(self):
        """Return a test string"""
        self.i += 1
        return "Test string " + str(self.i)



