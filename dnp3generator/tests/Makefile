TARGET = dnp3GenTest
OBJDIR := ../objs

# ***IMPORTANT*** TEST FILES MUST HAVE SUFFIX *Test.cpp
OBJS = $(addprefix $(OBJDIR)/,MasterTest.o $(CODE_OBJS))

include ../common.mk

vpath % ..
$(filter %Test.o,$(OBJS)): $(OBJDIR)/%Test.o: %Test.cpp
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) -c $< -o $@

$(filter-out %Test.o,$(OBJS)): $(OBJDIR)/%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) -c $< -o $@
