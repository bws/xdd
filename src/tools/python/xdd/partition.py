#
# Python stuff to improve portability
#
from __future__ import print_function

#
# XDD functionality
#
from xdd.core import XDDError

#
# Exceptions for this module
#
class InvalidPartitionError(XDDError):
    """An invalid set of parameters input to partitioner"""
    pass

#
# Partitioner used to break a contiguous range into a collection of
# similarly sized ranges.  This is used to determine how to split files over
# multiple hosts.
#
class PartitionStrategy(object):
    def partBegin(self, part):
        """@return the first offset in the partition"""
        return 0

    def partEnd(self, part):
        """@return the last offset in the partition"""
        return 0

    def partSize(self, part):
        """@return the size of the partition, part"""
        return self.partEnd(part) - self.partBegin(part) + 1

    def getPart(self, part):
        """
        @return an (offset,size) tuple describing the first offset and the
           size of the partition
        """        
        return (self.partBegin(part), self.partSize(part))

class SimplePartitionStrategy(PartitionStrategy):
    """
    Class that handles partitioning a size into partitions of granules.  In 
    this default case, all partitions are contiguous (and, with the exception
    of the final partition, a multiple of granules).
    """
    def __init__(self, parts, size):
        """
        Construct a partitioner based on the total size to partition, and 
        the total number of parts to create
        """
        if 0 == parts or 0 == size or parts > size:
            raise InvalidPartitionError

        self.parts = parts
        self.size = size
        pass

    def partBegin(self, part):
        """@return the first offset in the partition"""
        return (part * self.size) / self.parts
 
    def partEnd(self, part):
        """@return the last offset in the partition"""
        return self.partBegin(part + 1) - 1
 

class AlignedPartitionStrategy(PartitionStrategy):
    """
    Class that handles partitioning a size into partitions of aligned granules.
    If the number of partitions requested is too large, this strategy
    will create the maximum possible number of partitions, and then return
    empty partitions (0, 0) for partitions beyond that amount.  All possible
    partitions are contiguous.
    """
    def __init__(self, requestedParts, granule, size):
        """
        Construct a partitioner based on the total size to partition, the
        granularity of individual parts, and the total number of parts to 
        create
        """
        self.requestedParts = requestedParts
        self.granule = granule
        self.size = size

        # Handle the case where alignment will result in some empty
        # partitions
        if (requestedParts * granule) > size:
            self.parts = size / granule
        else:
            self.parts = requestedParts

    def possibleParts():
        """@return the number of parts that have a non-zero sized partition"""
        return self.parts

    def partBegin(self, part):
        """@return the first offset in the partition"""
        if part < self.parts:
            granules = self.size / self.granule
            beg = part * granules /  self.parts * self.granule
        else:
            beg = 0
        return beg

    def partEnd(self, part):
        """@return the last offset in the partition"""
        if part == self.parts - 1:
            end = self.size - 1
        elif part < self.parts:
            end = self.partBegin(part + 1) - 1
        else:
            end = 0
        return end
 
    def partSize(self, part):
        """@return the size of the partition, part"""
        if 0 == self.parts:
            diff = self.size
        elif part < self.parts:
            diff = self.partEnd(part) - self.partBegin(part) + 1
        else:
            diff = 0
        return diff


