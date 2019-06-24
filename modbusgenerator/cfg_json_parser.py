import json
import netifaces
import logging
from cidr_calculator import cidr_calculator
from node import node

class json_parser:
    def __init__(self, filename, test=False):
        self.logger = logging.getLogger('json_parser')
        if test:
            self.json_data = json.loads(filename)
        else:
            with open(filename) as json_file:
                self.json_data = json.load(json_file)
        self.cidr = self.json_data.get("CIDR Notation") and cidr_calculator(self.json_data.get("CIDR Notation"))
        if self.json_data.get("Virtual Interface") and self.json_data["Virtual Interface"] not in netifaces.interfaces():
            self.logger.error("Network interface %s not specified or invalid", self.json_data.get("Virtual Interface"))
            exit()

        self.process_nodes()

    def process_nodes(self):
        master_nodes = [] # list of unique master instances that will be created by the app
        outstation_nodes = [] # list of unique outstation instances that will be created by the app
        outstation_cidr_addrs = {} #outstation name (key) to cidr generated ip addresses(value). Not a complete list of outstations!
        outstation_set = set() #all unique outstation names connected to master nodes. Will be used to get details from cfg file
        ipaddr_outstation_map={} #ipaddress(key) to list of outstation names(value). Used so that we gather those outstations cfg info into one outstation instance for the app.

        #first we look at all the Master nodes and the outstations they are connected to.
        for connected_nodes in self.json_data["Master Nodes"]:
            try:
                master_name = connected_nodes["Name"] #must have name, else drop this item
                master_ip = connected_nodes.get("IP Address", self.cidr.get_addr())
            except:
                continue
            master_node = node(master_name, role = "Master", ipv4addr=master_ip)
            if not connected_nodes.get("IP Address"):
                master_node.allocate(self.json_data["Virtual Interface"])
            outstations = connected_nodes.get("Outstations", ())
            #Some checks- make sure all outstations this master is connected to are defined in configuration
            if any(name not in self.json_data for name in outstations):
                self.logger.error("Atleast one of the outstations is unspecified in the config file:%s" %outstations)
                exit()
            #Some more checks- if any ip address is specified for one outstation, it must be specified the same for them all
            # at this point, we dont have the ability to manually assign them to the same ip address. It is left for the user.
            # we do assign the same cidr address for outstations with unspecified addresses.
            if any([self.json_data[name].get("IP Address") for name in outstations]): # if any IP address is specified in the configuration for the outstations
                if all([self.json_data[name].get("IP Address") == self.json_data[outstations[0]].get("IP Address") for name in outstations]): #then all have to be the same
                    master_node.outstation_addr = self.json_data[outstations[0]].get("IP Address")
                else: #else need to specify configuration differently.
                    self.logger.error("Cannot have a master %s connect to different IP addresses. Check your configuration." %master_name)
                    exit()
            else: #no ip address specified, see if we can assign a vnic address without conflict
                assigned_ots = outstation_set & set(outstations) #intersection set of already processed outstations and current list of outstations
                already_assigned_addresses = set(outstation_cidr_addrs[name] for name in assigned_ots) #set of outstations, already assigned vnic addresses
                if len(already_assigned_addresses) > 1: #different vnics assigned, need to exit
                    self.logger.error("Too many unique vnics assigned, check configuration %s" %already_assigned_addresses)
                    exit()
                elif len(already_assigned_addresses) == 1: #get the assigned vnic
                    cidr_addr = outstation_cidr_addrs[assigned_ots.pop()]
                else:
                    cidr_addr = self.cidr.get_addr() #get a new vnic

                outstation_cidr_addrs.update(dict.fromkeys(outstations,cidr_addr))
                master_node.outstation_addr=cidr_addr
            #read any other configuration details that master needs to know about
            master_node.poll_dict = connected_nodes.get("Poll Interval",{}) and {p["Data type"]:p["Frequency"] for p in connected_nodes.get("Poll Interval")}
            master_node.simulate = connected_nodes.get("Simulate", True)
            master_node.outstation_info = self.get_outstation_datastore(outstations)
            master_node.port = self.get_outstation_port(outstations)

            outstation_set.update(outstations)
            self.logger.debug("Adding %s to list of nodes" %master_name)
            master_nodes.append(master_node)

        #for all the outstation nodes that the masters are connected to, collect them by ip address
        for name in outstation_set:
            ipaddr = self.json_data[name].get("IP Address") or outstation_cidr_addrs[name]
            ipaddr_outstation_map[ipaddr] = [name] if ipaddr not in ipaddr_outstation_map else ipaddr_outstation_map[ipaddr]+[name]

        # We dont assume that outstations that "a" master are connected to are the only ones at that ip address
        # Thats why we run the most of the same checks as on outstations connected to a master
        #now we can create an outstation node for each ip address
        for ipaddr in ipaddr_outstation_map:
            outstations = ipaddr_outstation_map[ipaddr]
            n = node(outstations, "Outstation", ipv4addr=ipaddr)
            n.port = self.get_outstation_port(outstations)
            n.simulate = self.get_outstation_sim(outstations)
            n.datastore = self.get_outstation_datastore(outstations)
            n.name_id_map = self.get_outstation_unitids(outstations)
            if not any([self.json_data[name].get("IP Address") for name in outstations]):
                n.allocate(self.json_data["Virtual Interface"])
            self.logger.debug("outstations %s has ip address:%s, port:%d"%(outstations, ipaddr, n.port))
            self.logger.debug("Adding %s to list of nodes" %outstations)
            outstation_nodes.append(n)
        self.masters = master_nodes
        self.outstations = outstation_nodes

    def get_outstation_datastore(self, names):
        return {self.json_data[name].get("Unit ID", 0):self.json_data[name].get("Data Count", 10) for name in names}

    def get_outstation_unitids(self, names):
        name_unitid_map = {name:self.json_data[name].get("Unit ID", 0) for name in names}
        unit_ids = name_unitid_map.values()
        if len(unit_ids) != len(set(unit_ids)):
            self.logger.error("Have duplicated unit ids:%s in the outstations:%s" %(unit_ids, names))
            exit()
        if int(max(unit_ids)) > 247: #maximum slave address http://www.modbus.org/docs/Modbus_over_serial_line_V1_02.pdf page 7
            self.logger.error("Cannot have unit id greater than 247. We have unit id:%d" %max(unit_ids))
            exit()
        return name_unitid_map

    def get_outstation_port(self, names):
        ports = [self.json_data[name].get("Port") for name in names]
        if len(set(ports)) > 1:
            self.logger.error("Outstations %s have different ports in cfg file:%s" %(names, ports))
            exit()
        return len(ports)==1 and ports[0] or 502

    def get_outstation_sim(self, names):
        simulate = [self.json_data[name].get("Simulate", True) for name in names]
        if len(set(simulate)) > 1:
            self.logger.error("Master cannot have a combination of real and simulated outstations %s at one IP address." %names)
            exit()
        return simulate[0]
