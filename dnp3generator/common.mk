
# The C compiler and flags
CC=gcc
CFLAGS=-pthread -g # -wall

# The C++ compiler and flags
CXX=g++
CXXFLAGS=-pthread -std=c++11 # -wall

DEBUG_FLAGS= -DDEBUG -g -O0
RELEASE_FLAGS= -O3

SCFLAGS=-std=c++11 -fprofile-arcs -ftest-coverage -g # -wall
LFLAGS=-lgcov --coverage
SGCCFLAGS= -static-libgcc -static-libstdc++

CODE_OBJS = Station.o MasterStation.o OutStation.o MappingOutstation.o Node.o CidrCalculator.o StringUtilities.o DataPoint.o CfgJsonParser.o MappingSoeHandler.o

DNP3_LIBS = -lasiodnp3 -lasiopal -lopendnp3 -lopenpal -lstdc++
LUA_LIBS = -llua5.2
BOOST_LIBS = -lboost_system -lboost_thread

#x86_64 Configuration
LIBS = $(DNP3_LIBS) $(LUA_LIBS) $(BOOST_LIBS)

#x86_32 Configuration
LIBS32 = $(DNP3_LIBS) $(subst x86_64,i386,$(LUA_LIBS) $(BOOST_LIBS))

#x86_32 Configuration--Static
SLIBS32 = $(LIBS32:%.so=%.a) -ldl

#RPI-ARM Configuration
ARMLIBS = $(DNP3_LIBS) $(subst x86_64-linux-gnu,arm-linux-gnueabihf,$(LUA_LIBS) $(BOOST_LIBS))

#OrionLX Static Configuration
sorionargs = -Wl,--whole-archive -lpthread -Wl,--no-whole-archive

#OrionLX Static/Dynamic Configuration
sdorionargs = -Wl,--dynamic-linker=/usr/local/dnp3gen/ld-dnp3gen.so $(sorionargs) -Wl,-Bdynamic /usr/local/lib/libdummy.so

all: $(TARGET)

#Static Build for OrionLX
sorion: $(OBJS)
	$(CXX) $(sorionargs) $(SCFLAGS) $(LFLAGS) -static $(SGCCFLAGS) -o $(TARGET)-orionstatic $(OBJS) $(SLIBS32)

#Static Build for RPI/ARM
srpiarm: $(OBJS)
	$(CXX) $(sorionargs) $(SCFLAGS) $(LFLAGS) -static $(SGCCFLAGS) -o $(TARGET)-armstatic $(OBJS) $(SLIBS32)

#Static/Dynamic Build for OrionLX with dummy lib for injection
sdorion: $(OBJS)
	$(CXX) $(sdorionargs) $(SCFLAGS) $(LFLAGS) $(SGCCFLAGS) -o $(TARGET)-orionsd $(OBJS) $(SLIBS32)
	patchelf --set-rpath /usr/local/dnp3gen $(TARGET)-orionsd
	cd dummy && $(MAKE) dummy


$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(DEBUG) -o $(TARGET) $(OBJS) $(LIBS)

.PHONY: clean all
clean:
	rm -f $(OBJS) $(TARGET) $(TARGET)-orion $(TARGET)-test32 $(TARGET)-orionstatic $(TARGET)-orionsd $(TARGET)-armstatic
	cd dummy && $(MAKE) clean

#$(filter %OutStation.o,$(OBJS)): $(OBJDIR)/%.o: %.cpp %.h
#	$(CXX) $(CXXFLAGS) $(DEBUG) -c $< -o $@ -llua5.2

#$(OBJS): $(OBJDIR)/%.o: %.cpp %.h
#	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) -c $< -o $@

$(OBJS): | $(OBJDIR)

$(OBJDIR):
	mkdir $(OBJDIR)
