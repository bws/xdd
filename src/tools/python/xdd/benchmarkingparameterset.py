
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

class TrialFailError(XDDError):

    def __init__(reason):
        self.reason = reason

#
#
#
class BenchmarkingParameterSet(object):
    """
    Parameter set for benchmarking
    """

    def __init__(self, locations, sizes, queueDepth, buffering, ordering, rate):
        """Constructor"""
        self._locationList = locations
        self.sizeList_ = sizes

    def writeOutput(self):
        return

    def start(self):
        """Start the benchmarking trials"""
        
    def wait(self):
        """Set the size"""
        self.transferSize = transferSize
        
    def name(self):
        """Return the name associated with this benchmarking parameter set"""
        return None

class CustomBenchmarkingParameterSet(object):
    """
    Parameter set for benchmarking
    """

    def __init__(self, location, sizes, queueDepth, buffering, ordering, rate):
        """Constructor"""

    def writeOutput(self):
        return

    def start(self):
        """Start the benchmarking trials"""
        
    def wait(self):
        """Set the size"""
        self.transferSize = transferSize
        
    def name(self):
        """Return the name associated with this benchmarking parameter set"""
        return "custom"

class LSIBenchmarkingParameterSet(BenchmarkingParameterSet):
    """
    Parameter set for benchmarking
    """

    def __init__(self, location):
        """Constructor"""
        super.__init__()

    def writeOutput(self):
        return

    def start(self):
        """Start the benchmarking trials"""
        
    def wait(self):
        """Set the size"""
        self.transferSize = transferSize
        
    def name(self):
        """Return the name associated with this benchmarking parameter set"""
        return "lsi"


class EonStoreBenchmarkingParameterSet(BenchmarkingParameterSet):
    """
    Parameter set for benchmarking
    """

    def __init__(self):
        """Constructor"""

    def writeOutput(self):
        return

    def start(self):
        """Start the benchmarking trials"""
        
    def wait(self):
        """Set the size"""
        self.transferSize = transferSize
        
    def name(self):
        """Return the name associated with this benchmarking parameter set"""
        return "eonstore"

class SFA11KStoreBenchmarkingParameterSet(BenchmarkingParameterSet):
    """
    Parameter set for benchmarking
    """

    def __init__(self, location, sizes, queueDepth, buffering, ordering, rate):
        """Constructor"""

    def writeOutput(self):
        return

    def start(self):
        """Start the benchmarking trials"""
        
    def wait(self):
        """Set the size"""
        self.transferSize = transferSize
        
    def name(self):
        """Return the name associated with this benchmarking parameter set"""
        return "sfa11k"


