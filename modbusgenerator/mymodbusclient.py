import socket

from pymodbus.constants import Defaults
from pymodbus.transaction import ModbusSocketFramer, ModbusBinaryFramer
from pymodbus.transaction import ModbusAsciiFramer, ModbusRtuFramer

from pymodbus.client.sync import ModbusTcpClient
#---------------------------------------------------------------------------#
# Logging
#---------------------------------------------------------------------------#
import logging
_logger = logging.getLogger(__name__)

class MyModbusClient(ModbusTcpClient):
    def __init__(self, host='127.0.0.1', port=Defaults.Port, framer=ModbusSocketFramer, src_ipaddr=None):
        super().__init__(host, port, framer)
        self.src_ipaddr=src_ipaddr

    def connect(self):
        ''' Connect to the modbus tcp server

        :returns: True if connection succeeded, False otherwise
        '''
        if self.socket: return True
        try:
            self.socket = socket.create_connection((self.host, self.port), Defaults.Timeout, (self.src_ipaddr, 46100))
        except socket.error as msg:
            _logger.error('Connection to (%s, %s) failed: %s' % \
                (self.host, self.port, msg))
            self.close()
        return self.socket != None
