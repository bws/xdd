
#
# Python stuff to improve portability
#
from __future__ import print_function

#
# Generic python packages used
#
import fcntl
import os
import subprocess
import sys
from stat import *

#
# XDD package imports
#
from xdd.constants import XDD_PREALLOC_TOKEN
from xdd.partition import AlignedPartitionStrategy

#
# Flow status used to monitor the output of a flow once it's been started
#
class FlowStatus(object):
    """
    Class that handles one of end a streaming data file transfer
    """
    def __init__(self):
        pass

    def cancel(self):
        return 1

    def poll(self):
        return 1

    def completion(self):
        return 1

    def currentByte(self):
        return -1

    def errorString(self):
        return 'ERROR'

    def output(self, flush=False):
        return self.errorString()

class FailedFlowStatus(FlowStatus):
    """
    FlowStatus returned when the flow could not start successfully
    """
    def __init__(self, reasons):
        """Constructor"""
        self.reasons = reasons

    def errorString(self):
        """@return the reasons provided by the flow"""
        errors = ''
        for r in self.reasons:
            errors += r + os.linesep
        return errors

class EmptyFileFlowStatus(FlowStatus):
    """
    XDD cannot handle flows of size 0.  In that case, just use this class to
    create an empty file, but still using the regular flow logic.
    """
    def __init__(self, isCreator, filename, restartFlag):
        """Create an empty file"""
        self.filename = filename
        if isCreator:
            self.success = False
            try:
                open(filename, 'w').close()
                self.success = True
            except OSError:
                # If the create failed, check and see if it exists
                if not os.path.isfile(source):
                    self.success = True
        else:
            self.success = True

    def cancel(self):
        return 0

    def poll(self):
        if self.success:
            return 0
        else:
            return 1

    def completion(self):
        if self.success:
            return 0
        else:
            return 1

    def currentByte(self):
        return 0

    def errorString(self):
        if not self.success:
            return 'ERROR Creating empty file'
        return None

    def output(self, flush=False):
        return 'Created file ' + self.filename

class XDDFlowStatus(FlowStatus):
    """
    Monitor the status of a started XDD process.
    """
    def __init__(self, process, restartByte):
        self.process = process
        self.errorData = ''
        self.heartbeatByte = restartByte

    def cancel(self):
        """Cancel the underlying XDD process that does the work"""
        print('Cancelling flow.')
        self.process.kill()

    def poll(self):
        """@return True if the XDD process is still active, otherwise False"""
        self.process.poll()
        if self.process.returncode is None:
            return True
        else:
            return False

    def errorString(self):
        """@return string describing error"""
        if self.errorData:
            return self.errorData
        return 'Unable to connect xdd processes. Ensure ports are free.'

    def output(self, flush=False):
        """@return the stdout produced thus far for the flow"""
        # Turn off blocking for stdout
        eflags = fcntl.fcntl(self.process.stdout, fcntl.F_GETFL)
        fcntl.fcntl(self.process.stdout, fcntl.F_SETFL, eflags | os.O_NONBLOCK)
        
        # Flush the contents of stderr using the currentByte() method
        if flush:
            self.currentByte()
        data = self.errorData

        # Read the data 1024 bytes at a time
        self.errorData = ''
        try:
            while True:
                temp = self.process.stdout.read(1024)
                data += temp
                if 0 == len(temp):
                    break
        except IOError:
            # This is invoked when the stream is out of data
            pass
        except OSError:
            # This is part of the spec, I don't know why
            pass

        return data

    def completion(self):
        """@return XDD return code"""
        self.process.poll()
        return self.process.returncode

    def currentByte(self):
        """@return the most recently completed byte"""
        # Turn off blocking for stderr
        eflags = fcntl.fcntl(self.process.stderr, fcntl.F_GETFL)
        fcntl.fcntl(self.process.stderr, fcntl.F_SETFL, eflags | os.O_NONBLOCK)        
        
        # For safety, get the last 2 lines of stderr
        last = ''
        try:
            while True:
                temp= self.process.stderr.read(1024)
                self.errorData += temp
                if 0 == len(temp):
                    break
                last = temp
        except IOError:
            # This is invoked when the stream is out of data
            pass
        except OSError:
            # This is part of the spec, I don't know why
            pass

        # Split the last line, and update the heartbeat byte if it exists
        sl = last.split(',')
        
        # Sometimes XDD gives an alternate WAITING format, if we verify that
        # the first field is Pass and the 5th field begins with B, 
        # then the bytes are in the 4th field
        if 5 <= len(sl) and '\nPass' == sl[0] and 'B' == sl[4][0]:
            # Convert the byte field into a int if possible
            byteString = sl[3]
            try:
                value = int(byteString)
                self.heartbeatByte = value
            except ValueError:
                print("ERROR: Invalid heartbeat", byteString)
        return self.heartbeatByte     

