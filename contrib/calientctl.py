#!/usr/bin/env python
#
# Control script to manage cross connections on a Calient switch
#

#
# Stupid python-isms to make code slightly more portable
#
from __future__ import print_function

#
# Import modules
#
import HTMLParser
import base64
import getpass
import optparse
import os
import httplib
import urllib
import urllib2
import sys
import time

class CalientController(object):
    """Abstract interface for all Calient Switch Controls"""

    def __init__(self):
        """Initialization"""
        self._conns = None

    def showConnections(self):
        """Print all of the available connections to the screen"""

        print('Port \t Name            \t XConnect')
        print('---- \t --------------- \t --------')
        self._conns = self._getConnections()
        for c in self._conns:
            (port, group, power, circuit, name) = c
            print(port, '\t', name, '\t', circuit)

    def showCrossConnections(self):
        """Print all of the available connections to the screen"""

        print('Cross Connections')
        print('-----------------')
        self._conns = self._getConnections()
        for c in self._conns:
            (port, group, power, circuit, name) = c
            if circuit:
                print(circuit)

    def refreshCrossConnections(self):
        """Refresh cross connections"""
        if self._conns is None:
            self._conns = self._getConnections()

        # Find the ports for each circuit
        circuitPorts = {}
        conns = self._getConnections()
        for c in conns:
            (port, group, power, circuit, name) = c
            if circuit:
                if circuit in circuitPorts:
                    circuitPorts[circuit].append(port)
                else:
                    circuitPorts[circuit] = [port]

        # Refresh each identified circuit
        for circ, ports in circuitPorts.items():
            print('Refreshing XConnect:', circ, circuitPorts[circ])
            self._refreshCrossConnect(ports[0], ports[1])

    def crossConnect(self, conn1, conn2):
        """Cross connect conn1 and conn2"""
        if self._conns is None:
            self._conns = self._getConnections()
        (port1, _, _, circuit1, name1) = self.getConnection(conn1)
        (port2, _, _, circuit2, name2) = self.getConnection(conn2)
        if circuit1 or circuit2:
            print('ERROR: Connection already exists')
        else:
            circuit = name1 + '-' + name2
            print('Adding circuit', circuit)
            self._addCrossConnect(name1, name2, circuit)

    def deleteCrossConnect(self, conn):
        """Cross connect conn1 and conn2"""
        if self._conns is None:
            self._conns = self._getConnections()

        # Get the first port
        (port1, _, _, circuit1, name1) = self.getConnection(conn)
        if not circuit1:
            print('ERROR: No XConnect exists for', port1)
        else:            
            # Get the second port
            (port1, port2) = self.getPorts(circuit1)
            self._removeCrossConnect(port1, port2)

    def getConnection(self, conn):
        """
        Lookup port by portId, shorthand, portname, or circuit.  Shorthand 
        means that instead of typing 1.1.3, the user can simply supply 113.
        """
        if self._conns is None:
            self._conns = self._getConnections()

        # First loopthru by port id and shorthand
        for c in self._conns:
            (portId, _, _, _, _) = c
            shorthand = portId.translate(None, '.')
            if conn == portId or conn == shorthand:
                return c

        # Search by port name
        for c in self._conns:
            (_, _, _, _, name) = c
            if conn == name:
                return c
            
        # Search by circuit name
        for c in self._conns:
            (_, _, _, circuit, _) = c
            if conn == circuit:
                return c
            
        return None

    def getPorts(self, circuit):
        """@return tuple with both ports for the circuit"""
        if self._conns is None:
            self._conns = self._getConnections()

        # Find the ports for each circuit
        circuitPorts = {}
        conns = self._getConnections()
        for c in conns:
            (port, group, power, circuit, name) = c
            if circuit:
                if circuit in circuitPorts:
                    circuitPorts[circuit].append(port)
                else:
                    circuitPorts[circuit] = [port]

        # Return the requested circuit ports
        return (circuitPorts[circuit][0], circuitPorts[circuit][0])
        
    def _getConnections(self):
        """@return map of port, connection name pairs"""
        print('ERROR: _getConnections not provided')
        return None

    def _addCrossConnect(self, portName1, portName2, circuitName):
        """Cross connect ports on the calient"""
        print('ERROR: _addCrossConnect not provided')
        
    def _refreshCrossConnect(self, port1, port2):
        """Cross connect ports on the calient"""
        print('ERROR: _refreshCrossConnect not provided')
        
    def _removeCrossConnect(self, port1, port2):
        """Cross connect ports on the calient"""
        print('ERROR: _removeCrossConnect not provided')
        
    

