#include"Station.h"
#include <iostream>
#include <fstream>

/***************** Analog Input variations ***************************/
std::map <int, opendnp3::StaticAnalogVariation> sAnalogInputVarMap = {
	{1, opendnp3::StaticAnalogVariation::Group30Var1},
	{2, opendnp3::StaticAnalogVariation::Group30Var2},
	{3, opendnp3::StaticAnalogVariation::Group30Var3},
	{4, opendnp3::StaticAnalogVariation::Group30Var4},
	{5, opendnp3::StaticAnalogVariation::Group30Var5},
	{6, opendnp3::StaticAnalogVariation::Group30Var6}
};
std::map <int, opendnp3::EventAnalogVariation> eAnalogInputVarMap = {
	{1, opendnp3::EventAnalogVariation::Group32Var1},
	{2, opendnp3::EventAnalogVariation::Group32Var2},
	{3, opendnp3::EventAnalogVariation::Group32Var3},
	{4, opendnp3::EventAnalogVariation::Group32Var4},
	{5, opendnp3::EventAnalogVariation::Group32Var5},
	{6, opendnp3::EventAnalogVariation::Group32Var6},
	{7, opendnp3::EventAnalogVariation::Group32Var7},
	{8, opendnp3::EventAnalogVariation::Group32Var8}
};
/***************** Counter variations ***************************/
std::map <int, opendnp3::StaticCounterVariation> sCounterVarMap = {
	{1, opendnp3::StaticCounterVariation::Group20Var1},
	{2, opendnp3::StaticCounterVariation::Group20Var2},
	{5, opendnp3::StaticCounterVariation::Group20Var5},
	{6, opendnp3::StaticCounterVariation::Group20Var6}
};
std::map <int, opendnp3::EventCounterVariation> eCounterVarMap = {
	{1, opendnp3::EventCounterVariation::Group22Var1},
	{2, opendnp3::EventCounterVariation::Group22Var2},
	{5, opendnp3::EventCounterVariation::Group22Var5},
	{6, opendnp3::EventCounterVariation::Group22Var6}
};
/***************** Frozen Counter variations ***************************/
std::map <int, opendnp3::StaticFrozenCounterVariation> sFrozenCounterVarMap = {
	{1, opendnp3::StaticFrozenCounterVariation::Group21Var1},
	{2, opendnp3::StaticFrozenCounterVariation::Group21Var2},
	{5, opendnp3::StaticFrozenCounterVariation::Group21Var5},
	{6, opendnp3::StaticFrozenCounterVariation::Group21Var6},
	{9, opendnp3::StaticFrozenCounterVariation::Group21Var9},
	{10, opendnp3::StaticFrozenCounterVariation::Group21Var10}
};
std::map <int, opendnp3::EventFrozenCounterVariation> eFrozenCounterVarMap = {
	{1, opendnp3::EventFrozenCounterVariation::Group23Var1},
	{2, opendnp3::EventFrozenCounterVariation::Group23Var2},
	{5, opendnp3::EventFrozenCounterVariation::Group23Var5},
	{6, opendnp3::EventFrozenCounterVariation::Group23Var6}
};
/***************** Binary Input variations ***************************/
std::map <int, opendnp3::StaticBinaryVariation> sBinaryVarMap = {
	{1, opendnp3::StaticBinaryVariation::Group1Var1},
	{2, opendnp3::StaticBinaryVariation::Group1Var2}
};
std::map <int, opendnp3::EventBinaryVariation> eBinaryVarMap = {
	{1, opendnp3::EventBinaryVariation::Group2Var1},
	{2, opendnp3::EventBinaryVariation::Group2Var2},
	{3, opendnp3::EventBinaryVariation::Group2Var3}
};
/***************** Double Binary Input variations ********************/
std::map <int, opendnp3::StaticDoubleBinaryVariation> sDoubleBinaryVarMap = {
	{2, opendnp3::StaticDoubleBinaryVariation::Group3Var2}
};
std::map <int, opendnp3::EventDoubleBinaryVariation> eDoubleBinaryVarMap = {
	{1, opendnp3::EventDoubleBinaryVariation::Group4Var1},
	{2, opendnp3::EventDoubleBinaryVariation::Group4Var2},
	{3, opendnp3::EventDoubleBinaryVariation::Group4Var3}
};
/***************** Event Class Map ********************/
std::map <std::string, opendnp3::PointClass> pointClassVarMap = {
    {"0", opendnp3::PointClass::Class0},
	{"1", opendnp3::PointClass::Class1},
	{"2", opendnp3::PointClass::Class2},
	{"3", opendnp3::PointClass::Class3},
};

Station::Station(std::unique_ptr<Node> node, std::atomic<bool> &quitFlag):
quitFlag(quitFlag), node(std::move(node)), currentLuaFileIndex(0), localLuaFlag(0)
{}

Station::~Station()
{
    std::cout<<"************Destructor called Station with node "<<node->name<<std::endl;
}

bool Station::FileExists(const char *fileName)
{
    std::ifstream infile(fileName, std::ifstream::in);
    return infile.good();
}

std::string Station::GetNextLuaFile(){
    if(currentLuaFileIndex < node->luaFileNames.size()-1){
        currentLuaFileIndex += 1;
    }
    else{
        currentLuaFileIndex=0;
    }
    return node->luaFileNames[currentLuaFileIndex];
}

