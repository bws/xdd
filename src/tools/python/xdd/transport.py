
#
# Python stuff to improve portability
#
from __future__ import print_function

#
# Generic python packages used
#
import getpass
import os
import threading
import select
import SocketServer
import socket
import sys

#
# Python packages supplied and needed by xdd
#
import Pyro4
from xdd.core import XDDError
from xdd.constants import XDD_PYRO_URI_DELIMITER, XDD_PYTHONPATH
from xdd.flowbuilder import FlowBuilder

class FlowBuilderTransportError(XDDError):
    """Exception raised when the flow proxy encounters problems."""

class FlowBuilderTransportForwardingServer(SocketServer.TCPServer):
    """
    SSH forwarding server to forward traffic over the forwarded ssh channel
    """
    daemon_threads = True
    allow_reuse_address = True
    
    def setTransport(self, transport, remoteAddr):
        """Set instance variables used by the underlying handler""" 
        self.transport = transport
        self.remoteAddr = remoteAddr

class FlowBuilderTransportForwardingHandler(SocketServer.BaseRequestHandler):
    """
    Request handler that establishes the forwarding SSH tunnel and 
    passes traffic over the tunnel as it is received
    """
    def handle(self):
        # This method requires paramiko
        import paramiko
        
        # Determine the SSH transport, remote hostname, and remote port
        sshTransport = self.server.transport
        remoteAddr = self.server.remoteAddr

        # The first time this function is invoked is by the instantiator
        try:
            destAddr = remoteAddr
            srcAddr = self.server.server_address
            channel = sshTransport.open_channel('direct-tcpip', 
                                                destAddr, 
                                                srcAddr)
            #print("SSH Channel opened:", srcAddr, ' -> ', destAddr)
        except paramiko.SSHException, e:
            print("ERROR: Unable to open channel for the existing transport:",
                  e)
            return
        except Exception, e:
            print("ERROR: Unable to create SSH forward channel.")
            import sys
            eInfo = sys.exc_info()[0]
            print("Exception:", e)
            print("Info:", eInfo)
            return

        # Forward the data through this server
        while True:
            r,w,x = select.select([self.request, channel], [], [])
            if self.request in r:
                data = self.request.recv(1024)
                if 0 == len(data):
                    break
                channel.send(data)
            if channel in r:
                data = channel.recv(1024)
                if 0 == len(data):
                    break
                self.request.send(data)

        channel.close()
        self.request.close()
        #print("SSH Channel closed:", srcAddr, ' -> ', destAddr)
            

