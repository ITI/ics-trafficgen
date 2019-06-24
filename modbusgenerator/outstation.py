import threading
import time
import socketserver
import logging

import lupa

from pymodbus.server.sync import ModbusSocketFramer, ModbusTcpServer
from pymodbus.device import ModbusDeviceIdentification
from pymodbus.datastore import ModbusSequentialDataBlock
from pymodbus.datastore import ModbusSlaveContext, ModbusServerContext

from node import node
from threadsafe_datastore import ThreadSafeDataBlock

start_address = 0x01

class outstation(threading.Thread):
    def __init__(self, node, thread_stop):
        threading.Thread.__init__(self)
        #Data Store for the server instance. Only set by connecting clients
        store={}
        for unit_id, data_count in node.datastore.items():
            if len(node.datastore) == 1:
                store = ModbusSlaveContext(
                    di = ThreadSafeDataBlock(ModbusSequentialDataBlock(start_address, [0x00]*data_count)),
                    co = ThreadSafeDataBlock(ModbusSequentialDataBlock(start_address, [0x00]*data_count)),
                    hr = ThreadSafeDataBlock(ModbusSequentialDataBlock(start_address, [0x00]*data_count)),
                    ir = ThreadSafeDataBlock(ModbusSequentialDataBlock(start_address, [0x00]*data_count)))
            else:
                store[unit_id] = ModbusSlaveContext(
                    di = ThreadSafeDataBlock(ModbusSequentialDataBlock(start_address, [0x00]*data_count)),
                    co = ThreadSafeDataBlock(ModbusSequentialDataBlock(start_address, [0x00]*data_count)),
                    hr = ThreadSafeDataBlock(ModbusSequentialDataBlock(start_address, [0x00]*data_count)),
                    ir = ThreadSafeDataBlock(ModbusSequentialDataBlock(start_address, [0x00]*data_count)))

        self.context = ModbusServerContext(slaves=store, single= len(node.datastore)==1 and True or False)
        self.outstation_stop = thread_stop

        self.identity = ModbusDeviceIdentification()
        self.identity.VendorName  = 'ITI'
        self.identity.ProductCode = 'PM'
        self.identity.VendorUrl   = 'code.iti.illinois.edu'
        self.identity.ProductName = 'Server Instance'
        self.identity.ModelName   = 'ITI Test'
        self.identity.MajorMinorRevision = '1.0'
        self.node = node
        self.logger = logging.getLogger('Outstation(ModbusTcpServer)')
        #self.logger.setLevel(logging.DEBUG)

    def run(self):
        print("Starting ModBus Server: {}:{}".format(self.node.ipaddr, self.node.port))
        framer = ModbusSocketFramer
        #TODO-REMOVE ALLOW_ADDRESS_REUSE IN FINAL CODE.
        socketserver.TCPServer.allow_reuse_address = True
        self.server = ModbusTcpServer(self.context, framer, self.identity, address=(self.node.ipaddr, self.node.port))
        for name in self.node.name:
            t = threading.Thread(target=ping_outstation, args=(self,name))
            t.start()
        self.server.serve_forever()

def ping_outstation(outstation, outstn_name):
    L = lupa.LuaRuntime()
    lua_script = open(outstn_name + ".lua").read()
    L_func = L.execute(lua_script)
    g = L.globals()
    #print(g.generate_data)
    lua_pymod_dtype_map = {'Discretes Input':'d', 'Coils':'c', 'Input Registers':'i', 'Holding Registers':'h'}

    while not outstation.outstation_stop.is_set():
        time.sleep(1)
        unit_id = outstation.node.name_id_map[outstn_name]
        try:
            slave = outstation.context[unit_id]
        except ParameterException as err:
            outstation.logger.debug("context.py returned:%s" %err)
            continue
        data_count = outstation.node.datastore[unit_id]

        data = g.generate_data()
        if not data:
            continue
        for d_type in lua_pymod_dtype_map.keys():
            if d_type not in data or not data[d_type]:
                continue
            pymodbus_d_type = lua_pymod_dtype_map[d_type]
            values = list(data[d_type].values())
            if len(values) == 0 or len(values) != data_count:
                continue
            dtype_datablock = slave.store.get(pymodbus_d_type)
            outstation.logger.debug("Updating Outstation:" + outstn_name + ", datacount:"+ str(data_count) + " dtype:"+ d_type + ", new values : " + str(values))
            dtype_datablock.setValues(start_address, values)
    outstation.logger.info("stopping server %s" %outstn_name)
    outstation.server.shutdown()
    outstation.server.server_close()
    if hasattr(outstation.node, 'vnic'):
        node.deallocate(outstation.node.vnic)