void Station::ConfigureDatabase(asiodnp3::DatabaseConfig &view)
{
	// example of configuring analog index 0 for Class2 with floating point variations by default
    //	view.analogs[0].variation = opendnp3::StaticAnalogVariation::Group30Var5;
    //	view.analogs[0].metadata.clazz = opendnp3::PointClass::Class2;
    //	view.analogs[0].metadata.variation = opendnp3::EventAnalogVariation::Group32Var7;

	for(auto dp : node->dataPoints) {
		//dpTypeSizeMap[dp.pointType] = node->dbSize[dp.pointType];//OUR CHECK FOR LUA RETURNED VALUES
		if (dp.pointType == "Analog Input") {
			try{
				sAnalogInputVarMap.at(dp.sVariation);
			} catch (const std::out_of_range& ) {
				dp.sVariation = 5;
			}
			try{
				eAnalogInputVarMap.at(dp.eVariation);
			} catch (const std::out_of_range& ) {
				dp.eVariation = 7;
			}
			view.analog[dp.index].svariation = sAnalogInputVarMap[dp.sVariation];
			view.analog[dp.index].clazz = pointClassVarMap[std::to_string(dp.eventClass)];
			view.analog[dp.index].evariation = eAnalogInputVarMap[dp.eVariation];
			dpTypeIndexMap[dp.pointType].insert(dp.index);
			if(dp.deadband>0){
				view.analog[dp.index].deadband = dp.deadband;
			}
		}
		else if (dp.pointType == "Binary Input"){
			try{
				sBinaryVarMap.at(dp.sVariation);
			} catch (const std::out_of_range& ) {
				dp.sVariation = 1;
			}
			try{
				eBinaryVarMap.at(dp.eVariation);
			} catch (const std::out_of_range& ) {
				dp.eVariation = 1;
			}
			view.binary[dp.index].svariation = sBinaryVarMap[dp.sVariation];
			view.binary[dp.index].clazz = pointClassVarMap[std::to_string(dp.eventClass)];
			view.binary[dp.index].evariation = eBinaryVarMap[dp.eVariation];
			dpTypeIndexMap[dp.pointType].insert(dp.index);
		}
		else if (dp.pointType == "Double Binary Input"){
			try{
				sDoubleBinaryVarMap.at(dp.sVariation);
			} catch (const std::out_of_range& ) {
				dp.sVariation = 2;
			}
			try{
				eDoubleBinaryVarMap.at(dp.eVariation);
			} catch (const std::out_of_range& ) {
				dp.eVariation = 1;
			}
			view.doubleBinary[dp.index].svariation = sDoubleBinaryVarMap[dp.sVariation];
			view.doubleBinary[dp.index].clazz = pointClassVarMap[std::to_string(dp.eventClass)];
			view.doubleBinary[dp.index].evariation = eDoubleBinaryVarMap[dp.eVariation];
			dpTypeIndexMap[dp.pointType].insert(dp.index);
		}
		else if (dp.pointType == "Counter"){
			try{
				sCounterVarMap.at(dp.sVariation);
			} catch (const std::out_of_range& ) {
				dp.sVariation = 5;
			}
			try{
				eCounterVarMap.at(dp.eVariation);
			} catch (const std::out_of_range& ) {
				dp.eVariation = 5;
			}
			view.counter[dp.index].svariation = sCounterVarMap[dp.sVariation];
			view.counter[dp.index].clazz = pointClassVarMap[std::to_string(dp.eventClass)];
			view.counter[dp.index].evariation = eCounterVarMap[dp.eVariation];
			dpTypeIndexMap[dp.pointType].insert(dp.index);
		}
		else if (dp.pointType == "Frozen Counter"){
			try{
				sFrozenCounterVarMap.at(dp.sVariation);
			} catch (const std::out_of_range& ) {
				dp.sVariation = 5;
			}
			try{
				eFrozenCounterVarMap.at(dp.eVariation);
			} catch (const std::out_of_range& ) {
				dp.eVariation = 5;
			}
			view.frozenCounter[dp.index].svariation = sFrozenCounterVarMap[dp.sVariation];
			view.frozenCounter[dp.index].clazz = pointClassVarMap[std::to_string(dp.eventClass)];
			view.frozenCounter[dp.index].evariation = eFrozenCounterVarMap[dp.eVariation];
			dpTypeIndexMap[dp.pointType].insert(dp.index);
		}
		else if (dp.pointType == "Binary Output"){
		}
		else if (dp.pointType == "Analog Output"){
		}
	}
	/*
	printf("Elements of Analog Input map are \n");
	for( std::set<int>::iterator it=dpTypeIndexMap["Analog Input"].begin(); it != dpTypeIndexMap["Analog Input"].end(); ++it){
		    std::cout << *it << ", ";
	}
	std::cout  << std::endl;
	printf("Elements of Binary Input map are \n");
	for( std::set<int>::iterator it=dpTypeIndexMap["Binary Input"].begin(); it != dpTypeIndexMap["Binary Input"].end(); ++it){
		    std::cout << *it << ", ";
	}
	std::cout  << std::endl;
	*/
}
