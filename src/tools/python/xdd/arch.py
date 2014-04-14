"""
NUMA functionality for determining hardware topology information
"""
#
# Python stuff to improve portability
#
from __future__ import print_function


class NumaFactory:
    """Factory class for learning about the numa nature of the system"""

    def __init__(self):
        """ """
        pass

    def convertBusIdsToNodeList(self, busIds):
        """Given a list of PCIe bus ids, return a sorted list of numa nodes"""
        # If this is Linux, use sys
        # Otherwise, just enumerate the numa nodes
        if os.platform.beginsWith('Linux'):
            pass 
        else:
            pass
                

    def getClosestNodesForFile(self, path):
        """@return a list of Node IDs sorted from closest to furthest from"""
        # Determine the mountpoint for the given path
        mount = path

        # Retrieve the device for the mount
        dev = mount

        # Convert the device into a list of pcie addresses
        busIds = []

        # For each Bus Id find the numa node
        for b in busIds:
            closest = 0
        
        nodes = []
        return nodes

    def getClosestNodesForHost(self, host):
        """@return a list of Node IDs sorted from closest to furthest from"""
        nodes = []
        return nodes

class NumaResolverStrategy(object):
    """Base class for numa strategies"""

    def convertClosestNodesToAllNodesList(self, nodes):
        """
        Given a list of nodes equidistant from some resource, pad the list to
        contain all system nodes sorted according to topological distance
        
        @return all nodes sorted closest to furthest
        """
        # The algorithm is very simple, 
        #  enumerate all nodes in the system
        #  figure out the distance from each of the originating nodes
        #  sort the list by those distances
        sortedNodes = []
        unsortedNodes = []
        for n in allNodes:
            if n in nodes:
                sortedNodes.push_front(n)
            else:
                count = getSmallestHops(n, nodes)
                unsortedNodes.append( (count, n) )

        # Sort the nodes by the key value in the tuple

        # Add the now sorted nodes to the sorted list
        for n in unsortedNodes:
            (count, node) = n
            sortedNodes.append(node)
            
        return sortedNodes
            
    def convertSystemIdToNode(self, id):
        """@return the NUMA node for the system ID"""
        return 0

    def getHops(self, src, dest):
        """@return the number of node hops between src and dest"""
        return 0
    

class LinuxNumaResolverStrategy(NumaResolverStrategy):
    """The Linux strategy converts PCIe bus addresses to numa nodes"""

    def convertSystemIdToNode(self, busId):
        """@return the Linux numa node id for the PCIe bus id"""
        return 0

    def getHops(self, src, dest):
        """@return"""

        
        
