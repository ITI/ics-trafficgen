import netaddr
import logging

class cidr_calculator:
    def __init__(self, cidr_notation):
        self.addr_generator = netaddr.IPNetwork(cidr_notation).iter_hosts()
        self.logger = logging.getLogger('CIDR Calculator')

    def get_addr(self):
        try:
            return str(next(self.addr_generator))
        except StopIteration:
            self.logger.exception("Not able to generate any more addresses. Change your CIDR subnet mask")
            exit()