class CalientHTTPController(CalientController):
    """Controller for Calient Optical Switch"""

    def __init__(self, host, user, password):
        CalientController.__init__(self)
        self._host = host
        self._auth = base64.encodestring(user + ':' + password)
        self._sessionId = None
        self._renewSession()

    def _renewSession(self):
        """@return the sessionid for the renewed session"""
        if self._sessionId is None:
            cookie = None
            while cookie is None:
                try:
                    # Request a session cookie
                    url = 'http://' + self._host + '/index.html'
                    request = urllib2.Request(url)
                    request.add_header('User-Agent', 'calientctl')
                    request.add_header('Connection', 'keep-alive')
                    request.add_header('Authorization', 'Basic ' + self._auth)
                    request.add_header('Cookie', 'sessionid=invalid')
                    r = urllib2.urlopen(request)
                    cookie = r.headers['set-cookie']
                except urllib2.HTTPError:
                    print('ERROR: Check user/password for', self._host)
                    pass
                if cookie is None:
                    print('Unable to establish session, trying again in 2 secs')
                    time.sleep(2)
            self._sessionId = cookie
        return self._sessionId

    def _getConnections(self):
        """@return map of connection name/port number pairs"""
        # Make sure there is a valid session
        self._renewSession()

        # Build the HTTP request header and parameters
        url = 'http://%s/port/portSummaryTable.html?shelf=1' % self._host
        request = urllib2.Request(url)
        request.add_header('User-Agent', 'calientctl')
        request.add_header('Connection', 'Keep-Alive')
        request.add_header('Authorization', 'Basic ' + self._auth)
        request.add_header('Cookie', self._sessionId)
        r = urllib2.urlopen(request)
        data = r.read()

        # Parse the HTML to get the port map
        parser = CalientPortTableParser()
        parser.feed(data)
        return parser.getConnections()

    def _addCrossConnect(self, portName1, portName2, circuitName):
        """Cross connect ports on the calient"""
        # Make sure there is a valid session
        self._renewSession()

        # Build the HTTP request header and parameters
        postData = {'groupName2' : 'SYSTEM',
                    'connName2' : circuitName,
                    'direction2' : 'Bi',
                    'lightBand2' : 'W',
                    'from2' : portName1,
                    'to2' : portName2,
                    'defDirection2' : 'Bi',
                    'autoFocusOnOff' : 'AFEnabled',
                    'noLightConnOnOff' : 'NLCDisabled'}

        url = 'http://%s/xConnects/xConnectsAddConnForm.html/bogusAction' % self._host
        data = urllib.urlencode(postData)
        request = urllib2.Request(url, data)
        request.add_header('User-Agent', 'calientctl')
        request.add_header('Connection', 'Keep-Alive')
        request.add_header('Authorization', 'Basic ' + self._auth)
        request.add_header('Cookie', self._sessionId)
        r = urllib2.urlopen(request)
        if 200 != r.getcode():
            print('ERROR cross connecting', portName1, portName2)
        pass

    def _removeCrossConnect(self, port1, port2):
        """Cross connect ports on the calient"""
        # Make sure there is a valid session
        self._renewSession()

        # Build the HTTP request header and parameters
        params = 'xgroup=SYSTEM&xconnect=%s-%s&actionType=Retry' % conn1, conn2
        url = 'http://%s/xConnects/processConnection.html' % self._host
        url += '?' + params
        request = urllib2.Request(url)
        request.add_header('User-Agent', 'calientctl')
        request.add_header('Connection', 'Keep-Alive')
        request.add_header('Authorization', 'Basic ' + self._auth)
        request.add_header('Cookie', self._sessionId)
        r = urllib2.urlopen(request)
        if 200 != r.getcode():
            print('ERROR removing cross connect:', port1, port2)

    def _refreshCrossConnect(self, port1, port2):
        """Cross connect ports on the calient"""
        # Make sure there is a valid session
        self._renewSession()

        # Build the HTTP request header and parameters
        xconnect = port1 + '-' + port2
        params = 'xgroup=SYSTEM&xconnect=' + xconnect + '&actionType=Retry'
        url = 'http://%s/xConnects/processConnection.html' % self._host
        url += '?' + params
        request = urllib2.Request(url)
        request.add_header('User-Agent', 'calientctl')
        request.add_header('Connection', 'Keep-Alive')
        request.add_header('Authorization', 'Basic ' + self._auth)
        request.add_header('Cookie', self._sessionId)
        r = urllib2.urlopen(request)
        if 200 != r.getcode():
            print('ERROR cross connecting', port1, port2)

