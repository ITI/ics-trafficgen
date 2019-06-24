import subprocess
import logging

class node:
    vnictrack=0
    def __init__ (self, name, role, **kwargs):
        self.name = name
        self.role = role
        self.ipaddr = kwargs.get("ipv4addr", None)
        self.logger = logging.getLogger('node')

    def allocate(self, vnic):
        self.vnic = vnic + ":" + str(node.vnictrack)
        self.logger.debug("Going to allocate for %s, got vnic:%s, ip address:%s" %(self.name, self.vnic, self.ipaddr))
        subprocess.call(["ifconfig", self.vnic, self.ipaddr])
        node.vnictrack +=1

    def deallocate(interface):
        interface and subprocess.call(["ifconfig", interface, "down"])
