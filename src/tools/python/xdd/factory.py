
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
from xdd.core import XDDError
from xdd.flowbuilder import FlowBuilder, RemoteFlowBuilder
from xdd.transport import FlowBuilderTransport, FlowBuilderTransportError

class EndpointCreationError(XDDError):
    """
    Error raised when endpoint creation fails
    """
    pass

class EndpointFactory:
    """
    The factory class that creates the FlowBuilders (locally or remote
    as required)
    """

    def __init__(self, transferRequestSize, 
                 sourceDioFlag, sourceSerialFlag, sourceVerboseFlag, 
                 sourceTimestampFlag, sourceXddPath, sources,
                 sinkDioFlag, sinkSerialFlag, sinkVerboseFlag, 
                 sinkTimestampFlag, sinkXddPath, sinks,
                 restartFlag=False):
        """Constructor"""
        # Transfer params
        self.transferRequestSize = transferRequestSize

        # Source params
        self.sourceDioFlag = sourceDioFlag
        self.sourceSerialFlag = sourceSerialFlag
        self.sourceVerboseFlag = sourceVerboseFlag
        self.sourceTimestampFlag = sourceTimestampFlag
        self.sourceXddPath = sourceXddPath
        self.sources = sources

        # Sink params
        self.sinkDioFlag = sinkDioFlag
        self.sinkSerialFlag = sinkSerialFlag
        self.sinkVerboseFlag = sinkVerboseFlag
        self.sinkTimestampFlag = sinkTimestampFlag
        self.sinkXddPath = sinkXddPath
        self.sinks = sinks

        # Optional params
        self.restartFlag = restartFlag

        # Internal variables
        self.remoteTransports = []
        self.sinkFlows = []
        self.sourceFlows = []
        self.allFlows = []
        self.allBuilders = []

    def shutdown(self):
        """Shutdown all transports"""
        for trans in self.remoteTransports:
            trans.shutdown()
        
    def getEndpoints(self):
        """Return all flows"""
        return self.allFlows

    def getSinkEndpoints(self):
        """Return sink flows"""
        return self.sinkFlows

    def getSourceEndpoints(self):
        """Return source flows"""
        return self.sourceFlows

    def buildEndpoint(self, flowSpec):
        """Create an individual endpoint"""
        b = None
        hostIP = flowSpec['ip']
        hostname = flowSpec['hostname']
        if 'localhost' == hostIP:
            b = FlowBuilder()
        else:
            # Build a remote transport
            try:
                trans = FlowBuilderTransport(hostIP, hostname)
                self.remoteTransports.append(trans)
                # Extract the flow builder proxy from the transport
                b = trans.getFlowBuilder()
            except FlowBuilderTransportError:
                raise EndpointCreationError()

        return b

    def createIfaceList(self, flowSpec):
        """Convert a TransferManager spec into a Flow ifaces tuple list"""
        flowIfaceList = []
        threads = flowSpec['threads']
        port = flowSpec['port']
        ifs = flowSpec['ifs']
        for iface in ifs:
            if 'numa' in flowSpec:
                numa = flowSpec['numa']
                flowIface = (iface, port, threads, numa)
            else:
                flowIface = (iface, port, threads)
            flowIfaceList.append(flowIface)
        return flowIfaceList
                
    def createEndpoints(self):
        """Create the endpoints for each source and sink"""
        rc = 0
        idx = 0
        for spec in self.sinks:
            try:
                b = self.buildEndpoint(spec)
            except EndpointCreationError:
                print('ERROR: Failed creating sink flow:', spec)
                rc += 1
                break

            ifaces = self.createIfaceList(spec)
            b.buildFlow(isSink=True,  
                        reqSize=self.transferRequestSize,
                        flowIdx=idx, numFlows=len(self.sinks),
                        ifaces=ifaces,
                        dioFlag=self.sinkDioFlag, 
                        serialFlag=self.sinkSerialFlag, 
                        verboseFlag=self.sinkVerboseFlag, 
                        timestampFlag=self.sinkTimestampFlag, 
                        xddPath=self.sinkXddPath)
            self.sinkFlows.append(b)
            self.allFlows.append(b)
            idx += 1
            
        idx = 0
        for spec in self.sources:
            try:
                b = self.buildEndpoint(spec)
            except EndpointCreationError:
                print('ERROR: Failed creating source flow:', spec)
                rc += 1
                break

            ifaces = self.createIfaceList(spec)
            b.buildFlow(isSink=False,
                        reqSize=self.transferRequestSize,
                        flowIdx=idx, numFlows=len(self.sources),
                        ifaces=ifaces,
                        dioFlag=self.sourceDioFlag, 
                        serialFlag=self.sourceSerialFlag, 
                        verboseFlag=self.sourceVerboseFlag, 
                        timestampFlag=self.sourceTimestampFlag, 
                        xddPath=self.sourceXddPath)
            self.sourceFlows.append(b)
            self.allFlows.append(b)
            idx += 1

        return rc

