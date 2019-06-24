import threading
import random
import time
import logging
import copy

from node import node
from mymodbusclient import MyModbusClient

start_address = 0x00

class master_station(threading.Thread):
    def __init__(self, node, thread_stop):
        threading.Thread.__init__(self)
        self.masterstop = threading.Event()
        self.node = node
        self.masterstop = thread_stop
        self.logger = logging.getLogger('Master(ModbusTcpClient)')
        #self.logger.setLevel(logging.DEBUG)

    def run(self):
        self.logger.debug("and we're off!")
        self.logger.info("Master %s at ip address %s connected to %s on port %s" %(self.node.name, self.node.ipaddr, self.node.outstation_addr, self.node.port))
        master = MyModbusClient(self.node.outstation_addr, self.node.port, src_ipaddr = self.node.ipaddr)
        master.connect()
        outstation_unitids = list(self.node.outstation_info.keys())
        sleep_dict = copy.deepcopy(self.node.poll_dict)
        while(not self.masterstop.is_set()):
            if not sleep_dict:
                time.sleep(1) #avoid hogging cpu if no polling specified, TODO- break instead??
                continue
            sleep_time = min(sleep_dict.values())
            self.logger.debug("Before, dict is {}, sleeping for {} secs".format(sleep_dict, sleep_time))
            time.sleep(sleep_time)
            pollfuncs = [key for key, value in sleep_dict.items() if value == sleep_time]
            self.logger.debug("Polling outstations for {}".format(pollfuncs))
            self.poll_outstations(master, pollfuncs, outstation_unitids)
            #update sleep_dict now.
            for k,v in sleep_dict.items():
                sleep_dict[k] = v - sleep_time if v-sleep_time > 0 else self.node.poll_dict[k]
            self.logger.debug("After modification, sleep_dict is {}".format(sleep_dict))
        self.logger.info("stopping master {}".format(self.node.name))
        if hasattr(self.node, 'vnic'):
            node.deallocate(self.node.vnic)
        master.close()

    def poll_outstations(self, master, pollfuncs, outstation_unitids):
        for unitid in outstation_unitids:
            data_count = self.node.outstation_info[unitid]
            self.logger.debug("Got id:" +str(unitid) + " with datacount:"+str(data_count))
            for fn in pollfuncs:
                output = ""
                if fn == "Coils":
                    try:
                         output = master.read_coils(start_address, data_count, unit=unitid)
                         self.logger.info("%s read coils to server %s:%s" %(self.node.name, unitid, output))
                    except Exception as ex:
                        self.logger.exception("Cannot read coils:%s", ex)
                elif fn == "Discretes Input":
                    try:
                         output = master.read_discrete_inputs(start_address, data_count, unit=unitid)
                         self.logger.info("%s read discrete inputs to server %s:%s" %(self.node.name, unitid, output))
                    except Exception as ex:
                        self.logger.exception("Cannot read discrete inputs:%s", ex)
                elif fn == "Holding Registers":
                    try:
                         output = master.read_holding_registers(start_address, data_count, unit=unitid)
                         self.logger.info("%s read holding registers to server %s:%s" %(self.node.name, unitid, output))
                    except Exception as ex:
                        self.logger.exception("Cannot read holding registers:%s", ex)
                elif fn == "Input Registers":
                    try:
                          output = master.read_input_registers(start_address, data_count, unit=unitid)
                          self.logger.info("%s read input registers to server %s:%s"%(self.node.name, unitid, output))
                    except Exception as ex:
                        self.logger.exception("Cannot read input registers:%s", ex)
                self.logger.info("Output:%s" %output)
