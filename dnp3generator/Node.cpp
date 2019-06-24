#include "Node.h"
#include <iostream>

void Node::Allocate()
{
    std::string subcmd="ifconfig "+ this->vnic+ " " + this->local_IPAddress;
    std::cout << subcmd << "##########\n";
    int shell = system(subcmd.c_str());
    printf("SHELL RESPONSE:%d\n", shell);
}

// new way of allocating network interface alias
void Node::Allocate(const std::string& nic)
{
    //subcmd="ifconfig "+ this->vnic+ " " + this->local_IPAddress;
    std::string subcmd = "ip addr add " + this->local_IPAddress + " dev " + nic + " " + " label " + this->vnic;
//    std::cout << subcmd << "##########\n";
    int shell = system(subcmd.c_str());
//    printf("SHELL RESPONSE:%d\n", shell);
}

Node::Node() : port(20000), allowUnsolicited(false)
{
    this->dbSize["Binary Input"] = 0;
    this->dbSize["Double Binary Input"] = 0;
    this->dbSize["Analog Input"] = 0;
    this->dbSize["Counter"] = 0;
    this->dbSize["Frozen Counter"] = 0;
    this->dbSize["Binary Output"] = 0;
    this->dbSize["Analog Output"] = 0;
    this->dbSize["Time Interval"] = 0;

    this->evtBufferSize["Binary Input"] = 0;
    this->evtBufferSize["Double Binary Input"] = 0;
    this->evtBufferSize["Analog Input"] = 0;
    this->evtBufferSize["Counter"] = 0;
    this->evtBufferSize["Frozen Counter"] = 0;
    this->evtBufferSize["Binary Output"] = 0;
    this->evtBufferSize["Analog Output"] = 0;
}
Node::Node(std::string name, std::string role): Node()
{
    this->name = name;
    this->luaFileNames.push_back(name + ".lua"); //default lua file name is its name.lua. May be overridden subsequently in the config file
    this->role = role;
    msToSleep = role=="Master"? 1000*1000 : 1000 ; //default sleep-Master 1 sec, outstation 1 ms
}

Node::~Node() {
    if (vnic == "")
        return;

    std::string subcmd="ifconfig "+ this->vnic+ " down" ;
    std::cout << "*****************NODE " << this->name<< " IS KILLED, IP address:"<<this->local_IPAddress<<", vnic going down:"<<this->vnic<<std::endl;
    int shell = system(subcmd.c_str());
}
