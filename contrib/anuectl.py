#!/usr/bin/env python
#
# Control script to set emulated delay on DG and XGem ANUEs
#

#
# Stupid python-isms to make code slightly more portable
#
from __future__ import print_function

#
# Import modules
#
import HTMLParser
import optparse
import httplib
import urllib
import sys

class AnueControllerFactory():
    """Abstract class for all ANUE Controller"""
    def __init__(self):
        pass

    def create(self, host):
        """ 
        This should search the index page to figure out if the ANUE is
        in XGEM mode, or DG mode, but for now this is easy enough.
        """
        ctl = None
        if "anue1" == host or "anue1.ccs.ornl.gov" == host:
            ctl = AnueDGController(host)
        elif "anue2" == host or "anue2.ccs.ornl.gov" == host:
            ctl = AnueDGController(host)
        elif "anue3" == host or "anue3.ccs.ornl.gov" == host:
            ctl = AnueXGemController(host)
        elif "anue4" == host or "anue4.ccs.ornl.gov" == host:
            ctl = AnueXGemController(host)
        return ctl
            

class AnueController(object):
    """Abstract interface for all ANUE Controllers"""

    def __init__(self):
        pass

    def setDelay(self, delay):
        """Set the delay for an ANUE by dividing the delay across each blade"""
        bladeDelay = delay / 2.0
        self._setBladeDelay(1, bladeDelay)
        self._setBladeDelay(3, bladeDelay)

    def showDelay(self):
        """Print the current ANUE delay to the screen"""
        b1_delay = self._getBladeDelay(1)
        b3_delay = self._getBladeDelay(3)
        print('Blade 1 delay:', b1_delay, 'ms')
        print('Blade 3 delay:', b3_delay, 'ms')
        print('ANUE delay:', b1_delay + b3_delay, 'ms')

    def _setBladeDelay(self, blade, delay):
        pass

    def _getBladeDelay(self, blade):
        return None

class AnueXGemController(AnueController):
    """Controller for 10Gig Ethernet ANUE (XGems)"""

    def __init__(self, host):
        self._host = host

    def _getBladeDelay(self, blade):
        """@return the blade delay screen scraped from HTML"""
        hc = httplib.HTTPConnection(self._host)
        hc.request('GET', 
                   '/xgem_profile_impairment.htm?profile=0&blade=' +str(blade))
        resp = hc.getresponse()
        data = resp.read()
        parser = AnueParser()
        parser.feed(data)
        return parser.get_delay()


    def _setBladeDelay(self, blade, delay):
        """
        Blade delay can be set by a simple HTTP POST command.  For example
        the following curl command seems to work:

        curl --data "html_delay_mode_type=1&html_delay_val=12.000000&
                    html_delay_units_type=0" 
             http://anue1.ccs.ornl.gov/XGEM_SET_IMPAIR?profile=0&blade=1
    
        But here we try to do that a little more cleanly
        """
        headers = {"Content-type": "application/x-www-form-urlencoded",
                   "Accept": "text/plain"}
        params = urllib.urlencode({'html_delay_mode_type' : '1',
                                   'html_delay_value' : str(delay),
                                   'html_delay_units_type' : '0'})
        hc = httplib.HTTPConnection(self._host)
        hc.request('POST', 
                   '/XGEM_SET_IMPAIR?profile=0&blade=' +str(blade), 
                   params, 
                   headers)
        pass

class AnueDGController(AnueController):
    """Controller for OC192 ANUE (DG)"""

    def __init__(self, host):
        self._host = host

    def _getBladeDelay(self, blade):
        """@return the blade delay screen scraped from HTML"""
        hc = httplib.HTTPConnection(self._host)
        hc.request('GET', '/DG_BLADE_CONTROLS_MAIN.HTM?blade=' + str(blade))
        resp = hc.getresponse()
        data = resp.read()
        parser = AnueParser()
        parser.feed(data)
        return parser.get_delay()

    def _setBladeDelay(self, blade, delay):
        """
        Blade delay can be set by a simple HTTP POST command.  For example
        the following curl command seems to work:

        curl --data "html_delay_mode_type=1&html_delay_val=12.000000&
              html_delay_units_type=0" http://anue1.ccs.ornl.gov/SET_D2?blade=1
    
        But here we try to do that a little more cleanly
        """
        headers = {"Content-type": "application/x-www-form-urlencoded",
                   "Accept": "text/plain"}
        params = urllib.urlencode({'html_delay_mode_type' : '1',
                                   'html_delay_value' : str(delay),
                                   'html_delay_units_type' : '0'})
        hc = httplib.HTTPConnection(self._host)
        hc.request('POST', '/SET_D2?blade=' + str(blade), params, headers)

class AnueParser(HTMLParser.HTMLParser):
    """HTML Parser for ANUE Blade info"""
    def get_delay(self):
        """@return the current blade delay"""
        return self._delay

    def handle_starttag(self, tag, attrs):
        """
        At present we only care about getting delay which is held in the
        value attr for an INPUT element named 'html_delay_val'
        """
        delayValue = None
        if 'input' == tag:
            foundDelayTag = False
            for name, value in attrs:
                if foundDelayTag and name == 'value':
                    delayValue = value
                elif name == 'name' and value == 'html_delay_val':
                    foundDelayTag = True

        if delayValue:
            self._delay = float(delayValue)
        return
        
        
def createOptionsParser():
    """Create an option parser for anuectl"""
    usage='usage: anuectl.py [Options] <host>'
    parser = optparse.OptionParser(usage=usage)
    parser.add_option('-d', '--delayms', dest='delayms',
                      action='store', type='float', metavar='MS',
                      help='RTT delay to set on emulator in milliseconds')
    return parser

def control_anue():
    """Set or show the ANUE delay"""
    parser = createOptionsParser()
    (opts, args) = parser.parse_args()
    if 1 > len(args):
        print('ERROR: ANUE hostname is required')
        parser.print_usage()
        return 1

    for host in args:
        ctl = AnueControllerFactory().create(host)
        if opts.delayms:
            ctl.setDelay(opts.delayms)
        ctl.showDelay()
            
def main():
    """Catch outermost exceptions"""
    try:
        rc = control_anue()
    except KeyboardInterrupt:
        print('Cancelled by user')
        rc = 1
        raise
    return rc

if __name__ == '__main__':
    rc = main()
    sys.exit(rc)
