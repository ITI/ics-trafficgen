TARGET = dnp3Generator
OBJDIR := objs
OBJS = $(addprefix $(OBJDIR)/,dnp3app.o $(CODE_OBJS))

include common.mk

#Build for OrionLX
orion: $(OBJS)
	$(CC) $(orionargs) $(CFLAGS) -o $(TARGET)-orion $(OBJS) $(LIBS32)

#Build for i386
test32: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET)-test32 $(OBJS) $(LIBS32)

#Build for ARM-RPI
rpiarm: $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(ARMLIBS)

$(OBJS): $(OBJDIR)/%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

#$(filter-out %OutStation.o,$(OBJS)): $(OBJDIR)/%.o: %.cpp %.h
#	$(CXX) $(CXXFLAGS) $(DEBUG) -c $< -o $@
