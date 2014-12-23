
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
    
    def __init__(self, reqSizes, queueDepths, dios, orders, patterns):
        """Constructor"""
        self.reqsizes = reqSizes
        self.qdepths = queueDepths
        self.dios = dios
        self.orders = orders
        self.patterns = patterns

    def name(self):
        """Return the name associated with this benchmarking parameter set"""
        return None

    @staticmethod
    def create(name):
        return ProfileParameters._profileParametersDict[name]
    
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
        accesses = ['seq', 'random']
        ProfileParameters.__init__(self, reqsizes, threads, dios, orders, accesses)

    def name(self):
        """Return the name associated with this benchmarking parameter set"""
        return 'default'
    
dpp = DefaultProfileParameters()
ProfileParameters._profileParametersDict[dpp.name()] = dpp

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
        accesses = ['seq']
        ProfileParameters.__init__(self, reqsizes, threads, dios, orders, accesses)

    def name(self):
        """Return the name associated with this benchmarking parameter set"""
        return 'ramses'

rpp = RamsesProfileParameters()
ProfileParameters._profileParametersDict[rpp.name()] = rpp

