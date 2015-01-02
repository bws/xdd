
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
        self.reqsizes = reqSizes
        self.qdepths = queueDepths
        self.dios = dios
        self.orders = orders
        self.patterns = patterns
        self.allocs = allocs
        ProfileParameters._profileParametersDict[self.name()] = self


    def name(self):
        """Return the name associated with this benchmarking parameter set"""
        return None

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

class TestingProfileParameters(ProfileParameters):
    """
    Parameter set for benchmarking
    """
    def __init__(self):
        """Constructor"""
        reqsizes = [1024*1024] 
        threads = [1]
        dios = [True]
        orders = ['serial']
        patterns = ['seq', 'random']
        allocs = ['demand', 'pre', 'trunc']
        ProfileParameters.__init__(self, reqsizes, threads, dios, orders,
                                   patterns, allocs)

    def name(self):
        """Return the name associated with this benchmarking parameter set"""
        return 'testing'

# Create the profile personalities
dpp = DefaultProfileParameters()
rpp = RamsesProfileParameters()
tpp = TestingProfileParameters()