#
# Flow class that does the work for the transfer hosts
#
class Flow():
    """
    Class that starts an XDD flow.  Since this class starts a UNIX process, it 
    is by its nature not serializable, and should rarely be used directly.  
    Instead, you should generally use a FlowBuilder (either directly, or via 
    a Pyro Proxy) to interact with flows. 
    Because of the serializable nature of this class, and the lack of easy name
    support, this class does not raise exceptions.  Error information is 
    provided via return codes and the getErrorString method.
    """

    def __init__(self, isSink, reqSize, flowIdx, numFlows, ifaces,
                 dioFlag, serialFlag, verboseFlag, timestampFlag, xddPath=''):
        self.isSink = isSink
        self.reqSize = reqSize
        self.flowIdx = flowIdx
        self.numFlows = numFlows
        self.ifaces = ifaces
        self.dio = dioFlag
        self.serial = serialFlag
        self.verbose = verboseFlag
        self.timestamp = timestampFlag

        # Find a path complemented version of XDD
        if '' != xddPath:
            self.xddExe = os.path.join(xddPath, 'xdd')
        else:
            # Try to use 'which' to find xdd's path
            cmd = ['which', 'xdd']
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
            (data, _) = p.communicate()
            self.xddExe = data.rstrip()
            if '' == self.xddExe:
                self.xddExe = 'xdd'

        self.processStatus = None
        self.process = None
        self.reasons = []

        # Variables set later
        self.target = ''
        self.dataSize = 0
        self.startOffset = 0
        self.portOffset = 0
        self.restart = False
        self.restartByte = 0
        self.heartbeatByte = self.restartByte

    def checkSinkPreconditions(self):
        """
        Checks the following preconditions for a sink:
          Either file exists and is writable or 
                 parent directory exists and is searchable and writable
          The data size provided maskes sense
          The dio flag is not set for character devices
        """
        assert self.isSink
        passes = True

        # Determine if the path exists
        if os.path.exists(self.target):
            # Check other preconditions based on file type
            res = os.stat(self.target)
            if S_ISREG(res.st_mode) or S_ISLNK(res.st_mode):
                # For regular files and links just check read permission
                if not os.access(self.target, os.W_OK):
                    self.reasons.append("Cannot write source: " + self.target)
                    passes = False;
            elif S_ISBLK(res.st_mode):
                # For block devices check read permissions
                if not os.access(self.target, os.W_OK):
                    self.reasons.append("Cannot write block device: " + 
                                        self.target)
                    passes = False;
            elif S_ISCHR(res.st_mode):
                # For character devices, check that DIO and restart are off
                if not os.access(self.target, os.W_OK):
                    self.reasons.append("Cannot write character device: " + 
                                        self.target)
                    passes = False;
                elif self.dio:
                    self.reasons.append("Device does not support Direct I/O: " +
                                        self.target)
                    passes = False;
                elif self.restart:
                    self.reasons.append("Character device does not support restart: " +
                                        self.target)
                    passes = False;

            else:
                passes = False
                self.reasons.append("Target is unsupported file type:" + 
                                    self.target)
        else:
            # Determine the parent directory
            parentDir = os.path.dirname(self.target)
            if not parentDir:
                parentDir = '.'

            # Check the parent directory for existence
            if not os.path.exists(parentDir):
                self.reasons.append("Parent directory does not exist: " +
                                    self.target)
                passes = False
            elif not os.access(parentDir, os.W_OK):
                self.reasons.append("Cannot write parent directory " + 
                                    self.target)
                passes = False

        if self.restart:
            # We must be able to create the restart file
            parentDir = os.path.dirname(self.target)
            if not os.path.exists(parentDir) and os.access(parentDir, os.W_OK):
                self.reasons.append("Restart requires write access to " +
                                    "destination parent directory")
                passes = False

        return passes

    def checkSourcePreconditions(self):
        """
        Check the following preconditions for a source:
          - The file exists
          - The file is readable
          - If the file is a block device or file, it is large enough to
            satisfy the data size
          - If the file is a character device, the data size must be positive
          - DIO can only be set for block devices and file
        """
        assert not self.isSink
        passes = True
        
        # Ensure the source exists
        if not os.path.exists(self.target):
            self.reasons.append("Target does not exist: " + self.target)
            passes = False
        else:
            # Check other preconditions based on file type
            res = os.stat(self.target)
            if S_ISREG(res.st_mode) or S_ISLNK(res.st_mode):
                # For regular files and links just check read permission
                if not os.access(self.target, os.R_OK):
                    self.reasons.append("Cannot read source: " + self.target)
                    passes = False;
            elif S_ISBLK(res.st_mode):
                # For block devices check read permissions
                if not os.access(self.target, os.R_OK):
                    self.reasons.append("Cannot read block device: " + 
                                        self.target)
                    passes = False;
            elif S_ISCHR(res.st_mode):
                # For character devices, check that DIO is off
                if not os.access(self.target, os.R_OK):
                    self.reasons.append("Cannot read character device: " + 
                                        self.target)
                    passes = False;
                elif self.dio:
                    self.reasons.append("Device does not support Direct I/O: " +
                                        self.target)
                    passes = False;
            else:
                passes = False
                self.reasons.append("Target is unsupported file type:" + 
                                    self.target)
        return passes

    def checkPreconditions(self):
        """
        Checks preconditions to ensure the XDD process should start correctly
        """
        # First ensure that xdd exists and is executable
        if ((not os.path.exists(self.xddExe)) or 
            (not os.access(self.xddExe, os.X_OK))):
            self.reasons.append('Unable to execute: ' + self.xddExe)
            return False

        # Make sure the restart offset isn't obviously corrupt
        restartIsValid = True
        if self.restartByte > 0:
            try:
                fd = os.open(self.target, os.O_RDONLY)
                try:
                    size = os.lseek(fd, 0, os.SEEK_END)
                    restartIsValid = (self.restartByte <= size)
                except:
                    restartIsValid = False
                os.close(fd)
            except:
                restartIsValid = False
        if not restartIsValid:
            cookie = self.getRestartCookiename(self.target)
            self.reasons.append('Corrupt restart cookie: ' + cookie)
            self.reasons.append('Restart offset larger than destination file')
            return False

        # Now check that permissions and flags are compatible
        if self.isSink:
            passes = self.checkSinkPreconditions()
        else:
            passes = self.checkSourcePreconditions()
        return passes

    def getRestartCookiename(self, filename):
        """@return the name of the restart file for this sink"""
        # The restart file is named .filename-idx-n.xrf
        (head, tail) = os.path.split(filename)
        name = '.' + tail + '-' + str(self.flowIdx) + '-' + str(self.numFlows)
        name += '.xrf'
        restartFilename = os.path.join(head, name)
        return restartFilename

    def getRestartOffset(self, filename):
        """@return the restart offset if the restart flag is true, else 0"""
        offset = 0
        rfn = self.getRestartCookiename(filename)
        try:
            rf = open(rfn, 'r')
            cookie = rf.readline();
            rf.close()
            cl = cookie.split(' ')
            offset = int(cl[2])
        except IOError:
            self.reasons.append('Unable to open file ' + rfn)            
        except (ValueError, IndexError) as e:
            self.reasons.append('Corrupt restart cookie found ' + cookie)
        return offset

    def createCommandArgs(self):
        """@return the command used to start the XDD process"""
        # Assemble the basic file system command
        cmd = [self.xddExe, '-target', self.target]
        if self.isSink:
            cmd.extend(['-op', 'write', '-e2e', 'isdest'])
        else:
            cmd.extend(['-op', 'read', '-e2e', 'issource'])
        cmd.extend(['-reqsize', str(self.reqSize), '-blocksize', '1'])
        cmd.extend(['-bytes', str(self.dataSize)])

        # Add interfaces for network
        for iface in self.ifaces:
            # Determine if optional NUMA has been supplied
            if 4 == len(iface):
                (host, port, threads, numa) = iface
            else:
                (host, port, threads) = iface
                numa = None
            # Modify the port number if this is a multi-host
            spec = host + ':' + str(port) + ',' + str(threads)
            if numa:
                spec += ',' + numa
            cmd.extend(['-e2e', 'dest', spec])
            
        
        # Process flags
        if self.dio:
            cmd.append('-dio')
        if self.serial:
            cmd.append('-serialordering')
        elif self.isSink:
            cmd.append('-noordering')
        else:
            cmd.append('-looseordering')

        # Restart processing
        if self.restart:
            if self.isSink:
                cookie = self.getRestartCookiename(self.target)
                cmd.extend(['-restart', 'offset', str(self.restartByte)])
                cmd.extend(['-restart', 'enable'])
                cmd.extend(['-restart', 'file', cookie])
            elif 1 == self.numFlows:
                cmd.extend(['-restart', 'offset', str(self.restartByte)])
            else:
                cmd.extend(['-startoffset', str(self.restartByte + self.startOffset)])

        # Multi-host stuff
        if not self.restart and 1 < self.numFlows:
            cmd.extend(['-startoffset', str(self.startOffset)])
   
        # Preallocation support
        if self.isSink and 0 == self.restartByte:
            cmd.extend(['-preallocate', str(self.dataSize)])

        # Add on all of the standard output stuff
        cmd.extend(['-verbose', '-minall', '-stoponerror'])

        # Add the heartbeat for sink
        if self.isSink:
            cmd.extend(['-hb', '1', '-hb', 'bytes', '-hb', 'lf'])

        return cmd

    def start(self, filename, flowSize, restartFlag=False, restartOffset=0):
        """Start an XDD process to perform work"""
        # Add the final options
        self.target = filename
        self.restart = restartFlag
        self.restartByte = restartOffset

        # If restarting multiple sources, rather than partitioning from the
        # beginning, partition from the restart point
        if self.restart and self.numFlows > 1:
            flowSize -= restartOffset

        # Use a partitioner to convert the flow size into the local flow params
        partitioner = AlignedPartitionStrategy(self.numFlows, 
                                               self.reqSize,
                                               flowSize)
        (self.startOffset, self.dataSize) = partitioner.getPart(self.flowIdx)

        # Check preconditions
        if self.checkPreconditions():
            # Use the file creator for size 0 flows, XDD for everything else
            if 0 == self.dataSize:
                isCreator = (0 == self.flowIdx) and self.isSink
                status = EmptyFileFlowStatus(isCreator, filename, restartFlag)
            else:
                args = self.createCommandArgs()
                try:
                    self.process = subprocess.Popen(args,
                                                    stdout=subprocess.PIPE,
                                                    stderr=subprocess.PIPE)
                    status = XDDFlowStatus(self.process, restartOffset)
                except OSError as e:
                    self.reasons.append("OS Error executing: " + args[0])
                    status = FailedFlowStatus(self.reasons)
                except:     
                    s = "Unknown error: " + str(sys.exc_info()[0])
                    self.reasons.append(s)
                    status = FailedFlowStatus(self.reasons)
        else:
            # Precondition failure
            status = FailedFlowStatus(self.reasons)
        return status
            
    def hasPreallocate(self):
        """@return true if XDD is compiled with preallocation support"""
        # Just use nm (assume its in the path)
        cmd = ['nm', self.xddExe]
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
        (stdoutdata, _) = p.communicate()
        # If XDD has preallocate support it will have a special symbol
        lines = stdoutdata.split(os.linesep)
        for l in lines:
            sl = l.split(' ')
            if 3 == len(sl) and XDD_PREALLOC_TOKEN == sl[2]:
                return True 
        return False
        
    def protocolVersion(self):
        """@return the XDD protocol version string"""
        cmd = [self.xddExe, '-version']
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
        (stdoutdata, _) = p.communicate()
        temp = stdoutdata.split(':')
        if 2 == len(temp):
            proto = temp[1].strip()
            return proto
        else:
            return None
        
