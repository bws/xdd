
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
class BenchmarkingManager:
    """
    Manager for running XDD benchmark routines
    """

    def __init__(self, requestSizes, trials, samples):
        """Constructor"""

    def writeOutput(self):
        return

    def start(self):
        """Start the benchmarking trials"""
        
    def wait(self):
        """Set the size"""
        self.transferSize = transferSize
        

