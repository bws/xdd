
#
# Python stuff to improve portability
#
from __future__ import print_function

#
# Generic python packages used
#
import sys

#
#
#
class ProfileParameters(object):
    """
    Parameter set for benchmarking
    """
    _profileParametersDict = {}
    
    def __init__(self, reqSizes, queueDepths, dios, orders, patterns, allocs):
        """Constructor"""
        self.target = None
        self.keepfiles = False
        self.reqsizes = reqSizes
        self.qdepths = queueDepths
        self.dios = dios
        self.offsets = [0]
        self.optypes = ['write', 'read']
        self.orders = orders
        self.patterns = patterns
        self.allocs = allocs
        self.modes = ['w', 'r']
        ProfileParameters._profileParametersDict[self.name()] = self

    def name(self):
        """Return the name associated with this benchmarking parameter set"""
        return None

    def setDIO(self):
        """Set profiling to directio only"""
        self.dios = [ True ]
        
    def setKeepFiles(self, keep):
        """Set profiling to keep the created files"""
        self.keepfiles = keep
        
    def setQueueDepth(self, qd):
        """Set the profiling reqsize"""
        self.qdepths = [ qd ]
        
    def setReadOnly(self):
        """Enable only read profiling"""
        self.optypes = ['read']
        
    def setReadWrite(self):
        """Enable read and write profiling"""
        self.optypes = ['write', 'read']
        
    def setWriteOnly(self):
        """Enable only write profiling"""
        self.optypes = ['write']

    def setOffset(self, offset):
        """Set the profiling offset"""
        self.offsets = [ offset ]

    def setOrderLoose(self):
        """Set the profiling order"""
        self.orders = [ 'loose' ]

    def setOrderNone(self):
        """Set the offset"""
        self.orders = [ 'none' ]

    def setOrderSerial(self):
        """Set the offset"""
        self.orders = [ 'serial' ]

    def setReqsize(self, size):
        """Set the reqsize"""
        self.reqsizes = [ size ]

    def setTarget(self, path):
        """Set the personality target"""
        self.target = path
        
    def getAllocs(self):
        """@return list of allocation strategies"""
        return self.allocs
    
    def getDios(self):
        """@return list of dio settings"""
        return self.dios
    
    def getKeepFiles(self):
        """@return true if profiled files are kept"""
        return self.keepfiles
    
    def getOffsets(self):
        """@return list of offsets"""
        return self.offsets
    
    def getOpTypes(self):
        """@return list of operation types"""
        return self.optypes
    
    def getOrders(self):
        """@return list of order settings"""
        return self.orders
    
    def getPatterns(self):
        """@return list of order settings"""
        return self.patterns

    def getQueueDepths(self):
        """@return list of queue depths"""
        return self.qdepths
    
    def getReqSizes(self):
        """@return list of request sizes"""
        return self.reqsizes
    
    def getTarget(self):
        return self.target
    
    @staticmethod
    def create(name):
        return ProfileParameters._profileParametersDict[name]

    @staticmethod
    def list():
        return ProfileParameters._profileParametersDict.keys()

class DefaultProfileParameters(ProfileParameters):
    """
    Parameter set for benchmarking
    """
    def __init__(self):
        """Constructor"""
        reqsizes = [4096, 64*1024, 256*1024,
                    1024*1024, 4*1024*1024, 8*1024*1024] 
        threads = [1, 2, 3, 4, 6, 8, 12, 16]
        dios = [True, False]
        orders = ['serial', 'loose', 'none']
        patterns = ['seq', 'random']
        allocs = ['demand', 'pre', 'trunc']
        ProfileParameters.__init__(self, reqsizes, threads, dios, orders,
                                   patterns, allocs)

    def name(self):
        """Return the name associated with this benchmarking parameter set"""
        return 'default'
    
class RamsesProfileParameters(ProfileParameters):
    """
    Parameter set for benchmarking
    """
    def __init__(self):
        """Constructor"""
        reqsizes = [1024*1024, 4*1024*1024, 8*1024*1024] 
        threads = [1, 2, 3, 4, 8]
        dios = [True, False]
        orders = ['serial', 'loose']
        patterns = ['seq']
        allocs = ['pre']
        ProfileParameters.__init__(self, reqsizes, threads, dios, orders,
                                   patterns, allocs)

    def name(self):
        """Return the name associated with this benchmarking parameter set"""
        return 'ramses'

class JoshProfileParameters(ProfileParameters):
    """
    Parameter set for benchmarking
    """
    def __init__(self):
        """Constructor"""
        reqsizes = [1024*1024, 4*1024*1024, 8*1024*1024] 
        threads = [1, 2, 3, 4, 8]
        dios = [True, False]
        orders = ['serial', 'loose']
        patterns = ['seq']
        allocs = ['pre']
        ProfileParameters.__init__(self, reqsizes, threads, dios, orders,
                                   patterns, allocs)

    def name(self):
        """Return the name associated with this benchmarking parameter set"""
        return 'joshpork'

class MemoryProfileParameters(ProfileParameters):
    """
    Parameter set for benchmarking
    """
    def __init__(self):
        """Constructor"""
        reqsizes = [4096, 64*1024, 256*1024,
                    1024*1024, 4*1024*1024] 
        threads = [1, 2, 3, 4, 6, 8, 12, 16]
        dios = [True, False]
        orders = ['serial', 'loose', 'none']
        patterns = ['seq', 'random']
        allocs = ['demand', 'pre', 'trunc']
        ProfileParameters.__init__(self, reqsizes, threads, dios, orders,
                                   patterns, allocs)

    def name(self):
        """Return the name associated with this benchmarking parameter set"""
        return 'memory'
    
class TestingProfileParameters(ProfileParameters):
    """
    Parameter set for benchmarking
    """
    def __init__(self):
        """Constructor"""
        reqsizes = [4*1024, 64*1024, 1024*1024, 4096*1024] 
        threads = [1, 2, 4]
        dios = [False, True]
        orders = ['loose', 'serial', 'none']
        patterns = ['seq', 'random']
        allocs = ['demand', 'pre']
        ProfileParameters.__init__(self, reqsizes, threads, dios, orders,
                                   patterns, allocs)

    def name(self):
        """Return the name associated with this benchmarking parameter set"""
        return 'testing'

# Create the profile personalities
dpp = DefaultProfileParameters()
jpp = JoshProfileParameters()
mpp = MemoryProfileParameters()
rpp = RamsesProfileParameters()
tpp = TestingProfileParameters()


