CXX      := g++
CXXFLAGS := -std=c++17 -O3 -Wall -Wextra -MMD
LIBS     := -lm
EXEC     := ../bin/Fill_Insertion
SRC_DIRS := .\
			DensityManager\
			Parser\
			ResultWriter\
			Structure/Geometry
SRCS     := $(wildcard $(SRC_DIRS:=/*.cpp))
OBJS     := $(SRCS:.cpp=.o)
DEPS     := $(OBJS:.o=.d)

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CXX) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(EXEC) $(OBJS) $(DEPS)

ifeq (test, $(firstword $(MAKECMDGOALS)))
  TESTCASE := $(word 2, $(MAKECMDGOALS))
  $(eval $(TESTCASE):;@:)
endif

test: $(EXEC)
	@echo test on $(TESTCASE).txt
	./$(EXEC) ../testcase/$(TESTCASE).txt ../output/$(TESTCASE).txt
	../verifier/verifier ../testcase/$(TESTCASE).txt ../output/$(TESTCASE).txt

.PHONY: all clean test
-include $(DEPS)