class CalientPortTableParser(HTMLParser.HTMLParser):

    def __init__(self):
        """Initialize the maps"""
        HTMLParser.HTMLParser.__init__(self)
        self._connections = []

    def getConnections(self):
        """
        @return a list of connection tuples
          the tuple contains (port, group, power, circuit, name)
        """
        return self._connections

    def handle_data(self, data):
        """Port descriptions are in the data stream"""

        port = ''
        group = ''
        power = ''
        circuit = ''
        name = ''
        for line in data.splitlines():
            if -1 != line.find('PortDesc ='):
                fields = line.split('"')
                port = fields[1]
            elif -1 != line.find('PortGroup ='):
                fields = line.split('"')
                group = fields[1]
            elif -1 != line.find('\tinPwrVal ='):
                fields = line.split('"')
                power = fields[1]
            elif -1 != line.find('CircuitID ='):
                fields = line.split('"')
                circuit = fields[1]
            elif -1 != line.find('PortName ='):
                fields = line.split('"')
                name = fields[1]
            elif -1 != line.find('new portsummary'):
                conn = (port, group, power, circuit, name)
                self._connections.append(conn)
        
def createOptionsParser():
    """Create an option parser for calientctl"""
    usage='usage: calientctl.py [Options] <host>'
    parser = optparse.OptionParser(usage=usage)
    parser.add_option('-d', '--delete', dest='delete',
                      action='store', type='string', metavar='conn',
                      help='Cross connection to delete (port, name, shorthand, or circuit)')
    parser.add_option('-p', '--password', dest='password', default='',
                      action='store', type='string', metavar='pw',
                      help='Calient access password')
    parser.add_option('-r', '--refresh', dest='refresh', default=False,
                      action='store_true',
                      help='Refresh all cross connections')
    parser.add_option('-u', '--user', dest='user', default=os.getlogin(),
                      action='store', type='string', metavar='user',
                      help='Calient access password')
    parser.add_option('-x', '--xconnect', dest='xc',
                      action='store', type='string', metavar='conn,conn',
                      help='Pair of connections (port, name, or shorthand) to cross connect')
    return parser

def control_calient():
    """Refresh, set or show the Calient cross connections"""
    parser = createOptionsParser()
    (opts, args) = parser.parse_args()
    if 1 > len(args):
        print('ERROR: calient hostname is required')
        parser.print_usage()
        return 1

    # For each host, perform the commands
    for host in args:
        ctl = CalientHTTPController(host, opts.user, opts.password)
        if opts.refresh:
            ctl.refreshCrossConnections()
        if opts.xc:
            c1,c2 = opts.xc.split(',')
            ctl.crossConnect(c1, c2)
        if opts.delete:
            ctl.deleteCrossConnect(opts.delete)

        if not opts.refresh and not opts.xc:
            ctl.showConnections()
        else:
            ctl.showCrossConnections()
            
def main():
    """Catch outermost exceptions"""
    try:
        rc = control_calient()
    except KeyboardInterrupt:
        print('Cancelled by user')
        rc = 1
        raise
    return rc

if __name__ == '__main__':
    rc = main()
    sys.exit(rc)
