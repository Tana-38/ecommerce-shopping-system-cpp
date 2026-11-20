# ShopSphere - Makefile
#
#   make            build bin/shopsphere (+ the test binaries)
#   make test       build and run the unit tests
#   make demo       run the scripted feature walkthrough
#   make run        start the interactive app
#   make clean      remove build artefacts
#
# Works with GNU make on Linux/macOS, and with mingw32-make on Windows.
# (On plain Windows cmd you can also just run build.bat.)

CXX      ?= g++
STD      ?= c++14
CXXFLAGS ?= -std=$(STD) -Wall -Wextra -O2
CPPFLAGS += -Iinclude

ifeq ($(OS),Windows_NT)
  EXE := .exe
else
  EXE :=
endif

MODULES  := util core cart payment services ui
LIB_SRC  := $(foreach m,$(MODULES),$(wildcard src/$(m)/*.cpp))

# Objects are kept flat (build/core_Product.o) so only two folders are needed.
objname   = build/$(subst /,_,$(patsubst %.cpp,%,$(patsubst src/%,%,$(1)))).o
LIB_OBJ  := $(foreach s,$(LIB_SRC),$(call objname,$(s)))
MAIN_OBJ := build/main.o

APP       := bin/shopsphere$(EXE)
TEST_BINS := bin/test_all$(EXE) bin/test_partA$(EXE) bin/test_partB$(EXE)

# Create output folders up-front (portable across cmd.exe and sh).
ifeq ($(wildcard bin/.),)
  $(shell mkdir bin)
endif
ifeq ($(wildcard build/.),)
  $(shell mkdir build)
endif

.PHONY: all app tests test demo run clean
.SECONDARY:

all: app tests

app: $(APP)

tests: $(TEST_BINS)

$(APP): $(LIB_OBJ) $(MAIN_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

bin/test_%$(EXE): build/test_%.o $(LIB_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

build/main.o: src/main.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

build/test_%.o: tests/test_%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

define compile_rule
$(call objname,$(1)): $(1)
	$$(CXX) $$(CPPFLAGS) $$(CXXFLAGS) -MMD -MP -c $$< -o $$@
endef
$(foreach s,$(LIB_SRC),$(eval $(call compile_rule,$(s))))

test: tests
	bin/test_all$(EXE)
	bin/test_partA$(EXE)
	bin/test_partB$(EXE)

demo: app
	$(APP) --demo

run: app
	$(APP)

clean:
ifeq ($(OS),Windows_NT)
	-if exist build rmdir /s /q build
	-if exist bin rmdir /s /q bin
else
	$(RM) -r build bin
endif

-include $(wildcard build/*.d)
