#!/usr/bin/env python3

import time
import argparse
import os
import signal
import threading

from master_station import master_station
from outstation import outstation
from cfg_json_parser import json_parser
import logging
import logging.handlers as Handlers

thread_stop = threading.Event()

def signal_handler(signal, frame):
    print('Cleaning up...')
    #TODO: Signal threads to end
    thread_stop.set()
    print ('Waiting for threads to finish')
    time.sleep(7)
    os._exit(0) #Hack because serve_forever callback is broken

def main():
    parser = argparse.ArgumentParser(description='Generate ModBus Traffic.')
    parser.add_argument('-c', '--config', default = "config.json", help='Configuration File')
    args = parser.parse_args()

    if not os.path.isfile(args.config):
        print("Invalid Configuration File")
        exit()

    logging.basicConfig()
    log = logging.getLogger()
    log.setLevel(logging.INFO)

    parser = json_parser(args.config)

    for node in parser.outstations:
        if node.simulate:
            o = outstation(node, thread_stop)
            o.start()

    for node in parser.masters:
        if node.simulate:
            m = master_station(node, thread_stop)
            m.start()

    signal.signal(signal.SIGINT, signal_handler)
    signal.pause()

if __name__ == '__main__':
    main()
