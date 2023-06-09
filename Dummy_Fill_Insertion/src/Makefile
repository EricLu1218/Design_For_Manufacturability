CXX      := g++
CXXFLAGS := -std=c++17 -O3 -Wall -Wextra -MMD
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
	$(CXX) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(EXEC) $(OBJS) $(DEPS)

ifeq (test,$(firstword $(MAKECMDGOALS)))
  TESTCASE_NAME := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  $(eval $(TESTCASE_NAME):;@:)
endif

test: $(EXEC)
	@echo test on $(TESTCASE_NAME).txt
	./$(EXEC) ../testcase/$(TESTCASE_NAME).txt ../output/$(TESTCASE_NAME).txt
	../verifier/verifier ../testcase/$(TESTCASE_NAME).txt ../output/$(TESTCASE_NAME).txt

.PHONY: all clean test
-include $(DEPS)