class FlowBuilderTransport:
    """
    Paramiko client that creates a FlowBuilderServer
    """
    def __init__(self, host, hostname, user=None):
        """Constructor"""
        # This method requires paramiko
        import paramiko

        # Start the remote service
        (ssh, uri) = self.createRemoteServer(host, hostname, user)
        self.host = host
        self.hostname = hostname
        self.user = user
        self.ssh = ssh
        self.remoteURI = uri

        # Port forward the remote service using SSH
        fields = uri.rsplit(':', 1)
        pyroPort = int(fields[1])
        (server, thread, port) = self.startLocalForwarder(pyroPort)
        self.remotePort = pyroPort
        self.localPort = port
        self.forwardingServer = server
        self.forwardingThread = thread

        # Modify the Pyro URI so that we use the locally forwarded server
        # and port rather than the remote end.  Since the host is already
        # 'localhost' on the remote side, the host is correct, just need
        # to use the locally forwarded port
        self.localURI = fields[0] + ':' + str(self.localPort)

        # Use the tunnel to retrieve the flow builder proxy
        #print("LOCAL URI: >", self.localURI, "<", sep='')
        flowBuilder = Pyro4.Proxy(self.localURI)
        self.flowBuilder = flowBuilder

        # Quickly test the flowbuilder
        self.flowBuilder.isReady()
        #print("FlowBuilder is ready:", self.flowBuilder.isReady())
        
    def shutdown(self):
        """
        Stop the remote pyro server, the local forwarding server and terminate 
        the ssh connection
        """
        try:
            self.flowBuilder.shutdown();
        except Pyro4.errors.ConnectionClosedError:
            # The only way to shutdown the server remotely is via the Pyro 
            # object.  So the return from the RMI fails.  Which is not ideal, 
            # but its relatively safe since we are on our way out anyway.
            pass
        self.forwardingServer.shutdown()
        self.forwardingThread.join()
        self.ssh.close()

    def createRemoteServer(self, host, hostname, user):
        """"Create the remote pyro server using paramiko"""
        #print('Creating remote XDD flow server on', host)
        uri = None
        ssh = paramiko.SSHClient()
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        try:
            # First try to use host-based or default authentication 
            hostkey = self._try_load_sshd_host_key()
            ssh.connect(host, username=user, hostkey=hostkey)
        except paramiko.AuthenticationException:
            import sys
            # Now give the user 2 tries with their password
            for i in range(0, 2):
                print(getpass.getuser(), '@', hostname, ' ', sep='', end='')
                sys.stdout.flush()
                pw = getpass.getpass()
                try:
                    ssh.connect(host, username=user, password=pw)
                    break
                except:
                    if 0 == i:
                        continue
                    print('Unable to authenticate SSH at', host)
                    import sys
                    e = sys.exc_info()[0]
                    print(e)
                    raise FlowBuilderTransportError()
        except paramiko.BadHostKeyException:
            print('Bad host key:', hostname, ' [', host, ']')
            raise FlowBuilderTransportError()
        except socket.gaierror:
            print('Unknown host/address:', hostname, ' [', host, ']')
            raise FlowBuilderTransportError()
        except:
            print('Unknown SSH error connecting to', hostname, ' [', host, ']')
            import sys
            e = sys.exc_info()[0]
            print(e)
            raise FlowBuilderTransportError()
        try:
            # Set the keep alive interval
            ssh.get_transport().set_keepalive(5)

            # Start the xdd flow server
            cmd = 'xddmcp-server'
            (stdin, stdout, stderr) = ssh.exec_command(cmd)
            
            # Convert the returned stuff into something useful
            channel = stdin.channel

            # Wait for the buffer to fill
            while (not channel.recv_ready()) and \
                  (not channel.exit_status_ready()):
                pass

            # If the command exited, something went wrong
            if channel.exit_status_ready():
                print("Error: Unable to start remote XDD flow server:", 
                      channel.recv_exit_status)
                for line in stdout:
                    print(hostname, "stdout:", line)
                for line in stderr:
                    print(hostname, "stderr", line)
            else:
                # Read stdout to find the delimited URI
                uri = None
                foundDelim = False;
                i = 0
                while None == uri and (not channel.exit_status_ready()):
                    line = stdout.readline()
                    #print("Line", line)
                    if line:
                        token = line.rstrip('\n')
                        if foundDelim:
                            uri = token
                            foundDelim = False
                            break
                        elif XDD_PYRO_URI_DELIMITER == token:
                            foundDelim = True 
                    else:
                        break
                    
                # If the URI is invalid, trigger an error
                if None == uri:
                    print("ERROR: XDD flow server terminated prematurely")
                    for line in stdout:
                        print(hostname, "stderr", line)
                    raise FlowBuilderTransportError()
                
        except paramiko.SSHException, e:
            print('SSH Exception', hostname, ' [', host, ']', e)
            raise FlowBuilderTransportError()

        # Return the SSH session and URI
        return (ssh, uri)

    def startLocalForwarder(self, pyroPort):
        """
        Establish an ssh tunnel and start a thread that forwards TCP
        requests on the local host to the server running on the remote host.
        """
        # Start a forwarding server and set SSH forwarding attributes
        pyroListenAddr = ('localhost', pyroPort)
        localListenAddr = ('localhost', 0)
        forwardServer = FlowBuilderTransportForwardingServer(
            localListenAddr, FlowBuilderTransportForwardingHandler)
        forwardServer.setTransport(self.ssh.get_transport(), pyroListenAddr)

        # Start the forwarding server in its own thread so we don't block
        # waiting to handle requests
        forwardThread = threading.Thread(target=forwardServer.serve_forever)
        forwardThread.daemon = True
        forwardThread.start()

        # Retrieve the local port for the forwarding server
        (host,port) = forwardServer.server_address
        return (forwardServer, forwardThread, port)

    
    def getFlowBuilder(self):
        """Return the flow builder object"""
        return self.flowBuilder

    def _try_load_sshd_host_key(self):
        """@return the SSH host key if it exists, None otherwise"""
        candidates = ['/etc/ssh/ssh_host_rsa_key.pub',
                      '/etc/ssh_host_rsa_key.pub']

        match = None
        for candidate in candidates:
            try:
                match = paramiko.opensshkey.load_pubkey_from_file(candidate)
                break
            except paramiko.SSHException:
               pass
        return match

