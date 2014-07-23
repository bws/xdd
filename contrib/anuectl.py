#!/usr/bin/env python

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

class ANUEParser(HTMLParser.HTMLParser):
    """Parser for ANUE Blade info"""
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
        
        
def createParser():
    """Create an option parser for anuectl"""
    usage='usage: anuectl.py [Options] <url>'
    parser = optparse.OptionParser(usage=usage)
    parser.add_option('-d', '--delayms', dest='delayms',
                      action='store', type='float', metavar='MS',
                      help='RTT delay to set on emulator in milliseconds')
    return parser

def get_blade_delay(url, blade):
    hc = httplib.HTTPConnection(url)
    hc.request('GET', '/DG_BLADE_CONTROLS_MAIN.HTM?blade=' + str(blade))
    resp = hc.getresponse()
    data = resp.read()
    parser = ANUEParser()
    parser.feed(data)
    return parser.get_delay()

def set_blade_delay(url, blade, delay):
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
    hc = httplib.HTTPConnection(url)
    hc.request('POST', '/SET_D2?blade=' + str(blade), params, headers)
    print('Blade', blade, 'delay: ', delay, 'ms')
    return 0

def set_anue_delay(url, delay):
    blade_delay = delay / 2.0
    set_blade_delay(url, 1, blade_delay)
    set_blade_delay(url, 3, blade_delay)
    print('ANUE Delay:', blade_delay + blade_delay)

def show_anue_delay(url):
    b1_delay = get_blade_delay(url, 1)
    b3_delay = get_blade_delay(url, 1)
    print('Blade 1 Delay:', b1_delay, 'ms')
    print('Blade 3 Delay:', b3_delay, 'ms')
    print('ANUE Delay:', b1_delay + b3_delay, 'ms')

def control_anue():
    parser = createParser()
    (opts, args) = parser.parse_args()
    if 1 > len(args):
        print('ERROR: ANUE URL is required')
        parser.print_usage()
        return 1

    for url in args:
        if opts.delayms:
            set_anue_delay(url, opts.delayms)
        else:
            show_anue_delay(url)
            
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
