

#include "dnp3app.h"
#include "MasterStation.h"
#include "OutStation.h"
#include "MappingOutstation.h"
#include "Node.h"
#include "CfgJsonParser.h"

#include <iostream>
#include <fstream>
#include <thread>
#include <signal.h>
#include <algorithm>
#include <atomic>
#include <sstream>
#include <getopt.h>
#include <chrono>
#include <sys/stat.h>

#include <asiodnp3/DNP3Manager.h>
#include <asiodnp3/ConsoleLogger.h>

void signal_h(int signalType)
{
	printf("Caught Signal, Exiting %d\n", signalType);
	exit(EXIT_SUCCESS);
}

ThreadSafeUserInput luaSwitchObj;
int main(int argc, char *argv[])
{
    //Handle Ctrl-C event
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = signal_h;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    //Check we can run shell commands
    if (system(NULL))
       puts("OK");
    else
        exit(EXIT_FAILURE);

    //Parse command line options
    std::string cfgFileName = "Config.json";
	std::string pipeName = "dnp3pipe";
    int iflag;
    while ((iflag = getopt(argc, argv, "c:p:")) != -1)
    {
        switch (iflag)
        {
            case 'c':
                cfgFileName = optarg;
                break;
			case 'p':
				pipeName = optarg;
				break;
            default:
                printf("Unknown command line option. Use -c ConfigFile to create your stations.\n");
                exit(EXIT_FAILURE);
        }
    }
	/* Create the pipe for user input, abort the application if it already exists */
	struct stat buffer;
	if (stat (pipeName.c_str(), &buffer) == 0){
		printf("Pipe %s already exists. Please delete it and restart the application.\n", pipeName.c_str());
		return 0;
	}
	std::string subcmd = "mkfifo " + pipeName;
	int success = system(subcmd.c_str());
	if(success != 0){
		printf("Was not able to create the specified pipe. Aborting application.\n");
		return 0;
	}

	CfgJsonParser cfgReader = CfgJsonParser(cfgFileName);
    std::vector<std::unique_ptr<Node>>& nodes = cfgReader.GetConfiguredNodes();

	std::atomic<bool> quitFlag(false);
	std::set<std::string> luaTriggers;
	std::vector<std::thread> threads;
	std::vector<std::shared_ptr<MasterStation>> srcMasterStations;
	std::vector<std::shared_ptr<MappingOutstation>> mappedOutStations;

    unsigned int nrSupportedConcurrentThreads = std::max(std::thread::hardware_concurrency(), (unsigned int)1);
    asiodnp3::DNP3Manager manager(nrSupportedConcurrentThreads, asiodnp3::ConsoleLogger::Create());
    for(auto& vect : nodes)
    {
    	printf("\n>>>>Name:%s IPADDRESS:%s NIC:%s  Remote IP Address:%s localDNP3Addr:%i  RemoteDNP3Address:%i ROLE:%s\n", vect->name.c_str(), vect->local_IPAddress.c_str(), \
		 vect->vnic.c_str(), vect->remote_IPAddress.c_str(), vect->localDNP3Addr, vect->remoteDNP3Addr, vect->role.c_str());

		if(!vect->luaKeySwitch.empty()){
			luaTriggers.insert(vect->luaKeySwitch);
		}
    	if(vect->role == "Master")
    	{
    	    std::shared_ptr<MasterStation> p_master = std::make_shared<MasterStation>(std::move(vect), quitFlag);
			p_master->initialize();
            p_master->manager = &manager;
			if(!p_master->node->boundOutstations.empty()){
				srcMasterStations.push_back(p_master);
			}
			threads.push_back(std::thread(&MasterStation::run, p_master));
    	}
    	else if(vect->role == "Outstation")
    	{
			std::shared_ptr<OutStation> p_outstation(nullptr);
			if (!vect->dataSources.empty()){
				std::shared_ptr<MappingOutstation> mapped_station= std::make_shared<MappingOutstation>(std::move(vect), quitFlag);
				mapped_station->initialize();
				mappedOutStations.push_back(mapped_station);
				p_outstation = mapped_station;
			}
			else
				p_outstation = std::make_shared<OutStation>(std::move(vect), quitFlag);
			p_outstation->manager = &manager;

			threads.push_back(std::thread(&OutStation::run, p_outstation));
    	}
    }
	//connect the mapped master and bound outstations

	for(auto masterStation : srcMasterStations) {
		for(auto boundOutStnName : masterStation->node->boundOutstations) {
			for(auto outStation : mappedOutStations) {
				if(outStation->node->name == boundOutStnName) {
					masterStation->destStations.push_back(outStation);
                    std::cout<<"Adding station " << outStation->node->name << " to destStations for master " << masterStation->node->name <<"\n";
				}
			}
		}
		masterStation->UpdateDestinations();//TODO TEMP NEED TO MAKE SURE IT IS TIMELY
    }

	std::ifstream pipe;
	std::string usr_input;
	std::cout << "Enter a command" << std::endl;
	std::cout << "x - exits program" << std::endl;

	std::chrono::milliseconds waitTime_sec(500);
	std::string exitStr("x");
	while (true)
    {
		if(!pipe.is_open()){
			pipe.open(pipeName.c_str(), std::ifstream::in);
			if(!pipe){
				printf("Error opening pipe! Will not be able to process user input.\n");
			}
		}
		std::getline(pipe, usr_input); //wait here for input from user on the pipe
		if (usr_input == exitStr){ // C++ destructor on DNP3Manager cleans everything up for you
			quitFlag=true;
			for(auto& t : threads)
				t.join();
			return 0;
		}
		else if (luaTriggers.count(usr_input) > 0){ //found a switching character that some outstation is interested in
			luaSwitchObj.unconditionalWriter(usr_input);
			pipe.close();
		}
		else{ //unrecognized string entered by user
			pipe.close();
			std::this_thread::sleep_for(waitTime_sec);
        }
    }
    return 0;
}
